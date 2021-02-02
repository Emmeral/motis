#pragma once

#include <cstdint>
#include "motis/core/schedule/nodes.h"
#include "motis/core/schedule/schedule.h"
#include "motis/routing/search_query.h"

namespace motis::routing {
template <typename Label>
struct lower_bounds_result;

using time_diff_t = uint32_t;
using interchanges_t = uint32_t;

template <typename Label>
class lower_bounds {
public:
  lower_bounds() = delete;
  lower_bounds(search_query const& routing_query, search_dir search_direction)
      : routing_query_(routing_query), search_direction_(search_direction) {}

  virtual time_diff_t time_from_node(node const* /*n*/) const { return 0; };
  virtual time_diff_t time_from_label(Label& l) const {
    return time_from_node(l.get_node());
  }
  virtual bool is_valid_time_diff(time_diff_t /*diff*/) const { return true; };

  virtual interchanges_t transfers_from_label(Label& l) const {
    return transfers_from_node(l.get_node());
  };
  virtual interchanges_t transfers_from_node(node const* /*n*/) const { return 0; };
  virtual bool is_valid_transfer_amount(interchanges_t /*amount*/) const {
    return true;
  };

  virtual uint16_t price_from_label(Label& l) {
    return price_from_node(l.get_node());
  };
  virtual uint16_t price_from_node(node const* /*n*/)  { return 0; };
  virtual bool is_valid_price_amount(uint16_t /*amount*/) const { return true;};

  virtual lower_bounds_result<Label> calculate() = 0;

  /**
   * Returns whether the label is on an optimal journey regarding the travel
   * time. If not known whether this is true returns false.
   * @param l
   * @return true if on optimal journey regarding travel time
   */
  virtual bool is_on_optimal_time_journey(Label& /*l*/) const { return false; }
  /**
   * Returns whether the label is on an optimal journey regarding transfers.
   * If not known whether this is true returns false.
   * @param l
   * @return true if on optimal journey regarding transfers
   */
  virtual bool is_on_optimal_transfers_journey(Label& /*l*/) const {
    return false;
  }

  /**
   * Returns the amount of optimal journeys known to exist. Used by pareto
   * dijkstra to determine whether all optimal connections have been found.
   * @return
   */
  virtual int get_optimal_journey_count() const { return 0; }

  virtual ~lower_bounds() = default;

protected:
  search_query const& routing_query_;
  search_dir const search_direction_;
};

template <typename Label>
struct lower_bounds_result {

  std::unique_ptr<lower_bounds<Label>> bounds_{};
  bool target_reachable{true};
  uint64_t travel_time_lb_{0};
  uint64_t transfers_lb_{0};
  uint64_t price_lb_{0};
  uint64_t optimality_lb_{0};
  uint64_t total_lb{0};
};



}  // namespace motis::routing
