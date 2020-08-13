#pragma once

#include <motis/csa/csa_query.h>
#include <motis/csa/csa_search_shared.h>
#include "motis/routing/lower_bounds/lower_bounds.h"
#include "motis/routing/search_query.h"

namespace motis::routing {

class lower_bounds_csa : public lower_bounds {

public:
  lower_bounds_csa(search_query const& routing_query, search_dir direction);
  time_diff_t time_from_node(node const* n) const override;
  bool is_valid_time_diff(time_diff_t diff) const override;
  interchanges_t transfers_from_node(node const* n) const override;
  bool is_valid_transfer_amount(interchanges_t amount) const override;

  /**
   * Calculates the lower bounds for all nodes.
   * @return true if the calculation was successful and the target could be
   * reached and false if the target is unreachable
   */
  bool calculate();

private:
  static constexpr lower_bounds::interchanges_t INVALID_INTERCHANGE_AMOUNT =
      std::numeric_limits<lower_bounds::interchanges_t>::max();
  static constexpr lower_bounds::time_diff_t INVALID_TIME_DIFF =
      std::numeric_limits<lower_bounds::time_diff_t>::max();

  struct combined_bound {
    time_diff_t time_diff_{INVALID_TIME_DIFF};
    interchanges_t transfer_amount_{INVALID_INTERCHANGE_AMOUNT};

    /**
     * Updates the combined bound to be the the minimum of itself and the given
     * bound
     * @param bound the other bound with potential updates
     */
    void update_with(const combined_bound& bound) {
      time_diff_ = std::min(time_diff_, bound.time_diff_);
      transfer_amount_ = std::min(transfer_amount_, bound.transfer_amount_);
    }
  };

  bool earlier(time t1, time t2) const;
  lower_bounds_csa::combined_bound get_best_bound_for(
      time search_start,
      const std::array<time, csa::MAX_TRANSFERS + 1>& arr) const;

  void process_backwards_query(csa::csa_query const& query);

  const search_query& routing_query_;
  const search_dir direction_;
  time minimal_time_;
  time invalid_time_;

  std::vector<combined_bound> bounds_;
};

}  // namespace motis::routing
