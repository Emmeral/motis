#pragma once

#include "motis/core/schedule/edges.h"

#include <cmath>

namespace motis::routing {

struct transfer_classes {
  uint8_t tclass_max_, tclass_sum_;
  bool modified_;
};

struct transfer_classes_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds&) {
    l.tclass_max_ = 0;
    l.tclass_sum_ = 0;
    l.modified_ = false;
  }
};

struct transfer_classes_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds&) {
    if (l.modified_ && l.edge_->m_.type_ != edge::ENTER_EDGE) {
      l.modified_ = false;
    }
    if ((ec.time_ != 0) && l.edge_->m_.type_ == edge::EXIT_EDGE) {
      l.modified_ = true;
    }
    if (l.edge_->m_.type_ == edge::ROUTE_EDGE && l.pred_ != nullptr &&
        l.pred_->pred_ != nullptr && l.pred_->pred_->pred_ != nullptr) {
      auto transfer_l = l.pred_->pred_;
      if (transfer_l->edge_->m_.type_ != edge::EXIT_EDGE) {
        return;
      }
      auto start = transfer_l->pred_->current_end();
      auto ic_time = transfer_l->edge_->get_edge_cost(0, ec.connection_).time_;
      auto d_time = ec.connection_->d_time_;
      uint8_t tc = d_time >= start + round(1.5 * ic_time) ? 0
                   : d_time >= start + ic_time            ? 1
                                                          : 2;
      l.tclass_max_ = std::max(l.tclass_max_, tc);
      l.tclass_sum_ += tc;
    }
  }
};

struct transfer_classes_sum_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.tclass_sum_ > b.tclass_sum_),
          smaller_(a.tclass_sum_ < b.tclass_sum_),
          can_dominate_(a.modified_ == b.modified_) {}
    inline bool greater() const { return can_dominate_ ? greater_ : true; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_, can_dominate_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

struct transfer_classes_max_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.tclass_max_ > b.tclass_max_),
          smaller_(a.tclass_max_ < b.tclass_max_),
          can_dominate_(a.modified_ == b.modified_) {}
    inline bool greater() const { return can_dominate_ ? greater_ : true; }
    inline bool smaller() const { return smaller_; }
    bool greater_, smaller_, can_dominate_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    return domination_info<Label>(a, b);
  }
};

struct transfer_classes_con_selector {
  int created_ = 0;

  bool continue_search() { return created_ < 3; }

  template <typename Label>
  bool create_needed(light_connection const* c, Label& old) {
    auto transfer_l = old.pred_;
    if (c == nullptr || transfer_l == nullptr || transfer_l->pred_ == nullptr ||
        old.connection_ != nullptr || transfer_l->connection_ != nullptr) {
      created_ = 3;
      return false;
    }
    if (transfer_l->edge_->m_.type_ != edge::EXIT_EDGE) {
      created_ = 3;
      return false;
    }

    auto start = transfer_l->pred_->current_end();
    auto ic_time = transfer_l->edge_->get_edge_cost(0, c).time_;
    if (c->d_time_ >= start + round(1.5 * ic_time)) {
      created_ = 3;
      return true;
    }
    if (created_ < 2 && c->d_time_ >= start + ic_time) {
      created_ = 2;
      return true;
    };
    if (created_ < 1) {
      created_ = 1;
      return true;
    }
    return false;
  }

  template <typename Label, typename Edge>
  bool minimum_found(light_connection const* c, Label& old, Edge const&) {
    auto transfer_l = old.pred_;
    if (c == nullptr || transfer_l == nullptr || transfer_l->pred_ == nullptr ||
        old.connection_ != nullptr || transfer_l->connection_ != nullptr) {
      return false;
    }
    if (transfer_l->edge_->m_.type_ != edge::EXIT_EDGE) {
      return false;
    }
    auto start = transfer_l->pred_->current_end();
    auto ic_time = transfer_l->edge_->get_edge_cost(0, c).time_;
    if (c->d_time_ >= start + round(1.5 * ic_time)) {
      return true;
    }
    return false;
  }
};

struct transfer_class_edge_cost_function {
  static edge_cost change_ec(edge_cost ec) {
    if (ec.transfer_) {
      ec.time_ = std::round(ec.time_ * 0.7);
    }
    return ec;
  }
};

}  // namespace motis::routing
