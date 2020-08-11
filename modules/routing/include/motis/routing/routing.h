#pragma once

#include <motis/routing/lower_bounds/lower_bounds_type.h>
#include <memory>
#include <mutex>
#include <vector>

#include "motis/module/module.h"

#include "motis/csa/csa.h"

namespace motis::routing {

struct memory;

struct routing : public motis::module::module {
  routing();
  ~routing() override;

  routing(routing const&) = delete;
  routing& operator=(routing const&) = delete;

  routing(routing&&) = delete;
  routing& operator=(routing&&) = delete;

  void init(motis::module::registry&) override;

private:
  motis::module::msg_ptr ontrip_train(motis::module::msg_ptr const&);
  motis::module::msg_ptr route(motis::module::msg_ptr const&);

  motis::module::msg_ptr lower_bounds(motis::module::msg_ptr const&);

  static motis::module::msg_ptr trip_to_connection(
      motis::module::msg_ptr const&);

  std::mutex mem_pool_mutex_;
  std::vector<std::unique_ptr<memory>> mem_pool_;
  lower_bounds_type lb_type_{lower_bounds_type::CG};

  bool extended_lb_stats_{false};

  std::unique_ptr<csa::csa_timetable> csa_timetable_;
  std::unique_ptr<csa::csa_timetable> csa_timetable_ignored_restrictions_;
};

}  // namespace motis::routing
