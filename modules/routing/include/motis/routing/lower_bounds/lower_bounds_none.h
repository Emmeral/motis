#pragma once

#include "motis/routing/lower_bounds/lower_bounds.h"

#include "motis/core/common/timing.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {
/**
 * lower_bounds implementation which does not calculate any lower bounds.
 * Exists only for testing purposes.
 */
class lower_bounds_none : public lower_bounds {

public:
  lower_bounds_none(search_query const& routing_query, search_dir dir);
  time_diff_t time_from_node(node const* n) const override;
  bool is_valid_time_diff(time_diff_t time) const override;
  interchanges_t transfers_from_node(node const* n) const override;
  bool is_valid_transfer_amount(interchanges_t amount) const override;

private:
};

}  // namespace motis::routing