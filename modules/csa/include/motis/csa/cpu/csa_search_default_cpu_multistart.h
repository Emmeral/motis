#pragma once

#include <algorithm>
#include <array>
#include <iostream>
#include <map>

#include "utl/erase_if.h"

#include "motis/core/common/logging.h"

#include "motis/csa/csa_journey.h"
#include "motis/csa/csa_reconstruction.h"
#include "motis/csa/csa_search_shared.h"
#include "motis/csa/csa_statistics.h"
#include "motis/csa/csa_timetable.h"

namespace motis::csa::cpu::multistart {

struct search_start_point {
  csa_station station;
  time start_time{};
};

template <search_dir Dir>
struct csa_search {
  static constexpr auto INVALID = Dir == search_dir::FWD
                                      ? std::numeric_limits<time>::max()
                                      : std::numeric_limits<time>::min();
  static constexpr auto ZERO = Dir == search_dir::FWD
                                   ? std::numeric_limits<time>::min()
                                   : std::numeric_limits<time>::max();

  csa_search(csa_timetable const& tt,
             std::vector<search_start_point> const& starts)
      : tt_(tt),
        starts_(starts),
        time_limit_(),
        best_arrival_time(
            tt.stations_.size(),
            array_maker<time, MAX_TRANSFERS + 1>::make_array(INVALID)),
        needed_time_(tt.stations_.size(),
                     array_maker<time, MAX_TRANSFERS + 1>::make_array(
                         std::numeric_limits<time>::max())),
        arrival_time_(
            tt.stations_.size(),
            std::vector<std::array<time, MAX_TRANSFERS + 1>>(
                starts.size(),
                array_maker<time, MAX_TRANSFERS + 1>::make_array(INVALID))),
        trip_reachable_(
            tt.trip_count_,
            std::vector<std::array<bool, MAX_TRANSFERS + 1>>(starts.size())) {

    first_start_time_ = INVALID;
    // initialize arrival times
    for (int i = 0; i < starts.size(); ++i) {
      auto const& start = starts[i];
      arrival_time_[start.station.id_][i][0] = start.start_time;

      expand_footpaths(start.station, start.start_time, 0, i);

      time limit;
      // set the start times accordingly
      if (Dir == search_dir::FWD) {
        if (first_start_time_ > start.start_time) {
          first_start_time_ = start.start_time;
        }
        limit = start.start_time + MAX_TRAVEL_TIME;
      } else {
        if (first_start_time_ < start.start_time) {
          first_start_time_ = start.start_time;
        }
        limit = start.start_time - MAX_TRAVEL_TIME;
      }
      time_limit_.push_back(limit);
    }
  }

  void search() {
    auto const& connections =
        Dir == search_dir::FWD ? tt_.fwd_connections_ : tt_.bwd_connections_;

    csa_connection const start_at{first_start_time_};
    auto const first_connection = std::lower_bound(
        begin(connections), end(connections), start_at,
        [&](csa_connection const& a, csa_connection const& b) {
          return Dir == search_dir::FWD ? a.departure_ < b.departure_
                                        : a.arrival_ > b.arrival_;
        });
    if (first_connection == end(connections)) {
      return;
    }

    std::vector<bool> time_limit_reached(starts_.size(), false);

    for (auto it = first_connection; it != end(connections); ++it) {
      auto const& con = *it;

      bool all_limits_reached = true;
      for (auto start_id = 0; start_id < starts_.size(); ++start_id) {
        time_limit_reached[start_id] = time_limit_reached[start_id] ||
            (Dir == search_dir::FWD ? con.departure_ > time_limit_[start_id]
                                    : con.arrival_ < time_limit_[start_id]);
        all_limits_reached = all_limits_reached && time_limit_reached[start_id];
      }
      if (all_limits_reached) {
        break;
      }

      for (auto start_id = 0; start_id < starts_.size(); ++start_id) {

        if (time_limit_reached[start_id]) {
          continue;
        }

        auto& trip_reachable = trip_reachable_[con.trip_][start_id];
        auto const& from_arrival_time =
            arrival_time_[con.from_station_][start_id];
        auto const& to_arrival_time = arrival_time_[con.to_station_][start_id];

        for (auto transfers = 0; transfers < MAX_TRANSFERS; ++transfers) {
          auto const via_trip = trip_reachable[transfers];  // NOLINT
          auto const via_station =
              Dir == search_dir::FWD
                  ? (from_arrival_time[transfers] <= con.departure_  // NOLINT
                     && con.from_in_allowed_)
                  : (to_arrival_time[transfers] >= con.arrival_ &&  // NOLINT
                     con.to_out_allowed_);
          if (via_trip || via_station) {
            trip_reachable[transfers] = true;  // NOLINT
            auto const update =
                Dir == search_dir::FWD
                    ? con.arrival_ <
                              to_arrival_time[transfers + 1] &&  // NOLINT
                          con.to_out_allowed_
                    : (con.departure_ >=
                       from_arrival_time[transfers + 1]) &&  // NOLINT
                          con.from_in_allowed_;
            if (update) {
              if (Dir == search_dir::FWD) {
                expand_footpaths(tt_.stations_[con.to_station_], con.arrival_,
                                 transfers + 1, start_id);
              } else {
                expand_footpaths(tt_.stations_[con.from_station_],
                                 con.departure_, transfers + 1, start_id);
              }
            }
          }
        }
      }
    }
  }

  void expand_footpaths(csa_station const& station, time station_arrival,
                        int transfers, int start_id) {
    if (Dir == search_dir::FWD) {
      for (auto const& fp : station.footpaths_) {
        auto const fp_arrival = station_arrival + fp.duration_;
        auto time_difference = fp_arrival - starts_[start_id].start_time;

        // if the arrival is not relevant for further search.
        if (fp_arrival > best_arrival_time[fp.to_station_][transfers] &&
            time_difference > needed_time_[fp.to_station_][transfers]) {
          continue;
        }

        if (arrival_time_[fp.to_station_][start_id][transfers] > fp_arrival) {
          arrival_time_[fp.to_station_][start_id][transfers] = fp_arrival;
          if (needed_time_[fp.to_station_][transfers] > time_difference) {
            needed_time_[fp.to_station_][transfers] = time_difference;
            best_arrival_time[fp.to_station_][transfers] = fp_arrival;
          }
        }
      }
    } else {
      for (auto const& fp : station.incoming_footpaths_) {
        auto const fp_arrival = station_arrival - fp.duration_;
        auto time_difference = starts_[start_id].start_time - fp_arrival;


        // if the arrival is not relevant for further search.
        if (fp_arrival < best_arrival_time[fp.to_station_][transfers] &&
            time_difference > needed_time_[fp.to_station_][transfers]) {
          continue;
        }


        if (arrival_time_[fp.to_station_][start_id][transfers] < fp_arrival) {
          arrival_time_[fp.to_station_][start_id][transfers] = fp_arrival;
          if (needed_time_[fp.to_station_][transfers] > time_difference) {
            needed_time_[fp.to_station_][transfers] = time_difference;
            best_arrival_time[fp.to_station_][transfers] = fp_arrival;
          }
        }
      }
    }
  }

  csa_timetable const& tt_;
  time first_start_time_;
  std::vector<search_start_point> const& starts_;
  std::vector<time> time_limit_;
  std::vector<std::array<time, MAX_TRANSFERS + 1>> best_arrival_time;
  std::vector<std::array<time, MAX_TRANSFERS + 1>> needed_time_;
  std::vector<std::vector<std::array<time, MAX_TRANSFERS + 1>>> arrival_time_;
  std::vector<std::vector<std::array<bool, MAX_TRANSFERS + 1>>> trip_reachable_;
};

}  // namespace motis::csa::cpu::multistart
