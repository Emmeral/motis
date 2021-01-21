#pragma once

#include <motis/csa/csa.h>
#include <motis/csa/csa_query.h>
#include <motis/csa/csa_search_shared.h>
#include <motis/csa/csa_to_journey.h>
#include <motis/csa/run_csa_search.h>
#include <motis/routing/label/criteria/transfers.h>
#include <motis/routing/label/criteria/travel_time.h>
#include "motis/routing/lower_bounds/optimality/csa_optimality_calculator.h"
#include "motis/routing/search_query.h"
#include "lower_bounds.h"

namespace motis::routing {
template <typename Label, typename BoundsDelegate, typename OptimalityDelegate>
class lower_bounds_optimality_delegate : public lower_bounds<Label> {

public:
  lower_bounds_optimality_delegate(
      search_query const& routing_query, search_dir direction,
      BoundsDelegate&& bounds_delegate,
      OptimalityDelegate&& optimality_delegate)
      : lower_bounds<Label>(routing_query, direction),
        bounds_delegate_{std::move(bounds_delegate)},
        optimality_delegate_{std::move(optimality_delegate)} {}

  time_diff_t time_from_label(Label& l) const override {
    return bounds_delegate_.time_from_label(l);
  }

  time_diff_t time_from_node(node const* n) const override {
    return bounds_delegate_.time_from_node(n);
  };
  bool is_valid_time_diff(time_diff_t diff) const override {
    return bounds_delegate_.is_valid_time_diff(diff);
  }

  time_diff_t transfers_from_label(Label& l) const override {
    return bounds_delegate_.transfers_from_label(l);
  }

  interchanges_t transfers_from_node(node const* n) const override {
    return bounds_delegate_.transfers_from_node(n);
  }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return bounds_delegate_.is_valid_transfer_amount(amount);
  }

  uint16_t price_from_node(node const* n) override {
    return bounds_delegate_.price_from_node(n);
  };
  bool is_valid_price_amount(uint16_t amount) const override {
    return bounds_delegate_.is_valid_price_amount(amount);
  };

  bool is_on_optimal_time_journey(Label& l) const override {
    return optimality_delegate_.is_optimal(l);
  }
  bool is_on_optimal_transfers_journey(Label& l) const override {
    return optimality_delegate_.is_optimal(l);
  }

  int get_optimal_journey_count() const override {
    return optimality_delegate_.get_optimal_journey_count();
  }

  /**
   * @return true if the calculation was successful and the target could be
   * reached and false if the target is unreachable
   */
  lower_bounds_result<Label> calculate() override {

    MOTIS_START_TIMING(optimality_timing);
    bool reachable =
        optimality_delegate_.calculate_optimality(this->routing_query_);
    MOTIS_STOP_TIMING(optimality_timing);

    lower_bounds_result<Label> r;
    if (reachable) {
      r = bounds_delegate_.calculate();
    }
    r.target_reachable = reachable;
    r.optimality_lb_ = MOTIS_TIMING_MS(optimality_timing);
    r.total_lb = r.optimality_lb_ + r.travel_time_lb_ + r.transfers_lb_;

    return r;
  }

private:
  BoundsDelegate bounds_delegate_;
  OptimalityDelegate optimality_delegate_;
};

}  // namespace motis::routing
