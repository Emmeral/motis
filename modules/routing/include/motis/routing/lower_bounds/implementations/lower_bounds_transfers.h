#pragma once

#pragma once

#include "motis/hash_map.h"
#include "lower_bounds.h"
#include "lower_bounds_price.h"

#include "motis/core/common/timing.h"
#include "motis/core/schedule/constant_graph.h"
#include "motis/core/schedule/schedule.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {
template <typename Label>
class lower_bounds_transfers {

public:
  lower_bounds_transfers(search_query const& routing_query,
                         search_dir const& search_direction,
                         const std::vector<int>& goals)
      : routing_query_(routing_query),
        search_direction_(search_direction),
        transfers_(
            search_direction == search_dir::FWD
                ? this->routing_query_.sched_->transfers_lower_bounds_fwd_
                : this->routing_query_.sched_->transfers_lower_bounds_bwd_,
            goals, additional_transfer_edges_,
            map_interchange_graph_node(
                this->routing_query_.sched_->station_nodes_.size())) {

    auto const route_offset =
        this->routing_query_.sched_->station_nodes_.size();
    // construct the lower bounds graph for the additional edges
    for (auto const& e : this->routing_query_.query_edges_) {
      auto const from_node =
          (this->search_direction_ == search_dir::FWD) ? e.from_ : e.to_;
      auto const to_node =
          (this->search_direction_ == search_dir::FWD) ? e.to_ : e.from_;

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

      additional_transfer_edges_[to_interchange].emplace_back(simple_edge{
          from_interchange, static_cast<uint16_t>(ec.transfer_ ? 1 : 0)});
    }
  }

  lower_bounds_transfers(lower_bounds_transfers&& other) noexcept
      : routing_query_(std::move(other.routing_query_)),
        search_direction_(std::move(other.search_direction_)),
        additional_transfer_edges_(std::move(other.additional_transfer_edges_)),
        transfers_(std::move(other.transfers_), additional_transfer_edges_) {}

  interchanges_t transfers_from_label(Label& l) const {
    return transfers_from_node(l.get_node());
  };
  interchanges_t transfers_from_node(node const* n) const {
    return transfers_[n];
  }
  bool is_valid_transfer_amount(interchanges_t amount) const {
    return transfers_.is_reachable(amount);
  };

  lower_bounds_result<Label> calculate() {

    auto result = lower_bounds_result<Label>{};

    MOTIS_START_TIMING(transfers_timing);
    transfers_.run();
    MOTIS_STOP_TIMING(transfers_timing);

    result.transfers_lb_ = MOTIS_TIMING_MS(transfers_timing);

    // questionable if condition might be removed
    if (!is_valid_transfer_amount(
            transfers_from_node(this->routing_query_.from_))) {
      result.target_reachable = false;
    }

    return result;
  }

private:
  search_query const& routing_query_;
  search_dir const search_direction_;
  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_transfer_edges_;

  constant_graph_dijkstra<MAX_TRANSFERS, map_interchange_graph_node> transfers_;
};

}  // namespace motis::routing
