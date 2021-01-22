#pragma once

#include <string>
#include <iostream>

namespace motis::routing {

enum class lower_bounds_travel_time_type { NONE, CG};
enum class lower_bounds_transfers_type { NONE, CG};
enum class lower_bounds_price_type { NONE, DISTANCE};
enum class optimality_type { NONE, CSA};

// Travel Time

inline std::istream& operator>>(std::istream& in, lower_bounds_travel_time_type& type) {
  std::string token;
  in >> token;

  if (token == "none") {
    type = lower_bounds_travel_time_type::NONE;
  } else if (token == "cg") {
    type = lower_bounds_travel_time_type::CG;
  }
  return in;
}

inline std::ostream& operator<<(std::ostream& out, lower_bounds_travel_time_type const& type) {
  switch (type) {
    case lower_bounds_travel_time_type::NONE: out << "none"; break;
    case lower_bounds_travel_time_type::CG: out << "cg"; break;
  }
  return out;
}


// Transfers

inline std::istream& operator>>(std::istream& in, lower_bounds_transfers_type& type) {
  std::string token;
  in >> token;

  if (token == "none") {
    type = lower_bounds_transfers_type::NONE;
  } else if (token == "cg") {
    type = lower_bounds_transfers_type::CG;
  }
  return in;
}

inline std::ostream& operator<<(std::ostream& out, lower_bounds_transfers_type const& type) {
  switch (type) {
    case lower_bounds_transfers_type::NONE: out << "none"; break;
    case lower_bounds_transfers_type::CG: out << "cg"; break;
  }
  return out;
}

// Price

inline std::istream& operator>>(std::istream& in, lower_bounds_price_type& type) {
  std::string token;
  in >> token;

  if (token == "none") {
    type = lower_bounds_price_type::NONE;
  } else if (token == "distance") {
    type = lower_bounds_price_type::DISTANCE;
  }
  return in;
}

inline std::ostream& operator<<(std::ostream& out, lower_bounds_price_type const& type) {
  switch (type) {
    case lower_bounds_price_type::NONE: out << "none"; break;
    case lower_bounds_price_type::DISTANCE: out << "distance"; break;
  }
  return out;
}

//Optimality

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
