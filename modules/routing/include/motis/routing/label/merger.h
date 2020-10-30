#pragma once

#pragma once

namespace motis::routing {

template <typename... Traits>
struct merger {};

template <typename FirstMerger, typename... RestMerger>
struct merger<FirstMerger, RestMerger...> {

  template <typename Label>
  static Label merge_with_optimal_result(Label const& original_label,
                                         Label const& result_label) {
    Label blank = original_label;
    merge_with_optimal_result(original_label, result_label, blank);
    return blank;
  }

  template <typename Label>
  static void merge_with_optimal_result(Label const& original_label,
                                        Label const& result_label,
                                        Label& merged_label) {
    FirstMerger::merge_with_optimal_result(original_label, result_label,
                                           merged_label);
    merger<RestMerger...>::merge_with_optimal_result(
        original_label, result_label, merged_label);
  }
};

template <>
struct merger<> {

  template <typename Label>
  static Label merge_with_optimal_result(Label const& original_label,
                                         Label const& result_label) {
    Label blank = original_label;
    merge_with_optimal_result(original_label, result_label, blank);
    return blank;
  }

  template <typename Label>
  static void merge_with_optimal_result(Label const&, Label const&, Label&) {}
};

}  // namespace motis::routing
