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
template <typename Label>
class lower_bounds_mixed : public lower_bounds<Label> {

public:
  lower_bounds_mixed(search_query const& routing_query,
                     search_dir const& search_direction,
                     const std::vector<int>& goals)
      : lower_bounds<Label>(routing_query, search_direction),
        csa_lower_bounds_(routing_query, search_direction),
        cg_lower_bounds(routing_query, search_direction, goals) {}


  time_diff_t time_from_label(Label& l) const override{
    auto const csa = csa_lower_bounds_.time_from_label(l);
    return csa;
  }

  time_diff_t time_from_node(node const* n) const override {
    auto const csa = csa_lower_bounds_.time_from_node(n);
    return csa;
  }
  bool is_valid_time_diff(time_diff_t time) const override {
    return csa_lower_bounds_.is_valid_time_diff(time);
  }

  interchanges_t transfers_from_label(Label& l) const override{
    auto const csa = csa_lower_bounds_.transfers_from_label(l);
    auto const cg = cg_lower_bounds.transfers_from_label(l);
    return std::max(csa, cg);
  }

  interchanges_t transfers_from_node(node const* n) const override {
    auto const csa = csa_lower_bounds_.transfers_from_node(n);
    auto const cg = cg_lower_bounds.transfers_from_node(n);
    return std::max(csa, cg);
  }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return cg_lower_bounds.is_valid_transfer_amount(amount);
  }

  bool calculate_timing(){
    return csa_lower_bounds_.calculate();
  }
  void calculate_transfers(){
    cg_lower_bounds.calculate_transfers();
  }

private:
  lower_bounds_csa<Label> csa_lower_bounds_;
  lower_bounds_const_graph<Label> cg_lower_bounds;
};

}  // namespace motis::routing
