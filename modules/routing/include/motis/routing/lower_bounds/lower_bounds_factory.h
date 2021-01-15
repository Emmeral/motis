#pragma once

#include "motis/routing/lower_bounds/implementations/lower_bounds.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_const_graph.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_none.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_optimality_delegate.h"
#include "motis/routing/search_query.h"
namespace motis::routing {

template <typename Label, typename LowerBounds>
std::unique_ptr<lower_bounds<Label>> combine_with_optimality(
    LowerBounds& lb, search_query const& q, search_dir dir) {

  if (q.optimality_type_ == optimality_type::CSA) {

    auto optimality_delegate = csa_optimality_calculator(dir);
    auto lb_delegate = std::unique_ptr<lower_bounds<Label>>(
        new lower_bounds_optimality_delegate<Label, LowerBounds,
                                             csa_optimality_calculator>(
            q, dir, std::move(lb), std::move(optimality_delegate)));

    return lb_delegate;
  } else {
    auto lb_ptr =
        std::unique_ptr<lower_bounds<Label>>(new LowerBounds(std::move(lb)));
    return lb_ptr;
  }
}

template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    lower_bounds_type type) {

  std::unique_ptr<lower_bounds<Label>> lbs;
  lower_bounds_result<Label> result{};

  switch (type) {
    case lower_bounds_type::CG: {

      auto lbs_cg = lower_bounds_const_graph<Label>(q, dir, goal_ids);
      lbs = combine_with_optimality<Label, lower_bounds_const_graph<Label>>(
          lbs_cg, q, dir);
      break;
    }
    case lower_bounds_type::NONE: {
      auto lbs_none = lower_bounds_none<Label>(q, dir);
      lbs = combine_with_optimality<Label, lower_bounds_none<Label>>(lbs_none,
                                                                     q, dir);
    }
  }

  result = lbs->calculate();
  result.bounds_ = std::move(lbs);

  return result;
}

template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir) {
  return get_lower_bounds_for_query<Label>(q, goal_ids, dir, q.lb_type_);
}

}  // namespace motis::routing
