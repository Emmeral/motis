#pragma once

#include <string>
#include <iostream>

namespace motis::routing {

enum class lower_bounds_type { NONE, CG, CSA, MIXED };

inline std::istream& operator>>(std::istream& in, lower_bounds_type& type) {
  std::string token;
  in >> token;

  if (token == "none") {
    type = lower_bounds_type::NONE;
  } else if (token == "cg") {
    type = lower_bounds_type::CG;
  } else if (token == "csa") {
    type = lower_bounds_type::CSA;
  } else if (token == "mixed") {
    type = lower_bounds_type::MIXED;
  }
  return in;
}

inline std::ostream& operator<<(std::ostream& out, lower_bounds_type const& type) {
  switch (type) {
    case lower_bounds_type::NONE: out << "none"; break;
    case lower_bounds_type::CG: out << "cg"; break;
    case lower_bounds_type::CSA: out << "csa"; break;
    case lower_bounds_type::MIXED: out << "mixed"; break;
  }
  return out;
}
}  // namespace motis::routing
