#pragma once

#include <motis/core/schedule/nodes.h>
#include <cstdint>

namespace motis::routing {

class lower_bounds {
public:
  using time_diff_t = uint32_t;
  using interchanges_t = uint32_t;

  virtual time_diff_t time_from_node(node const* n) = 0;
  virtual bool is_valid_time_diff(time_diff_t diff) = 0;

  virtual interchanges_t transfers_from_node(node const* n) = 0;
  virtual bool is_valid_transfer_amount(interchanges_t amount) = 0;

  virtual ~lower_bounds() = default;
};

}  // namespace motis::routing