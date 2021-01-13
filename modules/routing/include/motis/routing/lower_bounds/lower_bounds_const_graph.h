#pragma once

#include "motis/hash_map.h"
#include "motis/routing/lower_bounds/lower_bounds.h"

#include "motis/core/common/timing.h"
#include "motis/core/schedule/constant_graph.h"
#include "motis/core/schedule/schedule.h"

#include "motis/routing/label/criteria/transfers.h"
#include "motis/routing/label/criteria/travel_time.h"

namespace motis::routing {

class lower_bounds_const_graph : public lower_bounds {

public:
  lower_bounds_const_graph(search_query const& routing_query,
                           search_dir const& search_direction,  //
                           std::vector<int> const& goals);
  time_diff_t time_from_node(node const* n) const override;
  bool is_valid_time_diff(time_diff_t time) const override;
  interchanges_t transfers_from_node(node const* n) const override;
  bool is_valid_transfer_amount(interchanges_t amount) const override;

  void calculate_timing();
  void calculate_transfers();

private:
  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_time_edges_;
  mcd::hash_map<unsigned, std::vector<simple_edge>> additional_transfer_edges_;

  constant_graph_dijkstra<MAX_TRAVEL_TIME, map_station_graph_node> travel_time_;
  constant_graph_dijkstra<MAX_TRANSFERS, map_interchange_graph_node> transfers_;
};

}  // namespace motis::routing
