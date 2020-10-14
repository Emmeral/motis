#pragma once

#include <string>
#include <iostream>

namespace motis::routing {

enum class optimality_type { NONE, CSA};

inline std::istream& operator>>(std::istream& in, optimality_type& type) {
  std::string token;
  in >> token;

  if (token == "none") {
    type = optimality_type::NONE;
  } else if (token == "csa") {
    type = optimality_type::CSA;
  }
  return in;
}

inline std::ostream& operator<<(std::ostream& out, optimality_type const& type) {
  switch (type) {
    case optimality_type::NONE: out << "none"; break;
    case optimality_type::CSA: out << "csa"; break;
  }
  return out;
}
}  // namespace motis::routing
