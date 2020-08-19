
#include "motis/routing/lower_bounds/lower_bounds_mixed.h"

namespace motis::routing {

lower_bounds_mixed::lower_bounds_mixed(search_query const& routing_query,
                                       search_dir const& search_direction,
                                       const std::vector<int>& goals)
    : lower_bounds(routing_query, search_direction),
      csa_lower_bounds_(routing_query, search_direction),
      cg_lower_bounds(routing_query, search_direction, goals) {}

bool lower_bounds_mixed::calculate_timing() {
  return csa_lower_bounds_.calculate();
}

void lower_bounds_mixed::calculate_transfers() {
  cg_lower_bounds.calculate_transfers();
}

lower_bounds::time_diff_t lower_bounds_mixed::time_from_node(
    node const* n) const {
  return csa_lower_bounds_.time_from_node(n);
}
bool lower_bounds_mixed::is_valid_time_diff(time_diff_t time) const {
  return csa_lower_bounds_.is_valid_time_diff(time);
}
lower_bounds::interchanges_t lower_bounds_mixed::transfers_from_node(
    node const* n) const {
  return cg_lower_bounds.transfers_from_node(n);
}
bool lower_bounds_mixed::is_valid_transfer_amount(interchanges_t amount) const {
  return cg_lower_bounds.is_valid_transfer_amount(amount);
}

}  // namespace motis::routing
