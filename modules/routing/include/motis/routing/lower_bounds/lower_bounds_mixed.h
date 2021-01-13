#pragma once

#include "motis/hash_map.h"
#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/lower_bounds/lower_bounds_const_graph.h"
#include "motis/routing/lower_bounds/lower_bounds_csa.h"

#include "motis/core/common/timing.h"
#include "motis/core/schedule/constant_graph.h"
#include "motis/core/schedule/schedule.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {

/**
 * Lower Bounds implementation which uses cg for transfers and csa for travel
 * time
 */
class lower_bounds_mixed : public lower_bounds {

public:
  lower_bounds_mixed(search_query const& routing_query,
                     search_dir const& search_direction,  //
                     std::vector<int> const& goals);
  time_diff_t time_from_node(node const* n) const override;
  bool is_valid_time_diff(time_diff_t time) const override;
  interchanges_t transfers_from_node(node const* n) const override;
  bool is_valid_transfer_amount(interchanges_t amount) const override;

  bool calculate_timing();
  void calculate_transfers();

private:
  lower_bounds_csa csa_lower_bounds_;
  lower_bounds_const_graph cg_lower_bounds;
};

}  // namespace motis::routing
