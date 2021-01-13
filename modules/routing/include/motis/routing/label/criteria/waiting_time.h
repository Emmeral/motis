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

    const node* l_node = l.get_node();
    if (l_node->is_route_node() && l.connection_ == nullptr) {
      assert(std::count_if(l_node->edges_.begin(), l_node->edges_.end(),
                           [](edge const& e) {
                             return e.m_.type_ == edge::ROUTE_EDGE;
                           }) <= 1);
      for (edge const& e : l_node->edges_) {
        if (e.m_.type_ == edge::ROUTE_EDGE) {
          light_connection const* c = e.template get_connection(l.now_);
          if (c != nullptr) {
            auto wt = c->d_time_ - l.now_;
            l.waiting_time_ += wt;
            if(wt > MAX_NEGLECTED_WAITING_TIME){
              l.adjusted_waiting_time_ += (wt - MAX_NEGLECTED_WAITING_TIME);
            }
          } else {
            l.waiting_time_ = MAX_WAITING_TIME +1;
            l.adjusted_waiting_time_ = MAX_WAITING_TIME +1;
          }
          return;
        }
      }
      l.waiting_time_ = MAX_WAITING_TIME +1;
      l.adjusted_waiting_time_ = MAX_WAITING_TIME +1;
    }
  }
};

struct waiting_time_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b,
                    bool result_domination = false)
        : greater_(a.adjusted_waiting_time_ > b.adjusted_waiting_time_),
          smaller_(a.adjusted_waiting_time_ < b.adjusted_waiting_time_) {

      if (result_domination) {
        return;
      }

      // TODO: constraints
      // we might need to wait until the other label as we don't know
      // when the next train departs
      if (b.now_ > a.now_ &&
          !(a.get_node()->is_route_node() && a.connection_ == nullptr)) {
        duration awt = a.adjusted_waiting_time_ + (b.now_ - a.now_);
        greater_ = awt > b.adjusted_waiting_time_;
        smaller_ = awt < b.adjusted_waiting_time_;
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

  typedef bool has_result_dominates;
};

struct waiting_time_filter {
  template <typename Label>
  static bool is_filtered(Label const& l) {
    return l.waiting_time_ > MAX_WAITING_TIME;
  }
};

}  // namespace motis::routing
