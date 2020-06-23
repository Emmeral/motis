#pragma once

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
  // true if lower bounds should be calculated using csa
  bool csa_lower_bounds{false};
  // only set if csa_lower_bounds is true
  motis::csa::csa_timetable const* csa_timetable{nullptr};
};

}  // namespace motis::routing