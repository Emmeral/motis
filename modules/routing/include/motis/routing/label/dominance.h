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
struct dominance;

template <typename TieBreaker, typename FirstDominator,
          typename... RestDominators>
struct dominance<TieBreaker, FirstDominator, RestDominators...> {
  template <typename Label>
  static bool dominates(bool could_dominate, Label const& a, Label const& b) {
    auto d = FirstDominator::dominates(a, b);
    return !d.greater() && dominance<TieBreaker, RestDominators...>::dominates(
                               could_dominate || d.smaller(), a, b);
  }

  /**
   * @tparam MERGE true if optimal_result_to_merge != nullptr
   */
  template <typename Label, bool MERGE = false>
  static bool result_dominates(bool could_dominate, Label const& result,
                               Label const& label,
                               Label const* optimal_result_to_merge = nullptr) {

    bool greater;
    bool smaller;
    if constexpr (has_result_dominates<FirstDominator>::value) {
      auto d = MERGE
                   ? FirstDominator::result_dominates(result, label,
                                                      *optimal_result_to_merge)
                   : FirstDominator::result_dominates(result, label);

      greater = d.greater();
      smaller = d.smaller();
    } else {
      auto d = FirstDominator::dominates(result, label);
      greater = d.greater();
      smaller = d.smaller();
    }

    return !greater &&
           dominance<TieBreaker, RestDominators...>::template result_dominates<Label, MERGE>(
               could_dominate || smaller, result, label,
               optimal_result_to_merge);
  }
};

template <typename TieBreaker>
struct dominance<TieBreaker> {
  template <typename Label>
  static bool dominates(bool could_dominate, Label const& a, Label const& b) {
    return TieBreaker::dominates(could_dominate, a, b);
  }
  template <typename Label, bool MERGE = false>
  static bool result_dominates(bool could_dominate, Label const& result,
                               Label const& label, Label const*) {
    return TieBreaker::dominates(could_dominate, result, label);
  }
};

}  // namespace motis::routing
