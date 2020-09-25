#pragma once

namespace motis::routing {

template <typename... Traits>
struct optimality;

template <typename FirstOptimality, typename... RestOptimality>
struct optimality<FirstOptimality, RestOptimality...> {
  template <typename Label>
  static bool is_on_optimal_journey(Label& l) {
    return FirstOptimality::is_on_optimal_journey(l) ||
           optimality<RestOptimality...>::is_on_optimal_journey(l);
  }
};

template <>
struct optimality<> {
  template <typename Label>
  static bool is_on_optimal_journey(Label&) {
    return false;
  }
};


}  // namespace motis::routing
