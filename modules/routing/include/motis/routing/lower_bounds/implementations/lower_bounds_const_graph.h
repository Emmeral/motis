#pragma once

#include "motis/hash_map.h"
#include "lower_bounds.h"

#include "motis/core/common/timing.h"
#include "motis/core/schedule/constant_graph.h"
#include "motis/core/schedule/schedule.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {
template <typename Label>
class lower_bounds_const_graph : public lower_bounds<Label> {

public:
  lower_bounds_const_graph(
      search_query const& routing_query, search_dir const& search_direction,
      const std::vector<int>& goals)
      : lower_bounds<Label>(routing_query, search_direction),
        travel_time_(search_direction == search_dir::FWD
                     ? this->routing_query_.sched_->travel_time_lower_bounds_fwd_
                     : this->routing_query_.sched_->travel_time_lower_bounds_bwd_,
                     goals, additional_time_edges_),
        transfers_(search_direction == search_dir::FWD
                   ? this->routing_query_.sched_->transfers_lower_bounds_fwd_
                   : this->routing_query_.sched_->transfers_lower_bounds_bwd_,
                   goals, additional_transfer_edges_,
                   map_interchange_graph_node(
                       this->routing_query_.sched_->station_nodes_.size())) {

    auto const route_offset = this->routing_query_.sched_->station_nodes_.size();
    // construct the lower bounds graph for the additional edges
    for (auto const& e : this->routing_query_.query_edges_) {
      auto const from_node = (this->search_direction_ == search_dir::FWD) ? e.from_ : e.to_;
      auto const to_node = (this->search_direction_ == search_dir::FWD) ? e.to_ : e.from_;

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
  time_diff_t time_from_node(node const* n) const override {
    return travel_time_[n];
  }
  bool is_valid_time_diff(time_diff_t time) const override {
    return travel_time_.is_reachable(time);
  };
  interchanges_t transfers_from_node(node const* n) const override {
    return transfers_[n];
  }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return transfers_.is_reachable(amount);
  };

  lower_bounds_result<Label> calculate() override {

    auto result = lower_bounds_result<Label>{};

    MOTIS_START_TIMING(travel_time_timing);
    calculate_timing();
    MOTIS_STOP_TIMING(travel_time_timing);

    result.travel_time_lb_ = MOTIS_TIMING_MS(travel_time_timing);

    // questionable if condition might be removed
    if (!is_valid_time_diff(time_from_node(this->routing_query_.from_))) {
      result.target_reachable = false;
      return result;
    }

    MOTIS_START_TIMING(transfers_timing);
    calculate_transfers();
    MOTIS_STOP_TIMING(transfers_timing);
    result.transfers_lb_ = MOTIS_TIMING_MS(transfers_timing);
    result.total_lb = result.transfers_lb_ + result.travel_time_lb_;
    return result;
  }
  void calculate_timing() { travel_time_.run(); }
  void calculate_transfers() { transfers_.run(); }

private:
  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_time_edges_;
  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_transfer_edges_;

  constant_graph_dijkstra<MAX_TRAVEL_TIME, map_station_graph_node> travel_time_;
  constant_graph_dijkstra<MAX_TRANSFERS, map_interchange_graph_node> transfers_;
};

}  // namespace motis::routing
