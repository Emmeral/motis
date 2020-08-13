
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include <motis/core/schedule/interval.h>
#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>

namespace motis::routing {

lower_bounds_csa::lower_bounds_csa(search_query const& routing_query,
                                   search_dir direction)
    : routing_query_{routing_query},
      direction_{direction},
      bounds_(routing_query.sched_->stations_.size()) {

  // it is tricky thinking about forward and backwards search. Also CSA returns
  // 0 as invalid if searching backwards
  // the variables are names as if the search direction was forward
  invalid_time_ = direction_ == search_dir::FWD
                      ? std::numeric_limits<time>::max()
                      : std::numeric_limits<time>::min();
  minimal_time_ = direction_ == search_dir::FWD
                      ? std::numeric_limits<time>::min()
                      : std::numeric_limits<time>::max();
}

lower_bounds::time_diff_t lower_bounds_csa::time_from_node(
    node const* n) const {
  return bounds_[n->get_station()->id_].time_diff_;
}
bool lower_bounds_csa::is_valid_time_diff(time_diff_t diff) const {
  return diff != INVALID_TIME_DIFF;
}
lower_bounds::interchanges_t lower_bounds_csa::transfers_from_node(
    node const* n) const {
  return bounds_[n->get_station()->id_].transfer_amount_;
}
bool lower_bounds_csa::is_valid_transfer_amount(interchanges_t amount) const {
  return amount != INVALID_INTERCHANGE_AMOUNT;
}

using csa_arrival_times_t =
    std::vector<std::array<time, csa::MAX_TRANSFERS + 1>>;

bool lower_bounds_csa::calculate() {
  // TODO throw error if csa would decline request
  // TODO: further think about on- vs pretrip
  interval search_interval{routing_query_.interval_begin_,
                           routing_query_.interval_end_};

  auto const& from_id = routing_query_.from_->id_;
  auto const& to_id = routing_query_.to_->id_;

  // TODO: Meta start/target not considered so far
  csa::csa_query initial_query(from_id, to_id, search_interval, direction_);

  auto const times = motis::csa::get_arrival_times(
      *routing_query_.csa_timetable, initial_query);

  auto arrival_times = times[to_id];

  const search_dir notDir =
      direction_ == search_dir::FWD ? search_dir::BWD : search_dir::FWD;

  std::vector<time> valid_arrival_times;
  std::copy_if(arrival_times.begin(), arrival_times.end(),
               std::back_inserter(valid_arrival_times),
               [inv_time = invalid_time_](time t) { return t != inv_time; });

  // Return if no valid arrival could be found. Note interval extension not
  // included
  if (valid_arrival_times.empty()) {
    return false;
  }

  station_ptr const& to_station = routing_query_.sched_->stations_[to_id];
  auto const offset = direction_ == search_dir::FWD
                          ? +to_station->transfer_time_
                          : -to_station->transfer_time_;

  // look if the station can be reached from other stations by foot to prevent a
  // potential additional query
  bool to_reachable_by_foot{false};
  if (direction_ == search_dir::FWD) {
    auto const& footpaths =
        routing_query_.csa_timetable->stations_[to_id].incoming_footpaths_;
    for (auto const& fp : footpaths) {
      to_reachable_by_foot |= fp.from_station_ != to_id;
    }
  } else {
    auto const& footpaths =
        routing_query_.csa_timetable->stations_[to_id].footpaths_;
    for (auto const& fp : footpaths) {
      to_reachable_by_foot |= fp.to_station_ != to_id;
    }
  }

  for (auto arrival_time : valid_arrival_times) {

    // Subtract the transfer time from the last station to get the real arrival.
    const time real_arrival = arrival_time - offset;

    // signals ontrip station start because no end of interval
    const interval backwards_interval{real_arrival};
    const csa::csa_query backwards_query(to_id, from_id, backwards_interval, notDir);

    process_backwards_query(backwards_query);

    // if the station was reached by foot the subtraction of the offset leads to
    // potentially wrong results. Therefore another backwards query has to be
    // submitted
    if (to_reachable_by_foot) {
      const interval foot_backwards_interval{arrival_time};
      const csa::csa_query foot_backwards_query(to_id, from_id, foot_backwards_interval, notDir);
      process_backwards_query(foot_backwards_query);
    }
  }

  // decrease the lower bounds by the interchange time as they will be to high
  // for route nodes otherwise. TODO: this could make them to low to
  auto const& all_stations = routing_query_.sched_->stations_;
  for (auto i = 0; i < all_stations.size(); ++i) {
    if (is_valid_time_diff(bounds_[i].time_diff_)) {
      time_diff_t subtract = std::max(all_stations[i]->transfer_time_, 0);
      bounds_[i].time_diff_ -= std::min(subtract, bounds_[i].time_diff_);
    }
  }

  return true;
}
void lower_bounds_csa::process_backwards_query(csa::csa_query const& query) {
  // use a different timetable for the backward search because otherwise
  // "infinite" lb are produced for stations where you can't enter or exit
  // (Hildesheim Gbr)
  auto const backwards_times = motis::csa::get_arrival_times(
      *routing_query_.csa_timetable_ignored_restrictions, query);

  // get the lower bounds for each node and look if they were decreased
  for (auto i = 0; i < backwards_times.size(); ++i) {
    combined_bound b =
        get_best_bound_for(query.search_interval_.begin_, backwards_times[i]);
    bounds_[i].update_with(b);
  }
}
lower_bounds_csa::combined_bound lower_bounds_csa::get_best_bound_for(
    time search_start,
    const std::array<time, csa::MAX_TRANSFERS + 1>& arr) const {

  time latest_time{minimal_time_};
  interchanges_t least_interchanges{INVALID_INTERCHANGE_AMOUNT};
  bool min_interchanges_found = false;
  // find the "latest" departure time for one station (since the response came
  // from a backwards search the latest departure time corresponds to the
  // fastest connections) and also the least amount of interchanges
  for (auto i = 0; i < arr.size(); ++i) {
    if (earlier(latest_time, arr[i])) {
      latest_time = arr[i];
      if (!min_interchanges_found) {
        least_interchanges = i;
        min_interchanges_found = true;
      }
    }
  }
  time_diff_t diff;

  if (earlier(minimal_time_, latest_time)) {
    // abs(a - b)
    diff = search_start > latest_time ? search_start - latest_time
                                      : latest_time - search_start;
  } else {
    diff = INVALID_TIME_DIFF;
  }
  return combined_bound{diff, least_interchanges};
}
bool lower_bounds_csa::earlier(time t1, time t2) const {
  return direction_ == search_dir::FWD ? t1 < t2 : t1 > t2;
}

}  // namespace motis::routing
