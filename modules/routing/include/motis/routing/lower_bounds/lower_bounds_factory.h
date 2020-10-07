#pragma once


#include "motis/routing/lower_bounds/implementations/lower_bounds.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_const_graph.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_csa.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_none.h"
#include "motis/routing/search_query.h"
namespace motis::routing {


template <typename Label>
lower_bounds_result<Label> get_lower_bounds_for_query(
    search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
    lower_bounds_type type) {

  std::unique_ptr<lower_bounds<Label>> lbs;
  lower_bounds_result<Label> result{};

  switch (type) {
    case lower_bounds_type::CSA: {
      auto lbs_csa = std::make_unique<lower_bounds_csa<Label>>(q, dir);

      result = lbs_csa->calculate();

      result.bounds_ = std::move(lbs_csa);
      break;
    }
    case lower_bounds_type::CG: {

      auto lbs_cg =
          std::make_unique<lower_bounds_const_graph<Label>>(q, dir, goal_ids);
      result =  lbs_cg->calculate();
      result.bounds_ = std::move(lbs_cg);
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
