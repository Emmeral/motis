#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include "motis/core/schedule/schedule.h"
#include "motis/routing/search_query.h"

namespace motis::routing {

struct interval {
  time start_{0};
  time end_{std::numeric_limits<time>::max()};
  bool route_allowed{true};
  bool station_allowed{false};
  bool foot_allowed{false};
  uint8_t connection_id_{0};

  bool contains(time t) const { return t >= start_ && t <= end_; }
};

class csa_optimality_calculator {
public:
  csa_optimality_calculator(search_dir search_direction)
      : search_direction_{search_direction} {}

  /**
   *
   * @param routing_query the original query for the routing request
   * @param search_direction the direction of the routing request
   * @return true if the target is reachable and the calculation was successful
   */
  bool calculate_optimality(search_query const& routing_query) {
    const schedule* sched = routing_query.sched_;

    // TODO throw error if csa would decline request
    // TODO: further think about on- vs pretrip
    motis::interval search_interval{routing_query.interval_begin_,
                                    routing_query.interval_end_};

    auto const& from_id = routing_query.from_->id_;
    auto const& to_id = routing_query.to_->id_;

    // TODO: Meta start/target not considered so far
    csa::csa_query initial_query(from_id, to_id, search_interval,
                                 search_direction_);

    const csa::csa_timetable* csa_tt = routing_query.csa_timetable;

    csa::implementation_type impl_type = csa::implementation_type::CPU_SSE;
#ifdef MOTIS_CUDA
    impl_type = csa::implementation_type::GPU;
#endif

    csa::response const response = motis::csa::run_csa_search(
        *sched, *csa_tt, initial_query, SearchType::SearchType_Default,
        impl_type);

    optimal_journey_count_ = response.journeys_.size();
    uint8_t conn_id{0};
    for (auto const& csa_journey : response.journeys_) {
      journey j = csa::csa_to_journey(*sched, csa_journey);

      for (auto& s : j.stops_) {
        process_single_stop(s, *sched, conn_id);
      }

      handle_transitive_footpaths(j, *sched, *csa_tt, conn_id);
      ++conn_id;
    }

    return !response.journeys_.empty();
  }

  template <typename Label>
  bool is_optimal(Label const& l) const {

    if (l.pred_ != nullptr && !l.pred_->is_on_optimal_journey()) {
      return false;
    }

    const node* node = l.get_node();
    auto const node_id = node->get_station()->id_;
    auto iterator = optimal_map_.find(node_id);
    if (iterator == optimal_map_.end()) {
      return false;
    }

    const bool is_route_node = node->is_route_node();
    const bool is_station_node = node->is_station_node();
    const bool is_foot_node = node->is_foot_node();
    for (interval const& i : (*iterator).second) {
      // don't make station nodes optimal, if there was no transfer in the
      // interval.
      if (is_station_node && !i.station_allowed) {
        continue;
      }
      if (is_foot_node && !i.foot_allowed) {
        continue;
      }
      if (is_route_node && !i.route_allowed) {
        continue;
      }

      // if the interval matches the given time
      if (i.contains(l.current_end())) {

        // "Jumping" between mutliple optimal journeys produces an
        // not optimal journey which is still classified as optimal
        // If we are leaving the station we are only optimal if we the
        // destination is on an optimal journey and more importantly on the same
        // optimal journey as we are currently
        if (is_route_node &&
            l.edge_->type() != edge::ROUTE_EDGE) {  // -> on leaving route node
          bool opt_leave = check_optimal_station_leave<Label>(l, node, i);
          if (!opt_leave) {
            continue;
          }
          // Also to prevent Jumping: The interval must have started when we
          // entered the station. Otherwise the interval does not belong to our
          // journey
        } else if (l.pred_ != nullptr && l.pred_->get_node()->is_route_node() &&
                   l.edge_->type() != edge::ROUTE_EDGE &&
                   l.pred_->is_on_optimal_journey()) {  // some node which is
                                                        // not an
          // entering route node
          if (i.start_ != l.pred_->current_end()) {
            continue;
          }
        }
        return true;
      }
    }
    return false;
  }


  int get_optimal_journey_count() const{
    return optimal_journey_count_;
  }

private:
  /**
   *
   * @tparam Label
   * @param l the current processed label.
   * @param node the labels node. Shall be a route node leaving the station
   * @param current_con_id the id of the current optimal connection
   * @return whether the leaving route node is optimal
   */
  template <typename Label>
  bool check_optimal_station_leave(Label const& l, node const* node,
                                   interval current_interval) const {

    const uint8_t current_con_id = current_interval.connection_id_;

    if (search_direction_ == search_dir::FWD) {
      // check for all route edges whether the next connection arrives on the
      // same connection at a valid time
      for (auto const& e : node->edges_) {
        if (e.type() == edge::ROUTE_EDGE) {
          light_connection const* conn =
              e.template get_connection<search_dir::FWD>(l.current_end());
          if (conn == nullptr) {
            continue;
          }

          // check if departure matches
          if (conn->d_time_ != current_interval.end_) {
            continue;
          }

          auto dest_id =
              e.get_destination(search_direction_)->get_station()->id_;
          auto dest_iter = optimal_map_.find(dest_id);
          if (dest_iter == optimal_map_.end()) {
            continue;
          }
          std::vector<interval> dest_intervals = dest_iter->second;

          for (interval const& inter : dest_intervals) {
            if (conn->a_time_ == inter.start_ &&
                inter.connection_id_ == current_con_id) {
              return true;
            }
          }
        }
      }
    } else {
      for (auto const& e : node->incoming_edges_) {
        if (e->type() == edge::ROUTE_EDGE) {
          light_connection const* conn =
              e->template get_connection<search_dir::FWD>(l.current_end());
          if (conn == nullptr) {
            continue;
          }

          // check if departure matches
          if (conn->a_time_ != current_interval.start_) {
            continue;
          }

          auto dest_id =
              e->get_destination(search_direction_)->get_station()->id_;
          auto dest_iter = optimal_map_.find(dest_id);
          if (dest_iter == optimal_map_.end()) {
            continue;
          }
          std::vector<interval> dest_intervals = dest_iter->second;

          for (interval const& inter : dest_intervals) {
            if (conn->d_time_ == inter.end_ &&
                inter.connection_id_ == current_con_id) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  void process_single_stop(journey::stop const& s, schedule const& sched,
                           uint8_t connection_id) {

    // id in stations vector
    auto station = sched.eva_to_station_.at(s.eva_no_);
    auto id = station->index_;

    auto arrival_time_unix = s.arrival_.timestamp_;
    auto departure_time_unix = s.departure_.timestamp_;

    time departure_time;
    time arrival_time;

    bool is_start{false};
    bool is_end{false};

    if (arrival_time_unix == 0) {
      arrival_time = 0;
      is_start = true;
    } else {
      arrival_time =
          unix_to_motistime(sched.schedule_begin_, arrival_time_unix);
    }

    if (departure_time_unix == 0) {
      is_end = true;  // if there is no departure it is the last station
      // TODO: does this work for backwards search?
      departure_time = arrival_time + station->transfer_time_;
    } else {
      departure_time =
          unix_to_motistime(sched.schedule_begin_, departure_time_unix);
    }
    // because you only need to go to the station node if your are done or want
    // to enter the next train
    bool station_allowed = (s.enter_) || is_start || is_end;
    // because you only go to a foot node if you exit a train and don't enter
    // another at the same station. Except for the target. You won't want to
    // walk away from here again.
    bool foot_allowed =
        (s.exit_ && !s.enter_ && !is_end) || (is_start && !s.enter_);
    // don't allow route nodes at the start if the journey starts by foot
    bool route_allowed = !is_start || s.enter_;
    interval inter = {arrival_time,    departure_time, route_allowed,
                      station_allowed, foot_allowed,   connection_id};

    optimal_map_[id].push_back(inter);
  }
  /**
   * fix double-walk problem -> station not marked as optima because csa uses
   * transitive footpaths
   * @param j the journey the problem is fixed for
   */
  void handle_transitive_footpaths(journey const& j, schedule const& sched,
                                   csa::csa_timetable const& csa_tt,
                                   uint8_t connection_id) {

    for (auto const& t : j.transports_) {
      if (!t.is_walk_) {
        continue;  // problem does only occur at footpaths
      }

      auto const& from_station =
          sched.eva_to_station_.at(j.stops_[t.from_].eva_no_);
      auto from_station_id = from_station->index_;

      auto const& to_station =
          sched.eva_to_station_.at(j.stops_[t.to_].eva_no_);
      auto const& to_station_id = to_station->index_;

      std::vector<interval> optima_at_station = optimal_map_[from_station_id];

      // sometimes the same station is visited twice by the optimal journey but
      // only once left by foot
      auto it = std::find_if(
          optima_at_station.rbegin(), optima_at_station.rend(),
          [=](interval const& inter) {
            return inter.foot_allowed && inter.connection_id_ == connection_id;
          });
      assert(it != optima_at_station.rend());

      const interval last_inserted_optimum = *it;

      auto const& csa_from_station = csa_tt.stations_[from_station_id];
      auto const& csa_out_fps = csa_from_station.footpaths_;

      auto const& csa_to_station = csa_tt.stations_[to_station_id];

      footpath const& to_stat_fp =
          *std::find_if(csa_out_fps.begin(), csa_out_fps.end(),
                        [to_station_id](auto const fp) {
                          return fp.to_station_ == to_station_id;
                        });

      // make stations in the local footpath cluster optimal
      for (footpath const& fp : csa_out_fps) {

        if (fp.to_station_ == to_station_id ||
            fp.to_station_ == from_station_id) {
          continue;  // skip already inserted or loops
        }

        // skip if further away than  to_station
        if (fp.duration_ > to_stat_fp.duration_) {
          continue;
        }

        footpath const& out_fp = *std::find_if(
            csa_to_station.incoming_footpaths_.begin(),
            csa_to_station.incoming_footpaths_.end(), [fp](auto const out_fp) {
              return out_fp.from_station_ == fp.to_station_;
            });

        // skip if footpaths do not sum up to the total trans. fp.
        if(fp.duration_ + out_fp.duration_ != to_stat_fp.duration_){
          continue;
        }

        time time_at_new_station = last_inserted_optimum.end_ + fp.duration_;
        interval new_optimum = {
            time_at_new_station, time_at_new_station, false, true, true,
            connection_id};
        optimal_map_[fp.to_station_].push_back(new_optimum);
      }
    }
  }

  int optimal_journey_count_{0};
  const search_dir search_direction_{search_dir::FWD};
  std::map<uint32_t, std::vector<interval>> optimal_map_;
};
}  // namespace motis::routing
