#pragma once

#include "motis/core/schedule/edges.h"

namespace motis::routing {

constexpr duration MAX_TRANSFERS = 7;

struct transfers {
  uint8_t transfers_, transfers_lb_;
  bool on_optimal_transfers_journey_;
};

struct transfers_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds& lb) {
    l.transfers_ = 0;

    l.on_optimal_transfers_journey_ = lb.is_on_optimal_transfers_journey(l);
    auto const lb_val = lb.transfers_from_label(l);
    if (lb.is_valid_transfer_amount(lb_val)) {
      l.transfers_lb_ = lb_val;
    } else {
      l.transfers_lb_ = std::numeric_limits<uint8_t>::max();
    }
  }
};

struct transfers_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds& lb) {
    if (ec.transfer_) {
      ++l.transfers_;
    }

    l.on_optimal_transfers_journey_ = lb.is_on_optimal_transfers_journey(l);

    auto const lb_val = lb.transfers_from_label(l);
    if (lb.is_valid_transfer_amount(lb_val)) {
      l.transfers_lb_ = l.transfers_ + lb_val;
    } else {
      l.transfers_lb_ = std::numeric_limits<uint8_t>::max();
    }
  }
};

struct transfers_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.transfers_lb_ > b.transfers_lb_),
          smaller_(a.transfers_lb_ < b.transfers_lb_) {}
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

struct transfers_result_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.transfers_lb_ > b.transfers_lb_),
          smaller_(a.transfers_lb_ < b.transfers_lb_) {
      if (b.on_optimal_time_journey_ && !a.on_optimal_time_journey_) {
        greater_ = a.travel_time_lb_ > b.travel_time_lb_;
        smaller_ = false;
      }
    }
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

struct transfers_merger {
  template <typename Label>
  static void merge_with_optimal_result(Label const& original_label,
                                        Label const& result_label,
                                        Label& merged_label) {
    merged_label.transfers_lb_ =
        std::max(result_label.transfers_lb_, original_label.transfers_lb_);
  }
};

struct transfers_filter {
  template <typename Label>
  static bool is_filtered(Label const& l) {
    return l.transfers_lb_ > MAX_TRANSFERS;
  }
};

struct transfers_optimality {
  template <typename Label>
  static bool is_on_optimal_journey(Label const& l) {
    return l.on_optimal_transfers_journey_;
  }
};

}  // namespace motis::routing
