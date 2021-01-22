#pragma once

namespace motis::routing {

class optimality_none {
public:
  optimality_none() {}

  /**
   *
   * @param routing_query the original query for the routing request
   * @param search_direction the direction of the routing request
   * @return true if the target is reachable and the calculation was successful
   */
  bool calculate_optimality(search_query const& /* routing_query */) { return true; }

  template <typename Label>
  bool is_optimal(Label const& /*l*/) const {
    return false;
  }

  int get_optimal_journey_count() const { return 0; }
};
}  // namespace motis::routing
