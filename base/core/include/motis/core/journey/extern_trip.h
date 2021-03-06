#pragma once

#include <ctime>

#include "utl/struct/comparable.h"
#include "utl/struct/for_each_field.h"

#include "motis/string.h"

#include "motis/core/common/hash_helper.h"

namespace motis {

struct extern_trip {
  MAKE_COMPARABLE()

  mcd::string station_id_;
  uint32_t train_nr_{0};
  time_t time_{0};

  mcd::string target_station_id_;
  time_t target_time_{0};
  mcd::string line_id_;
};

}  // namespace motis