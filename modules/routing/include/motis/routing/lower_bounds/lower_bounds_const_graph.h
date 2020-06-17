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
  lower_bounds_const_graph(
      schedule const& sched,  //
      constant_graph const& travel_time_graph,
      constant_graph const& transfers_graph,  //
      std::vector<int> const& goals,
      mcd::hash_map<unsigned, std::vector<simple_edge>> const&
          additional_travel_time_edges,
      mcd::hash_map<unsigned, std::vector<simple_edge>> const&
          additional_transfers_edges);
  time_diff_t time_from_node(node const* n) override;
  bool is_valid_time_diff(time_diff_t time) override;
  interchanges_t transfers_from_node(node const* n) override;
  bool is_valid_transfer_amount(interchanges_t amount) override;

  void calculate_timing();
  void calculate_transfers();

private:
  constant_graph_dijkstra<MAX_TRAVEL_TIME, map_station_graph_node> travel_time_;
  constant_graph_dijkstra<MAX_TRANSFERS, map_interchange_graph_node> transfers_;

};

}  // namespace motis::routing
