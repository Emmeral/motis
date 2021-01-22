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
template <typename Label, typename TravelTimeDelegate,
          typename TransferDelegate, typename PriceDelegate,
          typename OptimalityDelegate>
class lower_bounds_delegator : public lower_bounds<Label> {

public:
  lower_bounds_delegator(search_query const& routing_query,
                         search_dir direction,
                         TravelTimeDelegate&& travel_time_delegate,
                         TransferDelegate&& transfer_delegate,
                         PriceDelegate&& price_delegate,
                         OptimalityDelegate&& optimality_delegate)
      : lower_bounds<Label>(routing_query, direction),
        travel_time_delegate_{std::move(travel_time_delegate)},
        transfers_delegate_{std::move(transfer_delegate)},
        price_delegate_{std::move(price_delegate)},
        optimality_delegate_{std::move(optimality_delegate)} {}

  time_diff_t time_from_label(Label& l) const override {
    return travel_time_delegate_.time_from_label(l);
  }

  time_diff_t time_from_node(node const* n) const override {
    return travel_time_delegate_.time_from_node(n);
  };
  bool is_valid_time_diff(time_diff_t diff) const override {
    return travel_time_delegate_.is_valid_time_diff(diff);
  }

  time_diff_t transfers_from_label(Label& l) const override {
    return transfers_delegate_.transfers_from_label(l);
  }

  interchanges_t transfers_from_node(node const* n) const override {
    return transfers_delegate_.transfers_from_node(n);
  }
  bool is_valid_transfer_amount(interchanges_t amount) const override {
    return transfers_delegate_.is_valid_transfer_amount(amount);
  }

  uint16_t price_from_node(node const* n) override {
    return price_delegate_.price_from_node(n);
  };
  bool is_valid_price_amount(uint16_t amount) const override {
    return price_delegate_.is_valid_price_amount(amount);
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

    lower_bounds_result<Label> r_final;
    r_final.target_reachable = reachable;
    lower_bounds_result<Label> r_temp;
    if (r_final.target_reachable) {
      r_temp = travel_time_delegate_.calculate();
    }
    r_final.target_reachable =
        r_final.target_reachable && r_temp.target_reachable;
    r_final.travel_time_lb_ = r_temp.travel_time_lb_;
    if (r_final.target_reachable) {
      r_temp = transfers_delegate_.calculate();
    }

    r_final.target_reachable =
        r_final.target_reachable && r_temp.target_reachable;
    r_final.transfers_lb_ = r_temp.transfers_lb_;
    if (r_final.target_reachable) {
      r_temp = price_delegate_.calculate();
    }

    r_final.target_reachable =
        r_final.target_reachable && r_temp.target_reachable;
    r_final.price_lb_ = r_temp.price_lb_;

    r_final.optimality_lb_ = MOTIS_TIMING_MS(optimality_timing);
    r_final.total_lb = r_final.optimality_lb_ + r_final.travel_time_lb_ +
                       r_final.transfers_lb_ + r_final.price_lb_;

    return r_final;
  }

private:
  TravelTimeDelegate travel_time_delegate_;
  TransferDelegate transfers_delegate_;
  PriceDelegate price_delegate_;
  OptimalityDelegate optimality_delegate_;
};

}  // namespace motis::routing
