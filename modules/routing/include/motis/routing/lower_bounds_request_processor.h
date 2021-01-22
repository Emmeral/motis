#pragma once

#include <motis/routing/lower_bounds/lower_bounds_type.h>
#include "motis/csa/build_csa_timetable.h"
#include "motis/routing/label/configs.h"
#include "motis/routing/lower_bounds/lower_bounds_factory.h"

#include "flatbuffers/flatbuffers.h"

namespace motis::routing {



template <search_dir Dir>
module::msg_ptr process_lb_request(search_query query) {

  std::vector<int> goal_ids{};

  if (query.use_dest_metas_) {

    auto const& goals = query.sched_->stations_[query.to_->id_]->equivalent_;
    for (auto const& goal : goals) {
      goal_ids.emplace_back(goal->index_);
    }
  } else {
    goal_ids.emplace_back(query.to_->id_);
  }

  auto const lbs_result =
      get_lower_bounds_for_query<default_label<search_dir::FWD>>(query,
                                                                 goal_ids, Dir);

  auto const& lbs = lbs_result.bounds_;

  module::message_creator fbb;

  std::vector<flatbuffers::Offset<LowerBoundEntry>> offsets;

  for (auto const& station : query.sched_->stations_) {
    auto index = station->index_;
    auto const& node = *query.sched_->station_nodes_[station->index_];

    auto eva_nr = station->eva_nr_.str();

    std::vector<flatbuffers::Offset<LowerBoundsRouteNodeEntry>> route_offsets;
    for (auto const& rn : node.route_nodes_) {
      auto time = lbs->time_from_node(rn.get());
      bool time_valid = lbs->is_valid_time_diff(time);

      auto interchanges = lbs->transfers_from_node(rn.get());
      bool interchanges_valid = lbs->is_valid_transfer_amount(interchanges);

      auto rn_offset = CreateLowerBoundsRouteNodeEntry(
          fbb, time, time_valid, interchanges, interchanges_valid);
      route_offsets.emplace_back(rn_offset);
    }

    auto time = lbs->time_from_node(&node);
    bool time_valid = lbs->is_valid_time_diff(time);

    auto interchanges = lbs->transfers_from_node(&node);
    bool interchanges_valid = lbs->is_valid_transfer_amount(interchanges);

    auto offset = CreateLowerBoundEntry(
        fbb, fbb.CreateString(eva_nr), index, time, time_valid, interchanges,
        interchanges_valid, fbb.CreateVector(route_offsets));

    offsets.emplace_back(offset);
  }

  auto response = CreateLowerBoundsResponse(fbb, fbb.CreateVector(offsets));
  fbb.create_and_finish(MsgContent_LowerBoundsResponse, response.Union());
  return make_msg(fbb);
}

}  // namespace motis::routing
