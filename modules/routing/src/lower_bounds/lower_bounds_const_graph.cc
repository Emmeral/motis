
#include "motis/routing/lower_bounds/lower_bounds_const_graph.h"

namespace motis::routing {

lower_bounds_const_graph::lower_bounds_const_graph(
    search_query const& routing_query, search_dir const& search_direction,
    const std::vector<int>& goals)
    : lower_bounds(routing_query, search_direction),
      travel_time_(search_direction == search_dir::FWD
                       ? routing_query_.sched_->travel_time_lower_bounds_fwd_
                       : routing_query_.sched_->travel_time_lower_bounds_bwd_,
                   goals, additional_time_edges_),
      transfers_(search_direction == search_dir::FWD
                     ? routing_query_.sched_->transfers_lower_bounds_fwd_
                     : routing_query_.sched_->transfers_lower_bounds_bwd_,
                 goals, additional_transfer_edges_,
                 map_interchange_graph_node(
                     routing_query_.sched_->station_nodes_.size())) {

  auto const route_offset = routing_query_.sched_->station_nodes_.size();
  // construct the lower bounds graph for the additional edges
  for (auto const& e : routing_query_.query_edges_) {
    auto const from_node = (search_direction_ == search_dir::FWD) ? e.from_ : e.to_;
    auto const to_node = (search_direction_ == search_dir::FWD) ? e.to_ : e.from_;

    // station graph
    auto const from_station = from_node->get_station()->id_;
    auto const to_station = to_node->get_station()->id_;

    // interchange graph
    auto const from_interchange = from_node->is_route_node()
                                  ? route_offset + from_node->route_
                                  : from_station;
    auto const to_interchange = to_node->is_route_node()
                                ? route_offset + to_node->route_
                                : to_station;

    auto const ec = e.get_minimum_cost();

    additional_time_edges_[to_station].emplace_back(
        simple_edge{from_station, ec.time_});
    additional_transfer_edges_[to_interchange].emplace_back(simple_edge{
        from_interchange, static_cast<uint16_t>(ec.transfer_ ? 1 : 0)});
  }
}

void lower_bounds_const_graph::calculate_timing() { travel_time_.run(); }

void lower_bounds_const_graph::calculate_transfers() { transfers_.run(); }

lower_bounds::time_diff_t lower_bounds_const_graph::time_from_node(
    node const* n) const {
  return travel_time_[n];
}
bool lower_bounds_const_graph::is_valid_time_diff(time_diff_t time) const {
  return travel_time_.is_reachable(time);
}
lower_bounds::interchanges_t lower_bounds_const_graph::transfers_from_node(
    node const* n) const {
  return transfers_[n];
}
bool lower_bounds_const_graph::is_valid_transfer_amount(
    interchanges_t amount) const {
  return transfers_.is_reachable(amount);
}

}  // namespace motis::routing
