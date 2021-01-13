#pragma once

#include "motis/core/schedule/schedule.h"

#include "motis/csa/csa_timetable.h"

namespace motis::csa {

/**
 *
 * @param sched
 * @param bridge_zero_duration_connections
 * @param add_footpath_connections
 * @param ignore_interchange_restrictions ignores whether you can exit or enter
 * a train at a specific station when building the timetable. Useful for the
 * lower bounds calculation in the routing module.
 * @return
 */
std::unique_ptr<csa_timetable> build_csa_timetable(
    schedule const&, bool bridge_zero_duration_connections,
    bool add_footpath_connections,
    bool ignore_interchange_restrictions = false);

}  // namespace motis::csa
