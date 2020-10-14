#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include "motis/core/schedule/schedule.h"
#include "motis/routing/search_query.h"

namespace motis::routing {

struct interval {
  time start_{0};
  time end_{std::numeric_limits<time>::max()};
  bool interchange_{false};

  bool contains(time t) const { return t >= start_ && t <= end_; }
};

class csa_optimality_calculator {
public:
  /**
   *
   * @param routing_query the original query for the routing request
   * @param search_direction the direction of the routing request
   * @return true if the target is reachable and the calculation was successful
   */
  bool calculate_optimality(search_query const& routing_query,
                            search_dir search_direction) {
    const schedule* sched = routing_query.sched_;

    // TODO throw error if csa would decline request
    // TODO: further think about on- vs pretrip
    motis::interval search_interval{routing_query.interval_begin_,
                                    routing_query.interval_end_};

    auto const& from_id = routing_query.from_->id_;
    auto const& to_id = routing_query.to_->id_;

    // TODO: Meta start/target not considered so far
    csa::csa_query initial_query(from_id, to_id, search_interval,
                                 search_direction);

    const csa::csa_timetable* csa_tt = routing_query.csa_timetable;
    csa::response const response = motis::csa::run_csa_search(
        *sched, *csa_tt, initial_query, SearchType::SearchType_Default,
        csa::implementation_type::CPU_SSE);

    for (auto const& csa_journey : response.journeys_) {

      journey j = csa::csa_to_journey(*sched, csa_journey);

      for (auto const& s : j.stops_) {
        process_single_stop(s, *sched);
      }

      handle_transitive_footpaths(j, *sched, *csa_tt);
    }

    return !response.journeys_.empty();
  }

  template <typename Label>
  bool is_optimal(Label const& l) const {

    if (l.pred_ != nullptr && !l.pred_->is_on_optimal_journey()) {
      return false;
    }

    auto const node_id = l.get_node()->get_station()->id_;
    auto iterator = optimal_map_.find(node_id);
    if (iterator == optimal_map_.end()) {
      return false;
    }

    const bool is_station = l.get_node()->is_station_node();
    for (interval const& i : (*iterator).second) {
      // don't make station nodes optimal, if there was no transfer in the
      // interval.
      if (!i.interchange_ && is_station) {
        continue;
      }
      if (i.contains(l.current_end())) {
        return true;
      }
    }
    return false;
  }

private:
  void process_single_stop(journey::stop const& s, schedule const& sched) {

    // id in stations vector
    auto station = sched.eva_to_station_.at(s.eva_no_);
    auto id = station->index_;

    auto arrival_time_unix = s.arrival_.timestamp_;
    auto departure_time_unix = s.departure_.timestamp_;

    time departure_time;
    time arrival_time;

    if (arrival_time_unix == 0) {
      arrival_time = 0;
    } else {
      arrival_time =
          unix_to_motistime(sched.schedule_begin_, arrival_time_unix);
    }

    if (departure_time_unix == 0) {
      // TODO: does this work for backwards search?
      departure_time = arrival_time + station->transfer_time_;
    } else {
      departure_time =
          unix_to_motistime(sched.schedule_begin_, departure_time_unix);
    }

    // TODO: is this only a approximation
    bool is_interchange{false};
    if (departure_time - arrival_time > station->transfer_time_) {
      is_interchange = true;  // mark whether there was an interchange at a stop
    }
    interval inter = {arrival_time, departure_time, is_interchange};

    optimal_map_[id].push_back(inter);
  }
  /**
   * fix double-walk problem -> station not marked as optima because csa uses
   * transitive footpaths
   * @param j the journey the problem is fixed for
   */
  void handle_transitive_footpaths(journey const& j, schedule const& sched,
                                   csa::csa_timetable const& csa_tt) {

    for (auto const& t : j.transports_) {
      if (!t.is_walk_) {
        continue;  // problem does only occur on footpaths
      }

      auto const& from_station =
          sched.eva_to_station_.at(j.stops_[t.from_].eva_no_);
      auto from_station_id = from_station->index_;

      auto const& to_station =
          sched.eva_to_station_.at(j.stops_[t.to_].eva_no_);
      auto const& to_station_id = to_station->index_;

      /** TODO: why does this optimization not work?
      for (auto const& fp : from_station->outgoing_footpaths_) {
        if (fp.to_station_ == to_station_id) {
          return;  // if the station is reached directly the fp was not
                   // transitive
        }
      }
      **/

      // TODO: there might be edge cases where there are multiple optima
      // inserted for a single journey
      const interval last_inserted_optimum =
          optimal_map_[from_station_id].back();
      auto const& csa_from_station = csa_tt.stations_[from_station_id];

      // make all stations in the local footpath cluster optimal
      for (auto const& fp : csa_from_station.footpaths_) {

        if (fp.to_station_ == to_station_id ||
            fp.to_station_ == from_station_id) {
          continue;  // skip already inserted or loops
        }
        time time_at_new_station = last_inserted_optimum.end_ + fp.duration_;
        interval new_optimum = {time_at_new_station, time_at_new_station};
        optimal_map_[fp.to_station_].push_back(new_optimum);
      }
    }
  }

  std::map<uint32_t, std::vector<interval>> optimal_map_;
};
}  // namespace motis::routing
