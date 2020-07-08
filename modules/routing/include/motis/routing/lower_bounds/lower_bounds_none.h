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
  lower_bounds_none();
  time_diff_t time_from_node(node const* n) override;
  bool is_valid_time_diff(time_diff_t time) override;
  interchanges_t transfers_from_node(node const* n) override;
  bool is_valid_transfer_amount(interchanges_t amount) override;

private:
};

}  // namespace motis::routing
