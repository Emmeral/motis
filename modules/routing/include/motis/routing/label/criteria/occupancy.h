
#pragma once

#include "motis/core/schedule/edges.h"

namespace motis {
namespace routing {

struct occupancy {
  uint16_t occ_;
  uint8_t max_occ_;
  bool sitting_;
};

struct occupancy_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds&) {
    l.occ_ = 0;
    l.max_occ_ = 0;
    l.sitting_ = false;
  }
};

template <bool Sit>
struct occupancy_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds&) {
    if (ec.transfer_) {
      l.sitting_ = false;
    }
    if (ec.connection_ != nullptr && (!l.sitting_ || !Sit)) {
      if (ec.connection_->occupancy_ == 0) {
        l.sitting_ = true;
      } else {
        l.occ_ += (ec.connection_->occupancy_ * ec.connection_->travel_time());
        l.max_occ_ = std::max(ec.connection_->occupancy_, l.max_occ_);
      }
    }
  }
};

struct occupancy_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.occ_ > b.occ_), smaller_(a.occ_ < b.occ_) {}
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

struct occupancy_dominance_max {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.max_occ_ > b.max_occ_),
          smaller_(a.max_occ_ < b.max_occ_) {}
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

/*
enum { NEXT_0, PARETO };

template <uint8_t Mode>
struct occupancy_connection_selector {
  bool min_created = false;
  time min_to_0_occ_1 = UINT16_MAX;
  time min_to_0_occ_2 = UINT16_MAX;

  bool continue_search() { return !min_created; }

  template <typename Label>
  bool create_needed(light_connection const* c, Label& old) {

    if (c == nullptr || old.connection_ != nullptr || min_created) {
      return false;
    }
    if (c->occupancy_ == 0) {
      min_created = true;
      return true;
    }
    if (Mode == NEXT_0) {
      return true;
    }
    if (Mode == PARETO) {
      auto max_occ = std::max(old.max_occ_, c->max_occ_to_0_);
      // Separate times for max_occ of 1 and 2 since it is one of the two pareto
      // criteria for occupancy
      if (c->occtime_to_0_occ_ >= min_to_0_occ_1) {
        return false;
      }
      if (max_occ == 2 && c->occtime_to_0_occ_ >= min_to_0_occ_2) {
        return false;
      }
      if (max_occ == 1) {
        min_to_0_occ_1 = c->occtime_to_0_occ_;
      } else {
        min_to_0_occ_2 = c->occtime_to_0_occ_;
      }
      return true;
    }

    // TODO: Search Dir Beachten?

    // FOUND MIN OPTIMMIZATION => REQUIRES EDGE
    if (c->occupancy_ == e.m_.route_edge_.min_occupancy_) {
      found_min = true;
      return true;
    }

}
template <typename Label, typename Edge>
bool minimum_found(light_connection const* con, Label&, Edge const&) {
  if (con->occupancy_ == 0) {
    return true;
  }
}
};
*/

}  // namespace routing
}  // namespace motis
