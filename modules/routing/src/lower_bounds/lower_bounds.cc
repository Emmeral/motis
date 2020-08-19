#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/lower_bounds/lower_bounds_const_graph.h"
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include "motis/routing/lower_bounds/lower_bounds_mixed.h"
#include "motis/routing/lower_bounds/lower_bounds_none.h"

namespace motis::routing {

lower_bounds::lower_bounds(const search_query& routing_query,
                           search_dir search_direction)
    : routing_query_(routing_query), search_direction_(search_direction) {}

lower_bounds_stats lower_bounds::get_stats() const {

  schedule const& sched = *routing_query_.sched_;

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
      auto lbs_csa = std::make_unique<lower_bounds_csa>(q, dir);

      MOTIS_START_TIMING(total_lb_timing);
      auto success = lbs_csa->calculate();
      MOTIS_STOP_TIMING(total_lb_timing);

      result.target_reachable = success;
      result.total_lb = MOTIS_TIMING_MS(total_lb_timing);
      result.bounds_ = std::move(lbs_csa);
      break;
    }
    case lower_bounds_type::CG: {

      auto lbs_cg =
          std::make_unique<lower_bounds_const_graph>(q, dir, goal_ids);

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
    case lower_bounds_type::MIXED: {
      auto lbs_mixed = std::make_unique<lower_bounds_mixed>(q, dir, goal_ids);

      MOTIS_START_TIMING(travel_time_lb_timing);
      auto success = lbs_mixed->calculate_timing();
      MOTIS_STOP_TIMING(travel_time_lb_timing);

      result.travel_time_lb_ = MOTIS_TIMING_MS(travel_time_lb_timing);

      // questionable if condition might be removed
      if (!success) {
        result.target_reachable = false;
        return result;
      }

      MOTIS_START_TIMING(transfers_lb_timing);
      lbs_mixed->calculate_transfers();
      MOTIS_STOP_TIMING(transfers_lb_timing);

      result.transfers_lb_ = MOTIS_TIMING_MS(transfers_lb_timing);
      result.total_lb = result.transfers_lb_ + result.travel_time_lb_;

      result.bounds_ = std::move(lbs_mixed);
      break;
    }
    case lower_bounds_type::NONE: {
      auto lbs_none = std::make_unique<lower_bounds_none>(q, dir);
      result.bounds_ = std::move(lbs_none);
    }
  }

  return result;
}

}  // namespace motis::routing
