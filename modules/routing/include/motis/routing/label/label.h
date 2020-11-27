#pragma once

#include "motis/core/schedule/edges.h"
#include "motis/routing/label/merger.h"
#include "motis/routing/label/optimality.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds.h"

namespace motis::routing {

template <typename... DataClass>
struct label_data : public DataClass... {};

template <search_dir Dir, std::size_t MaxBucket,
          bool PostSearchDominanceEnabled, typename GetBucket, typename Data,
          typename Init, typename Updater, typename Filter, typename Dominance,
          typename PostSearchDominance, typename Comparator,
          typename Optimality = optimality<>,
          typename ResultDominance = void>
struct label : public Data {  // NOLINT
  enum : std::size_t { MAX_BUCKET = MaxBucket };

  label() = default;  // NOLINT

  label(edge const* e, label* pred, time now, lower_bounds<label>& lb,
        light_connection const* lcon = nullptr)
      : pred_(pred),
        edge_(e),
        connection_(lcon),
        start_(pred != nullptr ? pred->start_ : now),
        now_(now),
        dominated_(false) {
    Init::init(*this, lb);
  }

  node const* get_node() const { return edge_->get_destination<Dir>(); }

  template <typename Edge, typename LowerBounds>
  bool create_label(label& l, Edge const& e, LowerBounds& lb, bool no_cost,
                    int additional_time_cost = 0) {
    if (pred_ && e.template get_destination<Dir>() == pred_->get_node()) {
      return false;
    }
    if ((e.type() == edge::BWD_EDGE ||
         e.type() == edge::AFTER_TRAIN_BWD_EDGE) &&
        edge_->type() == edge::EXIT_EDGE) {
      return false;
    }

    auto ec = e.template get_edge_cost<Dir>(now_, connection_);
    if (!ec.is_valid()) {
      return false;
    }
    if (no_cost) {
      ec.time_ = 0;
    } else {
      ec.time_ += additional_time_cost;
    }

    l = *this;
    l.pred_ = this;
    l.edge_ = &e;
    l.connection_ = ec.connection_;
    l.now_ += (Dir == search_dir::FWD) ? ec.time_ : -ec.time_;

    Updater::update(l, ec, lb);
    return !l.is_filtered();
  }

  inline bool is_filtered() { return Filter::is_filtered(*this); }

  bool dominates(label const& o) {
    if (incomparable(o)) {
      return false;
    }

    bool dominates =  Dominance::dominates(false, *this, o);
    /**
    if(dominates && o.is_on_optimal_journey()){
      o.transfer_optimality_to(*this);
    }
    **/
    return dominates;
  }

  /**
   * Returns true if this label has a chance to be in the given result set
   */
  bool may_be_in_result_set(const std::vector<label*>& result_set,
                            const std::vector<label*>& optimal_results) {

    // we already know that labels on optimal journeys will be a
    // valid result
    if (is_on_optimal_journey()) {
      return true;
    }

    auto comparable = [*this](label const& result) {
      return this->current_begin() == result.current_begin();
    };

    bool optimals_exist = false;
    constexpr bool result_dominance_exists =
        !std::is_same<ResultDominance, void>::value;

    // we can only apply the improved domination if the result dominance
    // template parameter is defined
    if constexpr (result_dominance_exists) {

      for (const label* opt_res : optimal_results) {

        if (!comparable(*opt_res)) {
          continue;
        }
        optimals_exist = true;

        bool merged_valid = true;
        for (label* r : result_set) {
          if (comparable(*r)) {
            if (ResultDominance::result_dominates(true, *r, *this, *opt_res)) {
              merged_valid = false;
              break;
            }
          }
        }
        // if the merged label was not dominated the original label can be
        // part of the result set
        if (merged_valid) {
          return true;
        }
      }

      if (optimals_exist) {
        return false;
      }
    }

    // if no optimal labels exists perform a default domination check
    if (!optimals_exist || !result_dominance_exists) {
      for (label* r : result_set) {
        if (comparable(*r)) {
          if (Dominance::dominates(false, *r, *this)) {
            return false;
          }
        }
      }
    }

    return true;
  }

  bool is_on_optimal_journey() const {
    return Optimality::is_on_optimal_journey(*this);
  }

  void transfer_optimality_to(label& new_label) const {
    Optimality::transfer_optimality(*this, new_label);
  }

  bool incomparable(label const& o) const {
    return current_begin() < o.current_begin() ||
           current_end() > o.current_end();
  }

  time current_begin() const { return Dir == search_dir::FWD ? start_ : now_; }

  time current_end() const { return Dir == search_dir::FWD ? now_ : start_; }

  bool dominates_post_search(label const& o) const {
    return PostSearchDominance::dominates(false, *this, o);
  }

  bool operator<(label const& o) const {
    return Comparator::lexicographical_compare(*this, o);
  }

  static inline bool is_post_search_dominance_enabled() {
    return PostSearchDominanceEnabled;
  }

  std::size_t get_bucket() const { return GetBucket()(this); }

  label* pred_;
  edge const* edge_;
  light_connection const* connection_;
  time start_, now_;
  bool dominated_;
};

}  // namespace motis::routing
