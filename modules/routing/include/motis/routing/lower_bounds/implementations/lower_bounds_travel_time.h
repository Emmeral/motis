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
class lower_bounds_travel_time {

public:
  lower_bounds_travel_time(search_query const& routing_query,
                           search_dir const& search_direction,
                           const std::vector<int>& goals)
      : routing_query_(routing_query),
        search_direction_(search_direction),
        travel_time_(
            search_direction == search_dir::FWD
                ? this->routing_query_.sched_->travel_time_lower_bounds_fwd_
                : this->routing_query_.sched_->travel_time_lower_bounds_bwd_,
            goals, additional_time_edges_) {

    // construct the lower bounds graph for the additional edges
    for (auto const& e : this->routing_query_.query_edges_) {
      auto const from_node =
          (this->search_direction_ == search_dir::FWD) ? e.from_ : e.to_;
      auto const to_node =
          (this->search_direction_ == search_dir::FWD) ? e.to_ : e.from_;

      // station graph
      auto const from_station = from_node->get_station()->id_;
      auto const to_station = to_node->get_station()->id_;

      auto const ec = e.get_minimum_cost();

      additional_time_edges_[to_station].emplace_back(
          simple_edge{from_station, ec.time_});
    }
  }

  lower_bounds_travel_time(lower_bounds_travel_time&& other) noexcept
      : routing_query_(std::move(other.routing_query_)),
        search_direction_(std::move(other.search_direction_)),
        additional_time_edges_(std::move(other.additional_time_edges_)),
        travel_time_(std::move(other.travel_time_), additional_time_edges_) {}

  time_diff_t time_from_label(Label& l) const {
    return time_from_node(l.get_node());
  }
  time_diff_t time_from_node(node const* n) const { return travel_time_[n]; }
  bool is_valid_time_diff(time_diff_t time) const {
    return travel_time_.is_reachable(time);
  };

  lower_bounds_result<Label> calculate()  {

    auto result = lower_bounds_result<Label>{};

    MOTIS_START_TIMING(travel_time_timing);
    travel_time_.run();
    MOTIS_STOP_TIMING(travel_time_timing);

    result.travel_time_lb_ = MOTIS_TIMING_MS(travel_time_timing);

    // questionable if condition might be removed
    if (!is_valid_time_diff(time_from_node(this->routing_query_.from_))) {
      result.target_reachable = false;
    }
    return result;
  }

private:
  search_query const& routing_query_;
  search_dir const search_direction_;

  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_time_edges_;

  constant_graph_dijkstra<MAX_TRAVEL_TIME, map_station_graph_node> travel_time_;
};

}  // namespace motis::routing
