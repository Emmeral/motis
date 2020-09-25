#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include <motis/csa/csa_search_shared.h>
#include <motis/csa/csa_to_journey.h>
#include <motis/csa/run_csa_search.h>
#include <motis/routing/label/criteria/transfers.h>
#include <motis/routing/label/criteria/travel_time.h>
#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/search_query.h"

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
  bool calculate() {

    // TODO throw error if csa would decline request
    // TODO: further think about on- vs pretrip
    motis::interval search_interval{this->routing_query_.interval_begin_,
                                    this->routing_query_.interval_end_};

    auto const& from_id = this->routing_query_.from_->id_;
    auto const& to_id = this->routing_query_.to_->id_;

    // TODO: Meta start/target not considered so far
    csa::csa_query initial_query(from_id, to_id, search_interval,
                                 this->search_direction_);

    csa::response const response = motis::csa::run_csa_search(
        *this->routing_query_.sched_, *this->routing_query_.csa_timetable,
        initial_query, SearchType::SearchType_Default,
        csa::implementation_type::CPU_SSE);

    for (auto const& csa_journey : response.journeys_) {

      journey j =
          csa::csa_to_journey(*this->routing_query_.sched_, csa_journey);
      

      for (auto const& s : j.stops_) {

        auto arrival_time_unix = s.arrival_.timestamp_;
        auto departure_time_unix = s.departure_.timestamp_;

        time departure_time;
        time arrival_time;

        if (departure_time_unix == 0) {
          departure_time = INVALID_TIME;
        } else {
          departure_time = unix_to_motistime(
              this->routing_query_.sched_->schedule_begin_, departure_time_unix);
        }

        if (arrival_time_unix == 0) {
          arrival_time = 0;
        } else {
          arrival_time = unix_to_motistime(
              this->routing_query_.sched_->schedule_begin_, arrival_time_unix);
        }

        interval i = {arrival_time, departure_time};

        auto id =
            this->routing_query_.sched_->eva_to_station_.at(s.eva_no_)->index_;

        optimal_map_[id].push_back(i);
      }
    }

    return !response.journeys_.empty();
  }

private:
  bool is_optimal(Label const& l) const {
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

  std::map<uint32_t, std::vector<interval>> optimal_map_;
};

}  // namespace motis::routing
