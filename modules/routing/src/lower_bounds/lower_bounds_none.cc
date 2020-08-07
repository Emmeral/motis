

#include "motis/routing/lower_bounds/lower_bounds_none.h"

namespace motis::routing {

lower_bounds_none::lower_bounds_none() {}

lower_bounds::time_diff_t lower_bounds_none::time_from_node(
    node const* /*n*/) const {
  return 0;
}
bool lower_bounds_none::is_valid_time_diff(time_diff_t /*time*/) const {
  return true;
}
lower_bounds::interchanges_t lower_bounds_none::transfers_from_node(
    node const* /*n*/) const {
  return 0;
}
bool lower_bounds_none::is_valid_transfer_amount(
    interchanges_t /*amount*/) const {
  return true;
}

}  // namespace motis::routing
