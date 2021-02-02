#pragma once
#include "motis/routing/label/configs.h"

namespace motis {
namespace routing {

template <typename Label>
std::pair<double, double> eval_price_lb(
    std::vector<Label*> const& /*terminal_labels*/) {

  return {0.0, 0.0};
}
template <search_dir Dir, bool MAX_REGIO>
std::pair<double, double> eval_price_lb(
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

  return {(sum_time_included / count), sum_raw / count};
}

}  // namespace routing
}  // namespace motis
