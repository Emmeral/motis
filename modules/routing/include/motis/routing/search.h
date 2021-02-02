#pragma once

#include <motis/routing/lower_bounds/lower_bounds_factory.h>
#include <motis/routing/lower_bounds/evaluator.h>
#include "motis/csa/build_csa_timetable.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds_none.h"
#include "motis/routing/search_query.h"
#include "utl/to_vec.h"

#include "motis/hash_map.h"

#include "motis/core/common/timing.h"
#include "motis/core/schedule/schedule.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds.h"
#include "motis/routing/output/labels_to_journey.h"
#include "motis/routing/pareto_dijkstra.h"

namespace motis::routing {



struct search_result {
  search_result() = default;
  search_result(statistics stats, std::vector<journey> journeys,
                time interval_begin, time interval_end)
      : stats_(std::move(stats)),  // NOLINT
        journeys_(std::move(journeys)),
        interval_begin_(interval_begin),
        interval_end_(interval_end) {}
  explicit search_result(unsigned travel_time_lb) : stats_(travel_time_lb) {}
  template <typename Label>
  explicit search_result(lower_bounds_result<Label> const& lb_result){
    stats_ = statistics();
    stats_.total_lb = lb_result.total_lb;
    stats_.travel_time_lb_ = lb_result.travel_time_lb_;
    stats_.price_lb_ = lb_result.price_lb_;
    stats_.optimality_lb_ = lb_result.optimality_lb_;
    stats_.transfers_lb_ = lb_result.transfers_lb_;
  }
  statistics stats_;
  std::vector<journey> journeys_;
  time interval_begin_{INVALID_TIME};
  time interval_end_{INVALID_TIME};
};

template <search_dir Dir, typename StartLabelGenerator, typename Label>
struct search {
  static search_result get_connections(search_query const& q) {

    auto const& meta_goals = q.sched_->stations_[q.to_->id_]->equivalent_;
    std::vector<int> goal_ids;
    boost::container::vector<bool> is_goal(q.sched_->stations_.size(), false);
    for (auto const& meta_goal : meta_goals) {
      goal_ids.push_back(meta_goal->index_);
      is_goal[meta_goal->index_] = true;
      if (!q.use_dest_metas_) {
        break;
      }
    }
    // TODO: What does this if statement do?
    if (q.to_ == q.sched_->station_nodes_.at(1).get()) {
      goal_ids.push_back(q.to_->id_);
      is_goal[q.to_->id_] = true;
    }

   auto lb_result = get_lower_bounds_for_query<Label>(q, goal_ids, Dir);
   lower_bounds<Label>& lbs = *lb_result.bounds_;

   if(!lb_result.target_reachable){
     return search_result(lb_result);
   }

    auto const create_start_edge = [&](node* to) {
      return Dir == search_dir::FWD ? make_foot_edge(nullptr, to)
                                    : make_foot_edge(to, nullptr);
    };
    auto mutable_node = const_cast<node*>(q.from_);  // NOLINT
    auto const start_edge = create_start_edge(mutable_node);

    std::vector<edge> meta_edges;
    if (q.from_->is_route_node() ||  // what does route node mean?
        q.from_ == q.sched_->station_nodes_.at(0).get()) {
      if (!lbs.is_valid_time_diff(
              lbs.time_from_node(q.from_))) {  // condition already checked?

        return search_result(lb_result);
      }
    } else if (!q.use_start_metas_) {
      if (!lbs.is_valid_time_diff(
              lbs.time_from_node(q.from_))) {  // same condition again?
        return search_result(lb_result);
      }
      meta_edges.push_back(start_edge);
    } else {
      auto const& meta_froms = q.sched_->stations_[q.from_->id_]->equivalent_;
      // check equivalent stations for reachability
      if (q.use_start_metas_ &&
          std::all_of(begin(meta_froms), end(meta_froms),
                      [&lbs, &q](station const* s) {
                        return !lbs.is_valid_time_diff(lbs.time_from_node(
                            q.sched_->station_nodes_[s->index_].get()));
                      })) {
        return search_result(lb_result);
      }
      for (auto const& meta_from : meta_froms) {
        auto meta_edge = create_start_edge(
            q.sched_->station_nodes_[meta_from->index_].get());
        meta_edges.push_back(meta_edge);
      }
    }

    mcd::hash_map<node const*, std::vector<edge>> additional_edges;
    for (auto const& e : q.query_edges_) {
      additional_edges[e.get_source<Dir>()].push_back(e);
    }

    pareto_dijkstra<Dir, Label, lower_bounds<Label>> pd(
        q.sched_->node_count_, q.sched_->stations_.size(), is_goal,
        std::move(additional_edges), lbs, *q.mem_);

    auto const add_start_labels = [&](time interval_begin, time interval_end) {
      pd.add_start_labels(StartLabelGenerator::generate(
          *q.sched_, *q.mem_, lbs, &start_edge, meta_edges, q.query_edges_,
          interval_begin, interval_end, q.lcon_, q.use_start_footpaths_));
    };

    time const schedule_begin = SCHEDULE_OFFSET_MINUTES;
    time const schedule_end =
        (q.sched_->schedule_end_ - q.sched_->schedule_begin_) / 60;

    auto const map_to_interval = [&schedule_begin, &schedule_end](time t) {
      return std::min(schedule_end, std::max(schedule_begin, t));
    };

    add_start_labels(q.interval_begin_, q.interval_end_);

    MOTIS_START_TIMING(pareto_dijkstra_timing);
    auto max_interval_reached = false;
    auto interval_begin = q.interval_begin_;
    auto interval_end = q.interval_end_;

    auto const departs_in_interval = [](Label const* l,
                                        motis::time interval_begin,
                                        motis::time interval_end) {
      return interval_end == INVALID_TIME ||  // ontrip
             (l->start_ >= interval_begin && l->start_ <= interval_end);
    };

    auto const number_of_results_in_interval =
        [&interval_begin, &interval_end,
         departs_in_interval](std::vector<Label*> const& labels) {
          return std::count_if(begin(labels), end(labels), [&](Label const* l) {
            return departs_in_interval(l, interval_begin, interval_end);
          });
        };

    auto search_iterations = 0UL;

    while (!max_interval_reached) {
      max_interval_reached =
          (!q.extend_interval_earlier_ || interval_begin == schedule_begin) &&
          (!q.extend_interval_later_ || interval_end == schedule_end);

      pd.search();
      ++search_iterations;

      if (number_of_results_in_interval(pd.get_results()) >=
          q.min_journey_count_) {
        break;
      }

      auto const new_interval_begin = q.extend_interval_earlier_
                                          ? map_to_interval(interval_begin - 60)
                                          : interval_begin;
      auto const new_interval_end = q.extend_interval_later_
                                        ? map_to_interval(interval_end + 60)
                                        : interval_end;

      if (interval_begin != schedule_begin) {
        add_start_labels(new_interval_begin,
                         map_to_interval(interval_begin - 1));
      }

      if (interval_end != schedule_end) {
        add_start_labels(map_to_interval(interval_end + 1), new_interval_end);
      }

      interval_begin = new_interval_begin;
      interval_end = new_interval_end;
    }
    MOTIS_STOP_TIMING(pareto_dijkstra_timing);

    statistics stats = pd.get_statistics();
    stats.travel_time_lb_ = lb_result.travel_time_lb_;
    stats.transfers_lb_ = lb_result.transfers_lb_;
    stats.price_lb_ = lb_result.price_lb_;
    stats.optimality_lb_ = lb_result.optimality_lb_;
    stats.total_lb = lb_result.total_lb;
    stats.pareto_dijkstra_ = MOTIS_TIMING_MS(pareto_dijkstra_timing);
    stats.interval_extensions_ = search_iterations - 1;
    stats.labels_popped_total_ = stats.labels_popped_ +
                                 stats.labels_equals_popped_ +
                                 stats.labels_optimals_popped_;
    stats.labels_dominated_by_results_perc_ =
        (stats.labels_dominated_by_results_ * 10000.0) /
        (stats.labels_created_);


    auto filtered = pd.get_results();
    filtered.erase(std::remove_if(begin(filtered), end(filtered),
                                  [&](Label const* l) {
                                    return !departs_in_interval(
                                        l, interval_begin, interval_end);
                                  }),
                   end(filtered));

    lower_bounds_evaluation_result evaluation = evaluate_lower_bounds(filtered);
    stats.lb_price_accuracy_ = evaluation.price_wage_evaluation * 100 * 100;
    stats.lb_price_accuracy_raw_ = evaluation.price_evaluation_ * 100 * 100;
    stats.lb_travel_time_accuracy_ =
        evaluation.travel_time_evaluation * 100 * 100;
    stats.lb_transfers_accuracy_ = evaluation.transfers_evaluation * 100 * 100;

    return search_result(stats,
                         utl::to_vec(filtered,
                                     [&q](Label* label) {
                                       return output::labels_to_journey(
                                           *q.sched_, label, Dir);
                                     }),
                         interval_begin, interval_end);
  }
};

}  // namespace motis::routing
