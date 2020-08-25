
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include "motis/csa/cpu/csa_search_default_cpu_multistart.h"
#include "motis/csa/csa.h"

namespace motis::routing {

using csa::cpu::multistart::csa_search;
using csa::cpu::multistart::search_start_point;

lower_bounds_csa::lower_bounds_csa(search_query const& routing_query,
                                   search_dir direction)
    : lower_bounds(routing_query, direction),
      bounds_(routing_query.sched_->stations_.size()) {

  invalid_time_ = search_direction_ == search_dir::FWD
                      ? std::numeric_limits<time>::max()
                      : std::numeric_limits<time>::min();
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
  csa::csa_query initial_query(from_id, to_id, search_interval,
                               search_direction_);

  auto const times = motis::csa::get_arrival_times(
      *routing_query_.csa_timetable, initial_query);

  auto arrival_times = times[to_id];


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
  auto const offset = search_direction_ == search_dir::FWD
                          ? +to_station->transfer_time_
                          : -to_station->transfer_time_;

  // look if the station can be reached from other stations by foot to prevent a
  // potential additional query
  bool to_reachable_by_foot{false};
  csa::csa_station const& csa_to_station =
      routing_query_.csa_timetable->stations_[to_id];

  if (search_direction_ == search_dir::FWD) {
    auto const& footpaths = csa_to_station.incoming_footpaths_;
    for (auto const& fp : footpaths) {
      to_reachable_by_foot |= fp.from_station_ != to_id;
    }
  } else {
    auto const& footpaths = csa_to_station.footpaths_;
    for (auto const& fp : footpaths) {
      to_reachable_by_foot |= fp.to_station_ != to_id;
    }
  }

  std::vector<search_start_point> starts;

  for (auto arrival_time : valid_arrival_times) {

    // Subtract the transfer time from the last station to get the real arrival.
    const time real_arrival = arrival_time - offset;

    starts.push_back(search_start_point{csa_to_station, real_arrival});

    // if the station was reached by foot the subtraction of the offset leads to
    // potentially wrong results. Therefore another backwards query has to be
    // submitted
    if (to_reachable_by_foot) {
      starts.push_back(search_start_point{csa_to_station, arrival_time});
    }
  }

  std::vector<std::array<time, MAX_TRANSFERS + 1>> response;
  if (search_direction_ == search_dir::FWD) {
    auto s = csa_search<search_dir::BWD>(
        *routing_query_.csa_timetable_ignored_restrictions, starts);
    s.search();
    response = s.needed_time_;
  }
  if (search_direction_ == search_dir::BWD) {
    auto s = csa_search<search_dir::FWD>(
        *routing_query_.csa_timetable_ignored_restrictions, starts);
    s.search();
    response = s.needed_time_;
  }

  auto const& all_stations = routing_query_.sched_->stations_;
  // get the lower bounds for each node and look if they were decreased
  for (auto i = 0; i < response.size(); ++i) {
    bounds_[i] = get_best_bound_for(response[i]);
    // decrease the lower bounds by the interchange time as they will be to high
    // for route nodes otherwise. TODO: this could make them too low to
    if (is_valid_time_diff(bounds_[i].time_diff_)) {
      time_diff_t subtract = std::max(all_stations[i]->transfer_time_, 0);
      bounds_[i].time_diff_ -= std::min(subtract, bounds_[i].time_diff_);
    }
  }

  return true;
}

lower_bounds_csa::combined_bound lower_bounds_csa::get_best_bound_for(
    const std::array<time, csa::MAX_TRANSFERS + 1>& arr) const {

  time min_time{std::numeric_limits<time>::max()};
  interchanges_t least_interchanges{INVALID_INTERCHANGE_AMOUNT};
  bool min_interchanges_found = false;

  for (auto i = 0; i < arr.size(); ++i) {
    if (arr[i] < min_time) {
      min_time = arr[i];
      if (!min_interchanges_found) {
        least_interchanges = i;
        min_interchanges_found = true;
      }
    }
  }

  time_diff_t diff;
  if (min_time != std::numeric_limits<time>::max()) {
    diff = min_time;
  } else {
    diff = INVALID_TIME_DIFF;
  }

  return combined_bound{diff, least_interchanges};
}

}  // namespace motis::routing
