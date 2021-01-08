#pragma once

namespace motis::routing {

constexpr duration MAX_TRAVEL_TIME = 1440;

struct travel_time {
  duration travel_time_, travel_time_lb_;
  bool on_optimal_time_journey_;
};

struct get_travel_time_lb {
  template <typename Label>
  duration operator()(Label const* l) {
    return l->travel_time_lb_;
  }
};
struct get_remaining_travel_time {
  template <typename Label>
  duration operator()(Label const* l) {
    return l->travel_time_lb_ - l->travel_time_;
  }
};


struct travel_time_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds& lb) {
    l.travel_time_ = std::abs(l.now_ - l.start_);

    l.on_optimal_time_journey_ = lb.is_on_optimal_time_journey(l);

    auto const lb_val = lb.time_from_label(l);
    if (lb.is_valid_time_diff(lb_val)) {
      l.travel_time_lb_ = l.travel_time_ + lb_val;
    } else {
      l.travel_time_lb_ = std::numeric_limits<duration>::max();
    }
  }
};

struct travel_time_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds& lb) {
    l.travel_time_ += ec.time_;

    l.on_optimal_time_journey_ = lb.is_on_optimal_time_journey(l);

    auto const lb_val = lb.time_from_label(l);
    if (lb.is_valid_time_diff(lb_val)) {
      l.travel_time_lb_ = l.travel_time_ + lb_val;
    } else {
      l.travel_time_lb_ = std::numeric_limits<duration>::max();
    }
  }
};

struct travel_time_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b, duration merged_lb = 0)
        : greater_(a.travel_time_lb_ > std::max(b.travel_time_lb_, merged_lb)),
          smaller_(a.travel_time_lb_ < std::max(b.travel_time_lb_, merged_lb)) {
    }
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
  template <typename Label>
  static domination_info<Label> result_dominates(Label const& a,
                                                 Label const& b) {
    return domination_info<Label>(a, b);
  }
  template <typename Label>
  static domination_info<Label> result_dominates(
      Label const& a, Label const& b, Label const& opt_result_to_merge) {
    return domination_info<Label>(a, b, opt_result_to_merge.travel_time_lb_);
  }

  typedef bool has_result_dominates;
};


struct travel_time_alpha_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : travel_time_a_(a.travel_time_),
          travel_time_b_(b.travel_time_),
          dist_(std::min(std::abs(static_cast<int>(a.start_) - b.start_),
                         std::abs(static_cast<int>(a.now_) - b.now_))) {}

    domination_info(time travel_time_a, time travel_time_b, time dist)
        : travel_time_a_(travel_time_a),
          travel_time_b_(travel_time_b),
          dist_(dist) {}

    inline bool greater() const {
      auto a = 5.0F * (static_cast<double>(travel_time_a_) / travel_time_b_);
      return travel_time_a_ + a * dist_ > travel_time_b_;
    }

    inline bool smaller() const {
      auto a = 5.0F * (static_cast<double>(travel_time_a_) / travel_time_b_);
      return travel_time_a_ + a * dist_ < travel_time_b_;
    }

    time travel_time_a_, travel_time_b_;
    time dist_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};


struct travel_time_filter {
  template <typename Label>
  static bool is_filtered(Label const& l) {
    return l.travel_time_lb_ > MAX_TRAVEL_TIME;
  }
};

struct travel_time_optimality {
  template <typename Label>
  static bool is_on_optimal_journey(Label const& l) {
    return l.on_optimal_time_journey_;
  }

  template <typename Label>
  static void transfer_optimality(Label& new_l, Label const& old_l) {
    if (old_l.on_optimal_time_journey_) {
      new_l.on_optimal_time_journey_ = true;
    }
  }
};

}  // namespace motis::routing
