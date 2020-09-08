#include "motis/routing/lower_bounds/lower_bounds_csa.h"
#include "motis/routing/label/configs.h"

namespace motis::routing {

// specializations for labels without time keeping
/**
template <>
time_diff_t
lower_bounds_csa<single_criterion_label<search_dir::FWD>>::time_from_label(
    single_criterion_label<search_dir::FWD>& l) const {
  return time_from_node(l.get_node());
}

template <>
time_diff_t
lower_bounds_csa<single_criterion_label<search_dir::BWD>>::time_from_label(
    single_criterion_label<search_dir::BWD>& l) const {
  return time_from_node(l.get_node());
}

template <>
time_diff_t
lower_bounds_csa<single_criterion_no_intercity_label<search_dir::FWD>>::
    time_from_label(
        single_criterion_no_intercity_label<search_dir::FWD>& l) const {
  return time_from_node(l.get_node());
}

template <>
time_diff_t
lower_bounds_csa<single_criterion_no_intercity_label<search_dir::BWD>>::
    time_from_label(
        single_criterion_no_intercity_label<search_dir::BWD>& l) const {
  return time_from_node(l.get_node());
}
**/
}  // namespace motis::routing
