#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include <motis/csa/csa_search_shared.h>
#include <motis/csa/csa_to_journey.h>
#include <motis/csa/run_csa_search.h>
#include <motis/routing/label/criteria/transfers.h>
#include <motis/routing/label/criteria/travel_time.h>
#include "motis/routing/search_query.h"
#include "lower_bounds.h"

namespace motis::routing {
template <typename Label>
class lower_bounds_csa : public lower_bounds<Label> {

public:
  lower_bounds_csa(search_query const& routing_query, search_dir direction)
      : lower_bounds<Label>(routing_query, direction) {}

  time_diff_t time_from_label(Label& l) const override { return 0; }

  time_diff_t time_from_node(node const* n) const override { return 0; };
  bool is_valid_time_diff(time_diff_t diff) const override { return true; }

  time_diff_t transfers_from_label(Label& l) const override { return 0; }

  interchanges_t transfers_from_node(node const* n) const override { return 0; }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return true;
  }

  bool is_on_optimal_time_journey(Label& l) const override {
    return is_optimal(l);
  }
  bool is_on_optimal_transfers_journey(Label& l) const override {
    return is_optimal(l);
  }

  /**
   * @return true if the calculation was successful and the target could be
   * reached and false if the target is unreachable
   */
  lower_bounds_result<Label> calculate() override {

    MOTIS_START_TIMING(optimality_timing);
    bool reachable = calculate_optimality();
    MOTIS_STOP_TIMING(optimality_timing);

    auto r = lower_bounds_result<Label>();
    r.target_reachable = reachable;
    r.optimality_lb_ = MOTIS_TIMING_MS(optimality_timing);

    return r;
  }

  bool calculate_optimality(){
    const schedule* sched = this->routing_query_.sched_;

    // TODO throw error if csa would decline request
    // TODO: further think about on- vs pretrip
    motis::interval search_interval{this->routing_query_.interval_begin_,
                                    this->routing_query_.interval_end_};

    auto const& from_id = this->routing_query_.from_->id_;
    auto const& to_id = this->routing_query_.to_->id_;

    // TODO: Meta start/target not considered so far
    csa::csa_query initial_query(from_id, to_id, search_interval,
                                 this->search_direction_);

    const csa::csa_timetable* csaTimetable = this->routing_query_.csa_timetable;
    csa::response const response = motis::csa::run_csa_search(
        *sched, *csaTimetable, initial_query, SearchType::SearchType_Default,
        csa::implementation_type::CPU_SSE);

    for (auto const& csa_journey : response.journeys_) {

      journey j = csa::csa_to_journey(*sched, csa_journey);

      for (auto const& s : j.stops_) {
        process_single_stop(s);
      }

      handle_transitive_footpaths(j);
    }

    return !response.journeys_.empty();
  }

private:
  bool is_optimal(Label const& l) const {

    if (l.pred_ != nullptr && !l.pred_->is_on_optimal_journey()) {
      return false;
    }

    auto const node_id = l.get_node()->get_station()->id_;
    auto iterator = optimal_map_.find(node_id);
    if (iterator == optimal_map_.end()) {
      return false;
    }

    for (interval const& i : (*iterator).second) {
      if (i.contains(l.current_end())) {
        return true;
      }
    }
    return false;
  }

  struct interval {
    time start_{0};
    time end_{std::numeric_limits<time>::max()};

    bool contains(time t) const { return t >= start_ && t <= end_; }
  };

  void process_single_stop(journey::stop const& s) {
    const schedule* sched = this->routing_query_.sched_;

    // id in stations vector
    auto station = sched->eva_to_station_.at(s.eva_no_);
    auto id = station->index_;

    auto arrival_time_unix = s.arrival_.timestamp_;
    auto departure_time_unix = s.departure_.timestamp_;

    time departure_time;
    time arrival_time;

    if (arrival_time_unix == 0) {
      arrival_time = 0;
    } else {
      arrival_time =
          unix_to_motistime(sched->schedule_begin_, arrival_time_unix);
    }

    if (departure_time_unix == 0) {
      departure_time =
          arrival_time +
          station
              ->transfer_time_;  // TODO: does this work for backwards search?
    } else {
      departure_time =
          unix_to_motistime(sched->schedule_begin_, departure_time_unix);
    }

    interval inter = {arrival_time, departure_time};

    optimal_map_[id].push_back(inter);
  }
  /**
   * fix double-walk problem -> station not marked as optima because csa uses
   * transitive footpaths
   * @param j the journey the problem is fixed for
   */
  void handle_transitive_footpaths(journey const& j) {
    const schedule* sched = this->routing_query_.sched_;

    for (auto const& t : j.transports_) {
      if (!t.is_walk_) {
        continue;  // problem does only occur on footpaths
      }

      auto const& from_station =
          sched->eva_to_station_.at(j.stops_[t.from_].eva_no_);
      auto from_station_id = from_station->index_;

      auto const& to_station =
          sched->eva_to_station_.at(j.stops_[t.to_].eva_no_);
      auto const& to_station_id = to_station->index_;

      /** TODO: why does this optimization not work?
      for (auto const& fp : from_station->outgoing_footpaths_) {
        if (fp.to_station_ == to_station_id) {
          return;  // if the station is reached directly the fp was not
                   // transitive
        }
      }
      **/

      const interval last_inserted_optimum =
          optimal_map_[from_station_id].back();
      auto const& csa_from_station =
          this->routing_query_.csa_timetable->stations_[from_station_id];

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
