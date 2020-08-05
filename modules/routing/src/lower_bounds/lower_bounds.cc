#include "motis/routing/lower_bounds/lower_bounds.h"

lower_bounds_stats motis::routing::lower_bounds::get_stats(
    const schedule& sched) {

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
