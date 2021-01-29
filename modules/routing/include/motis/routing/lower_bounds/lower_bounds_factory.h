#pragma once

#include "motis/routing/lower_bounds/implementations/lower_bounds.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_delegator.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_none.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_price.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_transfers.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_travel_time.h"
#include "motis/routing/lower_bounds/optimality/optimality_none.h"
#include "motis/routing/search_query.h"
namespace motis::routing {

template <typename Label, typename LowerBoundsTravelTime,
          typename LowerBoundsTransfers, typename LowerBoundsPrice>
std::unique_ptr<lower_bounds<Label>> combine_with_optimality(
    search_query const& q, search_dir dir, LowerBoundsTravelTime& lb_tt,
    LowerBoundsTransfers& lb_tr, LowerBoundsPrice& lb_pr) {

  if (q.optimality_type_ == optimality_type::CSA) {

    auto optimality_delegate = csa_optimality_calculator(dir);
    auto lb_delegate = std::unique_ptr<lower_bounds<Label>>(
        new lower_bounds_delegator<Label, LowerBoundsTravelTime,
                                   LowerBoundsTransfers, LowerBoundsPrice,
                                   csa_optimality_calculator>(
            q, dir, std::move(lb_tt), std::move(lb_tr), std::move(lb_pr),
            std::move(optimality_delegate)));

    return lb_delegate;
  } else {
    auto optimality_delegate = optimality_none();
    auto lb_delegate = std::unique_ptr<lower_bounds<Label>>(
        new lower_bounds_delegator<Label, LowerBoundsTravelTime,
                                   LowerBoundsTransfers, LowerBoundsPrice,
                                   optimality_none>(
            q, dir, std::move(lb_tt), std::move(lb_tr), std::move(lb_pr),
            std::move(optimality_delegate)));
    return lb_delegate;
  }
}

template <typename Label, typename LowerBoundsTravelTime,
          typename LowerBoundsTransfers>
std::unique_ptr<lower_bounds<Label>> combine_with_price(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    LowerBoundsTravelTime& lb_tt, LowerBoundsTransfers& lb_tr) {
  switch (q.lb_price_type_) {

    case lower_bounds_price_type::DISTANCE: {
      lower_bounds_price<Label> lbs_dist =
          lower_bounds_price<Label>(q, goal_ids);
      return combine_with_optimality<Label>(q, dir, lb_tt, lb_tr, lbs_dist);
    }
    case lower_bounds_price_type::NONE:
    default: {
      lower_bounds_none<Label> lbs_none = lower_bounds_none<Label>(q, dir);
      return combine_with_optimality<Label>(q, dir, lb_tt, lb_tr, lbs_none);
    }
  }
}

template <typename Label, typename LowerBoundsTravelTime>
std::unique_ptr<lower_bounds<Label>> combine_with_transfers(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    LowerBoundsTravelTime& lb_tt) {
  switch (q.lb_transfers_type_) {
    case lower_bounds_transfers_type::CG: {
      lower_bounds_transfers<Label> lbs_cg =
          lower_bounds_transfers<Label>(q, dir, goal_ids);
      return combine_with_price<Label>(q, goal_ids, dir, lb_tt, lbs_cg);
    }
    case lower_bounds_transfers_type::NONE:
    default: {
      lower_bounds_none<Label> lbs_none = lower_bounds_none<Label>(q, dir);
      return combine_with_price<Label>(q, goal_ids, dir, lb_tt, lbs_none);
    }
  }
}

template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir) {

  std::unique_ptr<lower_bounds<Label>> lbs;
  lower_bounds_result<Label> result{};
  switch (q.lb_travel_time_type_) {

    case lower_bounds_travel_time_type::CG: {
      lower_bounds_travel_time<Label> lbs_cg =
          lower_bounds_travel_time<Label>(q, dir, goal_ids);
      lbs = combine_with_transfers<Label>(q, goal_ids, dir, lbs_cg);
      break;
    }
    case lower_bounds_travel_time_type::NONE:
    default: {
      lower_bounds_none<Label> lbs_none = lower_bounds_none<Label>(q, dir);
      lbs = combine_with_transfers<Label>(q, goal_ids, dir, lbs_none);
      break;
    }
  }

  result = lbs->calculate();
  result.bounds_ = std::move(lbs);

  return result;
}


}  // namespace motis::routing
