#pragma once
#include <type_traits>

namespace motis::routing {

// see https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
template <typename... Ts>
using void_t = void;

template <typename Dominator, typename = void>
struct has_result_dominates : std::false_type {};

template <typename Dominator>
struct has_result_dominates<Dominator,
                            void_t<typename Dominator::has_result_dominates>>
    : std::true_type {};

template <typename TieBreaker, typename... Dominators>
struct result_dominance;

template <typename TieBreaker, typename FirstDominator,
          typename... RestDominators>
struct result_dominance<TieBreaker, FirstDominator, RestDominators...> {
  template <typename Label>
  static bool result_dominates(bool could_dominate, Label const& result,
                        Label const& label,
                        Label const& optimal_result_to_merge) {

    bool greater;
    bool smaller;
    if constexpr (has_result_dominates<FirstDominator>::value) {
      auto d = FirstDominator::result_dominates(result, label,
                                                optimal_result_to_merge);
      greater = d.greater();
      smaller = d.smaller();
    } else {
      auto d = FirstDominator::dominates(result, label);
      greater = d.greater();
      smaller = d.smaller();
    }

    return !greater &&
           result_dominance<TieBreaker, RestDominators...>::result_dominates(
               could_dominate || smaller, result, label,
               optimal_result_to_merge);
  }
};

template <typename TieBreaker>
struct result_dominance<TieBreaker> {
  template <typename Label>
  static bool result_dominates(bool could_dominate, Label const& result,
                        Label const& label, Label const&) {
    return TieBreaker::dominates(could_dominate, result, label);
  }
};

}  // namespace motis::routing
