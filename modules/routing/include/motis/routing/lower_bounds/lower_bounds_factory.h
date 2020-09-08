#pragma once


#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/lower_bounds/lower_bounds_const_graph.h"
#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include "motis/routing/lower_bounds/lower_bounds_mixed.h"
#include "motis/routing/lower_bounds/lower_bounds_none.h"
#include "motis/routing/search_query.h"
namespace motis::routing {


template <typename Label>
struct lower_bounds_result {
  std::unique_ptr<lower_bounds<Label>> bounds_;
  bool target_reachable{true};
  uint64_t travel_time_lb_{};
  uint64_t transfers_lb_{};
  uint64_t total_lb{};
};

template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    lower_bounds_type type) {

  std::unique_ptr<lower_bounds<Label>> lbs;
  lower_bounds_result<Label> result{};

  switch (type) {
    case lower_bounds_type::CSA: {
      auto lbs_csa = std::make_unique<lower_bounds_csa<Label>>(q, dir);

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
          std::make_unique<lower_bounds_const_graph<Label>>(q, dir, goal_ids);

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
      auto lbs_mixed =
          std::make_unique<lower_bounds_mixed<Label>>(q, dir, goal_ids);

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
      auto lbs_none = std::make_unique<lower_bounds_none<Label>>(q, dir);
      result.bounds_ = std::move(lbs_none);
    }
  }

  return result;
}
template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir) {
  return get_lower_bounds_for_query<Label>(q, goal_ids, dir, q.lb_type);
}

}  // namespace motis::routing
