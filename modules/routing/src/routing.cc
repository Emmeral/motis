#include "motis/routing/routing.h"

#include "boost/date_time/gregorian/gregorian_types.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/program_options.hpp"
#include <string>

#include "utl/to_vec.h"

#include "motis/core/common/logging.h"
#include "motis/core/common/timing.h"
#include "motis/core/schedule/schedule.h"
#include "motis/core/access/edge_access.h"
#include "motis/core/conv/trip_conv.h"
#include "motis/core/journey/journeys_to_message.h"
#include "motis/module/context/get_schedule.h"

#include "motis/routing/additional_edges.h"
#include "motis/routing/build_query.h"
#include "motis/routing/error.h"
#include "motis/routing/label/configs.h"
#include "motis/routing/mem_manager.h"
#include "motis/routing/mem_retriever.h"
#include "motis/routing/search.h"
#include "motis/routing/search_dispatch.h"
#include "motis/routing/start_label_generators/ontrip_gen.h"
#include "motis/routing/start_label_generators/pretrip_gen.h"

#include "motis/csa/build_csa_timetable.h"

#include "flatbuffers/flatbuffers.h"

// 64MB default start size
constexpr auto LABEL_STORE_START_SIZE = 64 * 1024 * 1024;

using namespace motis::logging;
using namespace motis::module;

namespace motis::routing {

routing::routing() : module("Routing", "routing") {

  param(lb_type_, "lb_type",
        "Select the method to calculate lower bounds (cg|csa|mixed|none)");
  param(extended_lb_stats_, "extended_lb_stats",
        "Set if extended stats about the lower bounds shall be calculated. The "
        "processing of a query will need more time if set");
}

routing::~routing() = default;

void routing::init(motis::module::registry& reg) {

  reg.register_op("/routing",
                  [this](msg_ptr const& msg) { return route(msg); });
  reg.register_op("/trip_to_connection", &routing::trip_to_connection);

  reg.register_op("/routing/lower_bounds",
                  [this](msg_ptr const& msg) { return lower_bounds(msg); });

  if (lb_type_ == lower_bounds_type::CSA) {
    LOG(logging::info) << "Building CSA timetable for routing";
    auto const& sched = get_sched();
    csa_timetable_ =
        motis::csa::build_csa_timetable(sched, false, false, false);
    csa_timetable_ignored_restrictions_ =
        motis::csa::build_csa_timetable(sched, false, false, true);
  }
}

msg_ptr routing::route(msg_ptr const& msg) {
  MOTIS_START_TIMING(routing_timing);

  auto const req = motis_content(RoutingRequest, msg);
  auto const& sched = get_schedule();
  auto query = build_query(sched, req);

  mem_retriever mem(mem_pool_mutex_, mem_pool_, LABEL_STORE_START_SIZE);
  query.mem_ = &mem.get();

  query.extended_lb_stats_ = extended_lb_stats_;
  query.lb_type = lb_type_;
  if (lb_type_ == lower_bounds_type::CSA) {
    query.csa_timetable = csa_timetable_.get();
    query.csa_timetable_ignored_restrictions =
        csa_timetable_ignored_restrictions_.get();
  }

  auto res = search_dispatch(query, req->start_type(), req->search_type(),
                             req->search_dir());

  MOTIS_STOP_TIMING(routing_timing);
  res.stats_.total_calculation_time_ = MOTIS_TIMING_MS(routing_timing);
  res.stats_.labels_created_ = query.mem_->allocations();
  res.stats_.num_bytes_in_use_ = query.mem_->get_num_bytes_in_use();

  message_creator fbb;
  std::vector<flatbuffers::Offset<Statistics>> stats{
      to_fbs(fbb, "routing", res.stats_)};
  fbb.create_and_finish(
      MsgContent_RoutingResponse,
      CreateRoutingResponse(
          fbb, fbb.CreateVectorOfSortedTables(&stats),
          fbb.CreateVector(utl::to_vec(
              res.journeys_,
              [&](journey const& j) { return to_connection(fbb, j); })),
          motis_to_unixtime(sched, res.interval_begin_),
          motis_to_unixtime(sched, res.interval_end_),
          fbb.CreateVector(
              std::vector<flatbuffers::Offset<DirectConnection>>{}))
          .Union());
  return make_msg(fbb);
}

msg_ptr routing::trip_to_connection(msg_ptr const& msg) {
  using label = default_label<search_dir::FWD>;

  auto const& sched = get_schedule();
  auto trp = from_fbs(sched, motis_content(TripId, msg));

  auto const first = trp->edges_->front()->from_;
  auto const last = trp->edges_->back()->to_;

  auto const e_0 = make_foot_edge(nullptr, first->get_station());
  auto const e_1 = make_enter_edge(first->get_station(), first);
  auto const e_n = make_exit_edge(last, last->get_station());

  auto const dep_time = get_lcon(trp->edges_->front(), trp->lcon_idx_).d_time_;

  auto const make_label = [&](label* pred, edge const* e,
                              light_connection const* lcon, time now) {
    auto l = label();
    l.pred_ = pred;
    l.edge_ = e;
    l.connection_ = lcon;
    l.start_ = dep_time;
    l.now_ = now;
    l.dominated_ = false;
    return l;
  };

  auto labels = std::vector<label>{trp->edges_->size() + 3};
  labels[0] = make_label(nullptr, &e_0, nullptr, dep_time);
  labels[1] = make_label(&labels[0], &e_1, nullptr, dep_time);

  int i = 2;
  for (auto const& e : *trp->edges_) {
    auto const& lcon = get_lcon(e, trp->lcon_idx_);
    labels[i] = make_label(&labels[i - 1], e, &lcon, lcon.a_time_);
    ++i;
  }

  labels[i] = make_label(&labels[i - 1], &e_n, nullptr, labels[i - 1].now_);

  message_creator fbb;
  fbb.create_and_finish(
      MsgContent_Connection,
      to_connection(
          fbb, output::labels_to_journey(sched, &labels[i], search_dir::FWD))
          .Union());
  return make_msg(fbb);
}

msg_ptr routing::lower_bounds(msg_ptr const& msg) {
  auto const req = motis_content(RoutingRequest, msg);
  auto const& sched = get_schedule();
  auto query = build_query(sched, req);

  query.extended_lb_stats_ = extended_lb_stats_;
  query.lb_type = lb_type_;
  if (lb_type_ == lower_bounds_type::CSA ||
      lb_type_ == lower_bounds_type::MIXED) {
    query.csa_timetable = csa_timetable_.get();
    query.csa_timetable_ignored_restrictions =
        csa_timetable_ignored_restrictions_.get();
  }

  std::vector<int> goal_ids{};

  if (query.use_dest_metas_) {

    auto const& goals = query.sched_->stations_[query.to_->id_]->equivalent_;
    for (auto const& goal : goals) {
      goal_ids.emplace_back(goal->index_);
    }
  } else {
    goal_ids.emplace_back(query.to_->id_);
  }

  auto dir = req->search_dir() == SearchDir_Forward ? search_dir::FWD
                                                    : search_dir::BWD;
  auto const& lbs_result =
      lower_bounds::get_lower_bounds_for_query(query, goal_ids, dir);
  auto const& lbs = lbs_result.bounds_;

  message_creator fbb;

  std::vector<flatbuffers::Offset<LowerBoundEntry>> offsets;
  for (auto const& station : query.sched_->stations_) {
    auto const& node = query.sched_->station_nodes_[station->index_].get();

    auto time = lbs->time_from_node(node);
    bool time_valid = lbs->is_valid_time_diff(time);

    auto interchanges = lbs->transfers_from_node(node);
    bool interchanges_valid = lbs->is_valid_transfer_amount(interchanges);

    auto eva_nr = station->eva_nr_.str();

    auto offset =
        CreateLowerBoundEntry(fbb, fbb.CreateString(eva_nr), time, time_valid,
                              interchanges, interchanges_valid);

    offsets.emplace_back(offset);
  }

  auto response = CreateLowerBoundsResponse(fbb, fbb.CreateVector(offsets));
  fbb.create_and_finish(MsgContent_LowerBoundsResponse, response.Union());
  return make_msg(fbb);
}

}  // namespace motis::routing
