
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include <motis/core/schedule/interval.h>
#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>

namespace motis::routing {

// in caps?
constexpr lower_bounds::interchanges_t invalid_interchange_amount =
    std::numeric_limits<lower_bounds::interchanges_t>::max();
constexpr lower_bounds::time_diff_t invalid_time_diff =
    std::numeric_limits<lower_bounds::time_diff_t>::max();

lower_bounds_csa::lower_bounds_csa(search_query const& routing_query,
                                   search_dir direction)
    : routing_query_{routing_query}, direction_{direction}, bounds_() {}

lower_bounds::time_diff_t lower_bounds_csa::time_from_node(node const* n) {
  return bounds_[n->id_].time_diff_;
}
bool lower_bounds_csa::is_valid_time_diff(time_diff_t diff) {
  return diff != invalid_time_diff;
}
lower_bounds::interchanges_t lower_bounds_csa::transfers_from_node(
    node const* n) {
  return bounds_[n->id_].transfer_amount;
}
bool lower_bounds_csa::is_valid_transfer_amount(interchanges_t amount) {
  return amount != invalid_interchange_amount;
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

  // do I use the correct ID?
  auto arrival_times = times[to_id];

  const search_dir notDir =
      direction_ == search_dir::FWD ? search_dir::BWD : search_dir::FWD;

  // check if target is reachable and find the latest arrival time
  bool valid{false};
  time last_arrival{0};
  for (auto t : arrival_times) {
    if (t != INVALID_TIME && t > last_arrival) {
      last_arrival = t;
      valid = true;
    }
  }

  // TODO return if not valid. Also what if the interval can be extended?

  // signals ontrip station start because no end of interval
  interval backwards_interval{last_arrival};

  csa::csa_query backwards_query(to_id, from_id, backwards_interval, notDir);

  auto const backwards_times = motis::csa::get_arrival_times(
      *routing_query_.csa_timetable, backwards_query);

  std::transform(backwards_times.begin(), backwards_times.end(),
                 bounds_.begin(), [&last_arrival](auto arr) {
                   auto minIt = std::min_element(arr.begin().arr.end());
                   time minElem = *minIt;
                   interchanges_t amount;
                   time_diff_t diff;

                   if (minElem < INVALID_TIME) {
                     amount = *minIt;
                     diff = minElem - last_arrival;
                   } else {
                     amount = invalid_interchange_amount;
                     diff = invalid_time_diff;
                   }
                   return combined_bound{diff, amount};
                 });
}

}  // namespace motis::routing