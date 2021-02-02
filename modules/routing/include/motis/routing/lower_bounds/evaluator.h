#pragma once
#include "motis/routing/label/configs.h"

namespace motis {
namespace routing {

struct lower_bounds_evaluation_result {
  double travel_time_evaluation;
  double transfers_evaluation;
  double price_evaluation_;
  double price_wage_evaluation;
};

template <typename Label>
lower_bounds_evaluation_result eval_price_lb(
    std::vector<Label*> const& /*terminal_labels*/) {

  return {0.0, 0.0, 0.0, 0.0};
}
template <search_dir Dir, bool MAX_REGIO>
lower_bounds_evaluation_result eval_price_lb(
    std::vector<price_wage_label<Dir, MAX_REGIO>*> const& terminal_labels) {

  double sum_raw{0};
  double sum_time_included{0};
  double count{};

  for (auto* term : terminal_labels) {

    double terminal_price_time = term->time_included_price_;
    double terminal_price = term->total_price_;

    auto c = term;
    while ((c = c->pred_)) {
      double current_price_time = c->time_included_price_;
      double lb_time = c->time_included_price_lb_ - current_price_time;
      double remaining_price_time = terminal_price_time - current_price_time;

      assert(lb_time <= remaining_price_time);
      if (remaining_price_time != 0) {
        sum_time_included += lb_time / remaining_price_time;
      } else {
        sum_time_included += 1.0;
      }

      double current_price = c->total_price_;
      double remaining_price = (terminal_price - current_price);
      double lb = c->total_price_lb_ - current_price;

      assert(lb <= remaining_price);
      if (remaining_price != 0) {
        sum_raw += lb / remaining_price;
      } else {
        sum_raw += 1;
      }

      ++count;
    }
  }

  return {0, 0, sum_raw / count, (sum_time_included / count)};
}

template <typename Label>
lower_bounds_evaluation_result evaluate_lower_bounds(
    std::vector<Label*> const& terminal_labels) {

  double sum_tt{}, sum_trans{};
  uint64_t count{};

  for (auto* term : terminal_labels) {

    double terminal_tt = term->travel_time_;
    double terminal_trans = term->transfers_;

    auto c = term;
    while ((c = c->pred_)) {
      double current_tt = c->travel_time_;
      double lb_tt = c->travel_time_lb_ - current_tt;
      double remaining_tt = terminal_tt - current_tt;

      assert(lb_tt <= remaining_tt);
      if (remaining_tt != 0) {
        sum_tt += lb_tt / remaining_tt;
      } else {
        sum_tt += 1.0;
      }

      double current_trans = c->transfers_;
      double remaining_trans = (terminal_trans - current_trans);
      double lb_trans = c->transfers_lb_ - current_trans;

      assert(lb_trans <= remaining_trans);
      if (remaining_trans != 0) {
        sum_trans += lb_trans / remaining_trans;
      } else {
        sum_trans += 1;
      }

      ++count;
    }
  }

  lower_bounds_evaluation_result price_eval = eval_price_lb(terminal_labels);

  price_eval.transfers_evaluation = sum_trans / count;
  price_eval.travel_time_evaluation = sum_tt / count;

  return price_eval;
}

template <search_dir Dir>
lower_bounds_evaluation_result evaluate_lower_bounds(
    std::vector<single_criterion_label<Dir>*> const& /*terminal_labels*/) {
  return {0, 0, 0, 0};
}
template <search_dir Dir>
lower_bounds_evaluation_result evaluate_lower_bounds(
    std::vector<
        single_criterion_no_intercity_label<Dir>*> const& /*terminal_labels*/) {
  return {0, 0, 0, 0};
}

}  // namespace routing
}  // namespace motis
