#pragma once

#include "motis/routing/lower_bounds/lower_bounds_type.h"
#include "motis/core/common/timing.h"
#include "motis/core/schedule/schedule.h"

#include "motis/csa/csa_timetable.h"
#include "motis/routing/mem_manager.h"

namespace motis::routing {

struct search_query {
  schedule const* sched_{nullptr};
  mem_manager* mem_{nullptr};
  node const* from_{nullptr};
  station_node const* to_{nullptr};
  time interval_begin_{0};
  time interval_end_{0};
  bool extend_interval_earlier_{false};
  bool extend_interval_later_{false};
  std::vector<edge> query_edges_;
  unsigned min_journey_count_{0};
  bool use_start_metas_{false};
  bool use_dest_metas_{false};
  bool use_start_footpaths_{false};
  light_connection const* lcon_{nullptr};
  lower_bounds_travel_time_type lb_travel_time_type_{lower_bounds_travel_time_type::CG};
  lower_bounds_transfers_type lb_transfers_type_{lower_bounds_transfers_type::CG};
  lower_bounds_price_type lb_price_type_{lower_bounds_price_type::NONE};
  optimality_type optimality_type_{optimality_type::NONE};
  // only set if lower_bounds_type::CSA is used
  motis::csa::csa_timetable const* csa_timetable{nullptr};
  // true if the response shall contain extended stats about the lower bounds
  bool extended_lb_stats_{false};
};

}  // namespace motis::routing
