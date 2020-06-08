#pragma once

#include <vector>

#include "motis/core/schedule/interval.h"
#include "motis/core/schedule/schedule.h"
#include "motis/csa/csa_timetable.h"

#include "motis/protocol/RoutingRequest_generated.h"

namespace motis::csa {

struct csa_query {
  csa_query(schedule const&, motis::routing::RoutingRequest const*);
  /**
   * Utility constructor for a point to point query
   */
  csa_query(station_id start, station_id dest, interval search_interval,
            search_dir dir)
      : csa_query(std::vector<station_id>({start}),
                  std::vector<station_id>({dest}), search_interval, dir){}
  /**
   * Constructor which directly includes the vector of meta stations
   */
  csa_query(std::vector<station_id> start_metas,
            std::vector<station_id> dest_metas, interval search_interval,
            search_dir dir);

  bool is_ontrip() const { return search_interval_.end_ == INVALID_TIME; }

  std::vector<station_id> meta_starts_, meta_dests_;
  interval search_interval_;
  unsigned min_connection_count_{0U};
  bool extend_interval_earlier_{false}, extend_interval_later_{false};
  search_dir dir_{search_dir::FWD};
};

}  // namespace motis::csa