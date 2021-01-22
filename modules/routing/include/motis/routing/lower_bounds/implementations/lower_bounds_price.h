#pragma once

#include "lower_bounds.h"

#include "motis/core/schedule/schedule.h"

#include "motis/core/schedule/price.h"

constexpr uint8_t SUBTRACTED_POSSIBLE_FOOTPATH_KM = 5;

namespace motis::routing {
template <typename Label>
class lower_bounds_price {

public:
  lower_bounds_price(search_query const& routing_query,
                     const std::vector<int>& goals)
      : routing_query_(routing_query), remaining_prices_(), goal_ids_{goals} {}

  uint16_t price_from_label(Label& l) { return price_from_node(l.get_node()); }

  uint16_t price_from_node(node const* n) {
    assert(n->get_station()->id_ <= remaining_prices_.size());
    auto station_id = n->get_station()->id_;
    auto price = remaining_prices_[station_id];
    if (price == std::numeric_limits<uint16_t>::max()) {
      return calculate_for(station_id);
    }

    return price;
  }
  bool is_valid_price_amount(uint16_t amount) const {
    return amount < std::numeric_limits<uint16_t>::max();
  }

  lower_bounds_result<Label> calculate() {
    MOTIS_START_TIMING(price_timing);
    remaining_prices_ =
        std::vector<uint16_t>(routing_query_.sched_->stations_.size(),
                              std::numeric_limits<uint16_t>::max());
    MOTIS_STOP_TIMING(price_timing);

    auto r = lower_bounds_result<Label>();
    r.price_lb_ = MOTIS_TIMING_MS(price_timing);
    return r;
  }

private:
  uint16_t calculate_for(node_id_t station_id) {

    schedule const& sched = *this->routing_query_.sched_;
    const station_ptr& source =
        this->routing_query_.sched_->stations_[station_id];

    // TODO: consider more goal ids;
    const station_ptr& target = sched.stations_[goal_ids_[0]];

    auto distance_km = get_distance(*source, *target);
    // TODO: Footpath handling could be more sophisticated
    distance_km = std::max(0.0, distance_km - SUBTRACTED_POSSIBLE_FOOTPATH_KM);

    remaining_prices_[station_id] = distance_km * sched.cheapest_price_per_km_;

    return remaining_prices_[station_id];
  }

private:
  search_query const& routing_query_;
  std::vector<uint16_t> remaining_prices_;
  const std::vector<int>& goal_ids_;
};

}  // namespace motis::routing
