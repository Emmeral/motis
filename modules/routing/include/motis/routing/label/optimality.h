#pragma once

namespace motis::routing {

template <typename... Traits>
struct optimality;

template <typename FirstOptimality, typename... RestOptimality>
struct optimality<FirstOptimality, RestOptimality...> {
  template <typename Label>
  static bool is_on_optimal_journey(Label const& l) {
    return FirstOptimality::is_on_optimal_journey(l) ||
           optimality<RestOptimality...>::is_on_optimal_journey(l);
  }
  /**
   * Sets all optimal flags of the old label to the flags of the new label. Can
   * be used if the old Label was dominated by the new Label which means the new
   * label must be optimal in the same criteria as the old one (if the labels
   * are at the same node)
   */
  template <typename Label>
  static void transfer_optimality(Label const& old_l, Label& new_l) {
    FirstOptimality::transfer_optimality(new_l, old_l);
    optimality<RestOptimality...>::transfer_optimality(old_l, new_l);
  }
};

template <>
struct optimality<> {
  template <typename Label>
  static bool is_on_optimal_journey(Label const&) {
    return false;
  }
  template <typename Label>
  static void transfer_optimality(Label const& /*old_l*/, Label& /*new_l*/) {}
};


}  // namespace motis::routing
