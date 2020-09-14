#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include <motis/csa/csa_search_shared.h>
#include <motis/routing/label/criteria/transfers.h>
#include <motis/routing/label/criteria/travel_time.h>
#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/search_query.h"

namespace motis::routing {
template <typename Label>
class lower_bounds_csa : public lower_bounds<Label> {

public:
  lower_bounds_csa(search_query const& routing_query, search_dir direction)
      : lower_bounds<Label>(routing_query, direction),
        bounds_(routing_query.sched_->stations_.size()) {

    // it is tricky thinking about forward and backwards search. Also CSA
    // returns 0 as invalid if searching backwards the variables are names as
    // if the search direction was forward
    invalid_time_ = this->search_direction_ == search_dir::FWD
                        ? std::numeric_limits<time>::max()
                        : std::numeric_limits<time>::min();
    minimal_time_ = this->search_direction_ == search_dir::FWD
                        ? std::numeric_limits<time>::min()
                        : std::numeric_limits<time>::max();
  }

  time_diff_t time_from_label(Label& l) const override {

    const node* node = l.get_node();

    time current_time = l.current_end();
    auto const& bound = bounds_[node->get_station()->id_];
    if (earlier(current_time, bound.last_time_valid) ||
        current_time == bound.last_time_valid) {
      return bound.time_diff_;
    } else {

      time_diff_t since_last_valid = this->search_direction_ == search_dir::FWD
                                         ? current_time - bound.last_time_valid
                                         : bound.last_time_valid - current_time;
      return bound.time_diff_ - std::min(bound.time_diff_, since_last_valid);
    }
  }

  time_diff_t time_from_node(node const* n) const override {
    return bounds_[n->get_station()->id_].time_diff_;
  };
  bool is_valid_time_diff(time_diff_t diff) const override {
    return diff != INVALID_TIME_DIFF;
  }

  time_diff_t transfers_from_label(Label& l) const override {
    const node* node = l.get_node();

    time current_time = l.current_end();
    auto const& bound = bounds_[node->get_station()->id_];
    if (earlier(current_time, bound.last_time_valid) ||
        current_time == bound.last_time_valid) {
      return bound.transfer_amount_;
    } else {
      return 0;
    }
  }

  interchanges_t transfers_from_node(node const* n) const override {
    return bounds_[n->get_station()->id_].transfer_amount_;
  }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return amount != INVALID_INTERCHANGE_AMOUNT;
  }

  /**
   * Calculates the lower bounds for all nodes.
   * @return true if the calculation was successful and the target could be
   * reached and false if the target is unreachable
   */
  bool calculate() {

    // TODO throw error if csa would decline request
    // TODO: further think about on- vs pretrip
    interval search_interval{this->routing_query_.interval_begin_,
                             this->routing_query_.interval_end_};

    auto const& from_id = this->routing_query_.from_->id_;
    auto const& to_id = this->routing_query_.to_->id_;

    // TODO: Meta start/target not considered so far
    csa::csa_query initial_query(from_id, to_id, search_interval,
                                 this->search_direction_);

    auto const times = motis::csa::get_arrival_times(
        *this->routing_query_.csa_timetable, initial_query);

    auto arrival_times = times[to_id];

    const search_dir notDir = this->search_direction_ == search_dir::FWD
                                  ? search_dir::BWD
                                  : search_dir::FWD;

    std::vector<time> valid_arrival_times;
    time last_added = invalid_time_;
    // only use valid arrival time + use each time only once
    for (auto const& arrival_time : arrival_times) {
      if (arrival_time != invalid_time_ && arrival_time != last_added) {
        valid_arrival_times.push_back(arrival_time);
        last_added = arrival_time;
      }
    }

    // Return if no valid arrival could be found. Note interval extension not
    // included
    if (valid_arrival_times.empty()) {
      return false;
    }

    station_ptr const& to_station =
        this->routing_query_.sched_->stations_[to_id];
    auto const offset = this->search_direction_ == search_dir::FWD
                            ? +to_station->transfer_time_
                            : -to_station->transfer_time_;

    // look if the station can be reached from other stations by foot to prevent
    // a potential additional query
    bool to_reachable_by_foot{false};
    if (this->search_direction_ == search_dir::FWD) {
      auto const& footpaths =
          this->routing_query_.csa_timetable->stations_[to_id]
              .incoming_footpaths_;
      for (auto const& fp : footpaths) {
        to_reachable_by_foot |= fp.from_station_ != to_id;
      }
    } else {
      auto const& footpaths =
          this->routing_query_.csa_timetable->stations_[to_id].footpaths_;
      for (auto const& fp : footpaths) {
        to_reachable_by_foot |= fp.to_station_ != to_id;
      }
    }

    for (auto arrival_time : valid_arrival_times) {

      // Subtract the transfer time from the last station to get the real
      // arrival.
      const time real_arrival = arrival_time - offset;

      // signals ontrip station start because no end of interval
      const interval backwards_interval{real_arrival};
      const csa::csa_query backwards_query(to_id, from_id, backwards_interval,
                                           notDir);

      process_backwards_query(backwards_query);

      // if the station was reached by foot the subtraction of the offset leads
      // to potentially wrong results. Therefore another backwards query has to
      // be submitted
      if (to_reachable_by_foot) {
        const interval foot_backwards_interval{arrival_time};
        const csa::csa_query foot_backwards_query(
            to_id, from_id, foot_backwards_interval, notDir);
        process_backwards_query(foot_backwards_query);
      }
    }

    // decrease the lower bounds by the interchange time as they will be to high
    // for route nodes otherwise. TODO: this could make them too low to
    auto const& all_stations = this->routing_query_.sched_->stations_;
    for (auto i = 0; i < all_stations.size(); ++i) {
      if (is_valid_time_diff(bounds_[i].time_diff_)) {
        time_diff_t subtract = std::max(all_stations[i]->transfer_time_, 0);
        bounds_[i].time_diff_ -= std::min(subtract, bounds_[i].time_diff_);
      }
    }

    return true;
  }

private:
  static constexpr interchanges_t INVALID_INTERCHANGE_AMOUNT =
      std::numeric_limits<interchanges_t>::max();
  static constexpr time_diff_t INVALID_TIME_DIFF =
      std::numeric_limits<time_diff_t>::max();

  struct combined_bound {
    time_diff_t time_diff_{INVALID_TIME_DIFF};
    interchanges_t transfer_amount_{INVALID_INTERCHANGE_AMOUNT};
    /** The last timestamp for which the calculated bound is valid **/
    time last_time_valid{0};
  };

  /**
   * returns combined bound which is the minimum of the give bounds.
   */
  combined_bound min_bound(const combined_bound& bound1,
                           const combined_bound& bound2) {
    auto time_diff = std::min(bound1.time_diff_, bound2.time_diff_);
    auto transfer_amount =
        std::min(bound1.transfer_amount_, bound2.transfer_amount_);

    time last_time_valid{};
    // always use the later valid time
    if (earlier(bound1.last_time_valid, bound2.last_time_valid)) {
      last_time_valid = bound2.last_time_valid;
    } else {
      last_time_valid = bound1.last_time_valid;
    }

    return combined_bound{time_diff, transfer_amount, last_time_valid};
  }

  bool earlier(time t1, time t2) const {
    return this->search_direction_ == search_dir::FWD ? t1 < t2 : t1 > t2;
  }

  lower_bounds_csa::combined_bound get_best_bound_for(
      time search_start,
      const std::array<time, csa::MAX_TRANSFERS + 1>& arr) const {

    time latest_time{minimal_time_};
    interchanges_t least_interchanges{INVALID_INTERCHANGE_AMOUNT};
    bool min_interchanges_found = false;
    // find the "latest" departure time for one station (since the response came
    // from a backwards search the latest departure time corresponds to the
    // fastest connections) and also the least amount of interchanges
    for (auto i = 0; i < arr.size(); ++i) {
      if (earlier(latest_time, arr[i])) {
        latest_time = arr[i];
        if (!min_interchanges_found) {
          least_interchanges = i;
          min_interchanges_found = true;
        }
      }
    }
    time_diff_t diff;

    if (earlier(minimal_time_, latest_time)) {
      // abs(a - b)
      diff = search_start > latest_time ? search_start - latest_time
                                        : latest_time - search_start;
    } else {
      diff = INVALID_TIME_DIFF;
    }
    return combined_bound{diff, least_interchanges, latest_time};
  }

  void process_backwards_query(csa::csa_query const& query) {
    // use a different timetable for the backward search because otherwise
    // "infinite" lb are produced for stations where you can't enter or exit
    // (Hildesheim Gbr)
    auto const backwards_times = motis::csa::get_arrival_times(
        *this->routing_query_.csa_timetable_ignored_restrictions, query);

    // get the lower bounds for each node and look if they were decreased
    for (auto i = 0; i < backwards_times.size(); ++i) {
      combined_bound b =
          get_best_bound_for(query.search_interval_.begin_, backwards_times[i]);
      bounds_[i] = min_bound(bounds_[i], b);
    }
  }

  time minimal_time_;
  time invalid_time_;

  std::vector<combined_bound> bounds_;
};

}  // namespace motis::routing
