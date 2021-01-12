#pragma once

#include "travel_time.h"

namespace motis::routing {

constexpr duration MAX_NEGLECTED_WAITING_TIME = 10;
constexpr duration MAX_WAITING_TIME = MAX_TRAVEL_TIME;

struct waiting_time {
  duration waiting_time_;
  duration adjusted_waiting_time_;
};

struct waiting_time_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds&) {
    l.waiting_time_ = 0;
    l.adjusted_waiting_time_ = 0;
  }
};

struct waiting_time_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds&) {
    // TODO: one could enter the train when it arrives at a station

    if (l.pred_->edge_->type() != edge::ROUTE_EDGE &&
        l.pred_->connection_ == nullptr) {
      l.waiting_time_ += ec.waiting_time_;
      if (ec.waiting_time_ > MAX_NEGLECTED_WAITING_TIME) {
        l.adjusted_waiting_time_ +=
            (ec.waiting_time_ - MAX_NEGLECTED_WAITING_TIME);
      }
    }
  }
};

struct waiting_time_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b, bool result_domination = false)
        : greater_(a.waiting_time_ > b.waiting_time_),
          smaller_(a.waiting_time_ < b.waiting_time_) {

      if(result_domination){
        return;
      }

      if(a.connection_ != nullptr && b.connection_ == nullptr && a.now_ < b.now_){
        greater_ = false;
      }

      if(b.connection_ != nullptr && a.connection_ == nullptr && b.now_ < a.now_){
        smaller_ = false;
      }

      if (a.now_ > b.now_) {
        duration bwt = b.waiting_time_ + (a.now_ - b.now_);
        greater_ = a.waiting_time_ > bwt;
      }
      if (b.now_ > a.now_) {
        duration awt = a.waiting_time_ + (b.now_ - a.now_);
        smaller_ = b.waiting_time_ > awt;
      }
    }
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    auto dom_info = domination_info<Label>(a, b);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(Label const& a,
                                                 Label const& b) {
    auto dom_info = domination_info<Label>(a, b, true);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(
      Label const& a, Label const& b, Label const& /* opt_result_to_merge */) {
    return result_dominates(a, b);
  }
};

struct waiting_time_filter {
  template <typename Label>
  static bool is_filtered(Label const& l) {
    return l.waiting_time_ > MAX_WAITING_TIME;
  }
};

}  // namespace motis::routing
