#include "motis/routing/lower_bounds/lower_bounds.h"
#include <motis/routing/lower_bounds/lower_bounds_const_graph.h>
#include <motis/routing/lower_bounds/lower_bounds_csa.h>
#include <motis/routing/lower_bounds/lower_bounds_none.h>

namespace motis::routing {

lower_bounds_stats lower_bounds::get_stats(
    const schedule& sched) const {

  auto& stations_nodes = sched.station_nodes_;
  uint64_t sum_times{0L}, sum_transfers{0L};
  uint32_t invalid_time_count{0}, invalid_transfers_count{0};
  uint32_t valid_time_count{0}, valid_transfers_count{0};

  auto process_node = [&, this](const node* n) {
    auto time = this->time_from_node(n);
    auto transfers = this->transfers_from_node(n);
    if (this->is_valid_time_diff(time)) {
      sum_times += time;
      ++valid_time_count;
    } else {
      ++invalid_time_count;
    }

    if (this->is_valid_transfer_amount(transfers)) {
      sum_transfers += transfers;
      ++valid_transfers_count;
    } else {
      ++invalid_transfers_count;
    }
  };

  for (auto& n : stations_nodes) {
    process_node(n.get());  // only process stations as route nodes may alter
                            // the statistics
  }

  double avg_time = static_cast<double>(sum_times) / valid_time_count;
  double avg_transfers =
      static_cast<double>(sum_transfers) / valid_transfers_count;

  return lower_bounds_stats{avg_time, avg_transfers, invalid_time_count,
                            invalid_transfers_count};
}

/**
   * Constructs an lower_bounds object for the specified search query
   * @param q the query.
   * @param goal_ids ids of the stations which are a goal of the search query
   * @param dir The search direction of the query
   * @return the lower_bounds object
 */
lower_bounds_result lower_bounds::get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    lower_bounds_type type) {

  std::unique_ptr<lower_bounds> lbs;
  lower_bounds_result result{};


  switch (type) {
    case lower_bounds_type::CSA: {
      auto lbs_csa = std::make_unique<lower_bounds_csa>(q, dir, goal_ids);

      MOTIS_START_TIMING(total_lb_timing);
      auto success = lbs_csa->calculate();
      MOTIS_STOP_TIMING(total_lb_timing);

      result.target_reachable = success;
      result.total_lb = MOTIS_TIMING_MS(total_lb_timing);
      result.bounds_ = std::move(lbs_csa);
      break;
    }
    case lower_bounds_type::CG: {

      auto const route_offset = q.sched_->station_nodes_.size();
      mcd::hash_map<unsigned, std::vector<simple_edge>>
          travel_time_lb_graph_edges;
      mcd::hash_map<unsigned, std::vector<simple_edge>>
          transfers_lb_graph_edges;

      // construct the lower bounds graph for the additional edges
      for (auto const& e : q.query_edges_) {
        auto const from_node = (dir == search_dir::FWD) ? e.from_ : e.to_;
        auto const to_node = (dir == search_dir::FWD) ? e.to_ : e.from_;

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

        travel_time_lb_graph_edges[to_station].emplace_back(
            simple_edge{from_station, ec.time_});
        transfers_lb_graph_edges[to_interchange].emplace_back(simple_edge{
            from_interchange, static_cast<uint16_t>(ec.transfer_ ? 1 : 0)});
      }

      auto lbs_cg = std::make_unique<lower_bounds_const_graph>(
          *q.sched_,  //
          dir == search_dir::FWD ? q.sched_->travel_time_lower_bounds_fwd_
                                 : q.sched_->travel_time_lower_bounds_bwd_,
          dir == search_dir::FWD ? q.sched_->transfers_lower_bounds_fwd_
                                 : q.sched_->transfers_lower_bounds_bwd_,
          goal_ids, travel_time_lb_graph_edges, transfers_lb_graph_edges);

      MOTIS_START_TIMING(travel_time_lb_timing);
      lbs_cg->calculate_timing();
      MOTIS_STOP_TIMING(travel_time_lb_timing);

      result.travel_time_lb_ = MOTIS_TIMING_MS(travel_time_lb_timing);

      // questionable if condition might be removed
      if (!lbs_cg->is_valid_time_diff(lbs_cg->time_from_node(q.from_))) {
        result.target_reachable = false;
        return result;
      }

      MOTIS_START_TIMING(transfers_lb_timing);
      lbs_cg->calculate_transfers();
      MOTIS_STOP_TIMING(transfers_lb_timing);

      result.transfers_lb_ = MOTIS_TIMING_MS(transfers_lb_timing);
      result.total_lb = result.transfers_lb_ + result.travel_time_lb_;

      result.bounds_ = std::move(lbs_cg);
      break;
    }
    case lower_bounds_type::NONE: {
      auto lbs_none = std::make_unique<lower_bounds_none>();
      result.bounds_ = std::move(lbs_none);
    }
  }

  return result;
}

}
