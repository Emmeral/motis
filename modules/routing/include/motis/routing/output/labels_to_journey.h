#pragma once

#include <numeric>

#include "motis/core/access/time_access.h"
#include "motis/core/journey/journey.h"
#include "motis/core/journey/journey_util.h"

#include "motis/routing/label/configs.h"
#include "motis/routing/output/label_chain_parser.h"
#include "motis/routing/output/stop.h"
#include "motis/routing/output/to_journey.h"
#include "motis/routing/output/transport.h"
#include "motis/routing/output/walks.h"

namespace motis {

struct schedule;

namespace routing::output {

template <typename Label>
inline std::vector<unsigned> criteria_data(Label const&) {
  return {0, 0, 0, 0, 0, 0};
}

template <search_dir Dir>
inline std::vector<unsigned> criteria_data(
    price_transfer_classes_label<Dir> const& l) {
  return {l.total_price_, l.tclass_max_, l.tclass_sum_, 0, 0, 0};
}
template <search_dir Dir>
inline std::vector<unsigned> criteria_data(price_label<Dir> const& l) {
  return {l.total_price_, 0, 0, 0, 0, 0};
}

template <typename Label>
inline unsigned db_costs(Label const&) {
  return 0;
}

template <search_dir Dir>
inline unsigned db_costs(late_connections_label<Dir> const& l) {
  return l.db_costs_;
}

template <search_dir Dir>
inline unsigned db_costs(late_connections_label_for_tests<Dir> const& l) {
  return l.db_costs_;
}

template <typename Label>
inline unsigned night_penalty(Label const&) {
  return 0;
}

template <search_dir Dir>
inline unsigned night_penalty(late_connections_label<Dir> const& l) {
  return l.night_penalty_;
}

template <search_dir Dir>
inline unsigned night_penalty(late_connections_label_for_tests<Dir> const& l) {
  return l.night_penalty_;
}

template <typename Label>
journey labels_to_journey(schedule const& sched, Label* label,
                          search_dir const dir) {
  auto parsed = parse_label_chain(sched, label, dir);
  std::vector<intermediate::stop>& s = parsed.first;
  std::vector<intermediate::transport> const& t = parsed.second;
  update_walk_times(s, t);

  journey j;
  j.stops_ = generate_journey_stops(s, sched);
  j.transports_ = generate_journey_transports(t, sched);
  j.trips_ = generate_journey_trips(t, sched);
  j.attributes_ = generate_journey_attributes(t);

  j.duration_ = label->now_ > label->start_ ? label->now_ - label->start_
                                            : label->start_ - label->now_;
  j.transfers_ =
      std::accumulate(begin(j.stops_), end(j.stops_), 0,
                      [](int transfers_count, journey::stop const& s) {
                        return s.exit_ ? transfers_count + 1 : transfers_count;
                      });
  j.accessibility_ = get_accessibility(j);

  j.db_costs_ = db_costs(*label);
  j.night_penalty_ = night_penalty(*label);

  std::vector<unsigned> criteria = criteria_data(*label);
  j.price_ = criteria[0];
  j.transfer_class_max_ = criteria[1];
  j.transfer_class_sum_ = criteria[2];
  j.occupancy_max_ = criteria[4];
  j.occupancy_ = criteria[3];
  // j.temp_attr_amd_ = criteria[5];

  return j;
}

}  // namespace routing::output
}  // namespace motis
