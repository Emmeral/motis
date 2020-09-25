#pragma once

#include <cstdint>
#include "motis/core/schedule/nodes.h"
#include "motis/core/schedule/schedule.h"
#include "motis/routing/lower_bounds/lower_bounds_stats.h"
#include "motis/routing/search_query.h"

namespace motis::routing {


using time_diff_t = uint32_t;
using interchanges_t = uint32_t;

template <typename Label>
class lower_bounds {
public:
  lower_bounds() = delete;
  lower_bounds(search_query const& routing_query, search_dir search_direction)
      : routing_query_(routing_query), search_direction_(search_direction) {}

  virtual time_diff_t time_from_node(node const* n) const = 0;
  virtual time_diff_t time_from_label(Label& l) const {
    return time_from_node(l.get_node());
  }
  virtual bool is_valid_time_diff(time_diff_t diff) const = 0;

  virtual interchanges_t transfers_from_label(Label& l) const {
    return transfers_from_node(l.get_node());
  };
  virtual interchanges_t transfers_from_node(node const* n) const = 0;
  virtual bool is_valid_transfer_amount(interchanges_t amount) const = 0;

  /**
   * Returns whether the label is on an optimal journey regarding the travel
   * time. If not known whether this is true returns false.
   * @param l
   * @return true if on optimal journey regarding travel time
   */
  virtual bool is_on_optimal_time_journey(Label& /*l*/) const { return false; }
  /**
   * Returns whether the label is on an optimal journey regarding transfers.
   * If not known whether this is true returns false.
   * @param l
   * @return true if on optimal journey regarding transfers
   */
  virtual bool is_on_optimal_transfers_journey(Label& /*l*/) const { return false; }

  lower_bounds_stats get_stats() const {

    schedule const& sched = *routing_query_.sched_;

    auto& stations_nodes = sched.station_nodes_;
    uint64_t sum_times{0L}, sum_transfers{0L};
    uint32_t invalid_time_count{0}, invalid_transfers_count{0};
    uint32_t valid_time_count{0}, valid_transfers_count{0};

    auto process_node = [&, this](const node* n) {
      auto time = this->time_from_node(n);
      auto transfers = this->transfers_from_node(n);
      if (this->is_valid_time_diff(time)) {
        sum_times += time;
        ++valid_time_count;
      } else {
        ++invalid_time_count;
      }

      if (this->is_valid_transfer_amount(transfers)) {
        sum_transfers += transfers;
        ++valid_transfers_count;
      } else {
        ++invalid_transfers_count;
      }
    };

    for (auto& n : stations_nodes) {
      process_node(n.get());  // only process stations as route nodes may alter
      // the statistics
    }

    double avg_time = static_cast<double>(sum_times) / valid_time_count;
    double avg_transfers =
        static_cast<double>(sum_transfers) / valid_transfers_count;

    return lower_bounds_stats{avg_time, avg_transfers, invalid_time_count,
                              invalid_transfers_count};
  }

  virtual ~lower_bounds() = default;

protected:
  search_query const& routing_query_;
  search_dir const search_direction_;
};



}  // namespace motis::routing
