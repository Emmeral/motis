#pragma once

#include <cstdint>
#include "motis/core/schedule/nodes.h"
#include "motis/core/schedule/schedule.h"
#include "motis/routing/lower_bounds/lower_bounds_stats.h"
#include "motis/routing/search_query.h"

namespace motis::routing {

class lower_bounds;

struct lower_bounds_result {
  std::unique_ptr<lower_bounds> bounds_;

  bool target_reachable{true};
  uint64_t travel_time_lb_{};
  uint64_t transfers_lb_{};
  uint64_t total_lb{};
};

class lower_bounds {
public:
  using time_diff_t = uint32_t;
  using interchanges_t = uint32_t;

  virtual time_diff_t time_from_node(node const* n) const = 0;
  virtual bool is_valid_time_diff(time_diff_t diff) const = 0;

  virtual interchanges_t transfers_from_node(node const* n) const = 0;
  virtual bool is_valid_transfer_amount(interchanges_t amount) const = 0;

  lower_bounds_stats get_stats(const schedule& sched) const;

  static lower_bounds_result get_lower_bounds_for_query(
      search_query const& q, std::vector<int> const& goal_ids, search_dir dir,
      lower_bounds_type type);

  static lower_bounds_result get_lower_bounds_for_query(
      search_query const& q, std::vector<int> const& goal_ids, search_dir dir){
    return get_lower_bounds_for_query(q, goal_ids, dir, q.lb_type);
  }

  virtual ~lower_bounds() = default;
};



}  // namespace motis::routing
