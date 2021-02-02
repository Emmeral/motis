#pragma once
#include "motis/routing/label/configs.h"

namespace motis {
namespace routing {

template <typename Label>
double eval_price_lb(std::vector<Label*> const& /*terminal_labels*/) {

  return 0.0;
}
template <search_dir Dir, bool MAX_REGIO>
double eval_price_lb(
    std::vector<price_wage_label<Dir, MAX_REGIO>*> const& terminal_labels) {

  double sum = 0;
  double count = 0;

  for (auto* term : terminal_labels) {

    double terminal_price = term->time_included_price_;

    auto c = term;
    while ((c = c->pred_)) {
      double lb = c->time_included_price_lb_;
      double percentage = lb / terminal_price;
      sum += percentage;
      ++count;
    }
  }

  return (sum / count);
}

}  // namespace routing
}  // namespace motis
