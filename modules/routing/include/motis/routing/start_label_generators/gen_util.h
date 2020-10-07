#pragma once

#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include "motis/core/schedule/schedule.h"
#include "motis/routing/error.h"
#include "motis/routing/lower_bounds/implementations/lower_bounds.h"
#include "motis/routing/mem_manager.h"

namespace motis::routing {

template <search_dir Dir, typename Fn>
void for_each_edge(node const* rn, Fn fn) {
  if (Dir == search_dir::FWD) {
    for (auto const& re : rn->edges_) {
      fn(re);
    }
  } else {
    for (auto const& re : rn->incoming_edges_) {
      fn(*re);
    }
  }
}

template <search_dir Dir>
bool end_reached(time departure_begin, time departure_end, time t) {
  if (Dir == search_dir::FWD) {
    return t > departure_end;
  } else {
    return t < departure_begin;
  }
}

template <search_dir Dir>
time get_time(light_connection const* lcon) {
  return (Dir == search_dir::FWD) ? lcon->d_time_ : lcon->a_time_;
}

template <search_dir Dir, typename Fn>
void create_labels(time departure_begin, time departure_end, edge const& re,
                   Fn create_func) {
  if (re.empty()) {
    return;
  }
  auto i = 0;
  auto t = Dir == search_dir::FWD ? departure_begin : departure_end;
  auto const max_start_labels = departure_end - departure_begin + 1;
  while (!end_reached<Dir>(departure_begin, departure_end, t)) {
    auto con = re.get_connection<Dir>(t);

    if (con == nullptr ||
        end_reached<Dir>(departure_begin, departure_end, get_time<Dir>(con))) {
      break;
    }

    t = get_time<Dir>(con);

    create_func(t);

    (Dir == search_dir::FWD) ? ++t : --t;

    if (++i > max_start_labels) {
      throw std::system_error(error::too_many_start_labels);
    }
  }
}

}  // namespace motis::routing
