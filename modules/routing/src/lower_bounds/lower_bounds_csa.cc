
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include <motis/core/schedule/interval.h>
#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>

namespace motis::routing {

lower_bounds_csa::lower_bounds_csa(search_query const& routing_query,
                                   search_dir direction)
    : routing_query_{routing_query},
      direction_{direction},
      bounds_(routing_query.sched_->stations_.size()) {}

lower_bounds::time_diff_t lower_bounds_csa::time_from_node(node const* n) {
  return bounds_[n->get_station()->id_].time_diff_;
}
bool lower_bounds_csa::is_valid_time_diff(time_diff_t diff) {
  return diff != INVALID_TIME_DIFF;
}
lower_bounds::interchanges_t lower_bounds_csa::transfers_from_node(
    node const* n) {
  return bounds_[n->get_station()->id_].transfer_amount;
}
bool lower_bounds_csa::is_valid_transfer_amount(interchanges_t amount) {
  return amount != INVALID_INTERCHANGE_AMOUNT;
}

void lower_bounds_csa::calculate() {
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

  // it is tricky thinking about forward and backwards search. Also CSA returns
  // 0 as invalid if searching backwards
  // the variables are names as if the search direction was forward
  auto const invalid_time = direction_ == search_dir::FWD
                                ? std::numeric_limits<time>::max()
                                : std::numeric_limits<time>::min();
  auto const minimal_time = direction_ == search_dir::FWD
                                ? std::numeric_limits<time>::min()
                                : std::numeric_limits<time>::max();

  auto const earlier =
      direction_ == search_dir::FWD
          ? [](const time t1, const time t2) { return t1 < t2; }
          : [](const time t1, const time t2) { return t1 > t2; };

  const search_dir notDir =
      direction_ == search_dir::FWD ? search_dir::BWD : search_dir::FWD;

  // find "greatest" time which is not the invalid one
  bool valid{false};
  time last_arrival{minimal_time};
  for (auto t : arrival_times) {
    if (t != invalid_time && earlier(last_arrival, t)) {
      last_arrival = t;
      valid = true;
    }
  }

  // TODO return if not valid. Also what if the interval can be extended?
  if (!valid) {
    return;
  }

  // signals ontrip station start because no end of interval
  interval backwards_interval{last_arrival};

  csa::csa_query backwards_query(to_id, from_id, backwards_interval, notDir);

  auto const backwards_times = motis::csa::get_arrival_times(
      *routing_query_.csa_timetable, backwards_query);

  std::transform(
      backwards_times.begin(), backwards_times.end(), bounds_.begin(),
      [last_arrival, &earlier, minimal_time](auto arr) {
        auto last_departure_it =
            std::max_element(arr.begin(), arr.end(), earlier);
        time last_departure = *last_departure_it;
        interchanges_t amount;
        time_diff_t diff;

        if (earlier(minimal_time, last_departure)) {
          amount = std::distance(arr.begin(), last_departure_it);
          diff = last_arrival > last_departure ? last_arrival - last_departure
                                               : last_departure - last_arrival;
        } else {
          amount = INVALID_INTERCHANGE_AMOUNT;
          diff = INVALID_TIME_DIFF;
        }
        return combined_bound{diff, amount};
      });
}

}  // namespace motis::routing