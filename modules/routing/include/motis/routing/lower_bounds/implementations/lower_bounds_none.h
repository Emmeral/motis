#pragma once

#include "motis/routing/lower_bounds/implementations/lower_bounds.h"

#include "motis/core/common/timing.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {
/**
 * lower_bounds implementation which does not calculate any lower bounds.
 * Exists only for testing purposes.
 */
template <typename Label>
class lower_bounds_none : public lower_bounds<Label> {

public:
  lower_bounds_none(search_query const& routing_query, search_dir dir)
      : lower_bounds<Label>(routing_query, dir){};
  time_diff_t time_from_node(node const* /*n*/) const override { return 0; };
  bool is_valid_time_diff(time_diff_t /*time*/) const override { return true; }
  interchanges_t transfers_from_node(node const* /*n*/) const override {
    return 0;
  };
  bool is_valid_transfer_amount(interchanges_t /* amount */) const override {
    return true;
  }

  lower_bounds_result<Label> calculate() override{
    return lower_bounds_result<Label>{};
  }

private:
};

}  // namespace motis::routing
