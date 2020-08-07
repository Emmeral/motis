#pragma once

#include <cstdint>
#include "motis/core/schedule/nodes.h"
#include "motis/core/schedule/schedule.h"
#include "motis/routing/lower_bounds/lower_bounds_stats.h"

namespace motis::routing {

class lower_bounds {
public:
  using time_diff_t = uint32_t;
  using interchanges_t = uint32_t;

  virtual time_diff_t time_from_node(node const* n) const = 0;
  virtual bool is_valid_time_diff(time_diff_t diff) const = 0;

  virtual interchanges_t transfers_from_node(node const* n) const = 0;
  virtual bool is_valid_transfer_amount(interchanges_t amount) const = 0;

  lower_bounds_stats get_stats(const schedule& sched) const;

  virtual ~lower_bounds() = default;
};

}  // namespace motis::routing
