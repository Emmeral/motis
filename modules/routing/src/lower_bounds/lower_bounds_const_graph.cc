
#include "motis/routing/lower_bounds/lower_bounds_const_graph.h"

namespace motis::routing {

lower_bounds_const_graph::lower_bounds_const_graph(
    const schedule& sched, const constant_graph& travel_time_graph,
    const constant_graph& transfers_graph, const std::vector<int>& goals,
    const mcd::hash_map<unsigned int, std::vector<simple_edge>>&
        additional_travel_time_edges,
    const mcd::hash_map<unsigned int, std::vector<simple_edge>>&
        additional_transfers_edges)
    : travel_time_(travel_time_graph, goals, additional_travel_time_edges),
      transfers_(transfers_graph, goals, additional_transfers_edges,
                 map_interchange_graph_node(sched.station_nodes_.size())) {}

void lower_bounds_const_graph::calculate_timing() { travel_time_.run(); }

void lower_bounds_const_graph::calculate_transfers() { transfers_.run(); }

lower_bounds::time_diff_t lower_bounds_const_graph::time_from_node(node const* n) {
  return travel_time_[n];
}
bool lower_bounds_const_graph::is_valid_time_diff(time_diff_t time) {
  return travel_time_.is_reachable(time);
}
lower_bounds::interchanges_t lower_bounds_const_graph::transfers_from_node(node const* n) {
  return transfers_[n];
}
bool lower_bounds_const_graph::is_valid_transfer_amount(interchanges_t amount) {
  return transfers_.is_reachable(amount);
}

}  // namespace motis::routing