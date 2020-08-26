#pragma once

#include "motis/core/schedule/edges.h"

namespace motis {
namespace routing {

enum {
  MOTIS_LOCAL_TRANSPORT_PRICE,
  MOTIS_REGIONAL_TRAIN_PRICE,
  MOTIS_IC_PRICE,
  MOTIS_ICE_PRICE,
  ADDITIONAL_PRICE
};
#define MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE (4200u)
#define MINUTELY_WAGE 8

struct price {
  uint16_t total_price_;
  uint16_t prices_[5];
  bool ic_, ice_;
};

struct price_inititalizer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds&) {
    l.total_price_ = 0;
    l.prices_[0] = 0;
    l.prices_[1] = 0;
    l.prices_[2] = 0;
    l.prices_[3] = 0;
    l.prices_[4] = 0;
    l.ic_ = false;
    l.ice_ = false;
  }
};

struct price_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds&) {
    if (ec.connection_ != nullptr) {
      connection const* con = ec.connection_->full_con_;
      switch (con->clasz_) {
        case service_class::ICE:
          if (l.prices_[MOTIS_ICE_PRICE] == 0) {
            l.ice_ = true;
            l.ic_ = true;
            if (l.prices_[MOTIS_IC_PRICE] != 0) {
              l.prices_[MOTIS_ICE_PRICE] += 100 + con->price_;
            } else {
              l.prices_[MOTIS_ICE_PRICE] += 700 + con->price_;
            }
          } else {
            l.prices_[MOTIS_ICE_PRICE] += con->price_;
          }
          break;

        case service_class::IC:
          if (l.prices_[MOTIS_IC_PRICE] == 0) {
            l.ic_ = true;
            if (l.prices_[MOTIS_ICE_PRICE] != 0) {
              l.prices_[MOTIS_IC_PRICE] += con->price_;
            } else {
              l.prices_[MOTIS_IC_PRICE] += 600 + con->price_;
            }
          } else {
            l.prices_[MOTIS_IC_PRICE] += con->price_;
          }
          break;

        case service_class::RE:
        case service_class::RB:
        case service_class::S:
          l.prices_[MOTIS_REGIONAL_TRAIN_PRICE] += con->price_;
          if (l.prices_[MOTIS_REGIONAL_TRAIN_PRICE] >
              MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE) {
            l.prices_[MOTIS_REGIONAL_TRAIN_PRICE] =
                MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE;
          }

          break;

        case service_class::N:
        case service_class::U:
        case service_class::STR:
        case service_class::BUS:
          l.prices_[MOTIS_LOCAL_TRANSPORT_PRICE] += con->price_;
          break;
        default:
          break;
          // not supported, because legacy label
      }
    } else {
      l.prices_[4] += ec.price_;
    }

    unsigned price_sum =
        l.prices_[0] + l.prices_[1] + l.prices_[2] + l.prices_[3];
    auto train_price = std::min(price_sum, 14000u);

    l.total_price_ = train_price + l.prices_[4];
    l.total_price_ = l.total_price_ + l.travel_time_ * MINUTELY_WAGE;
  }
};

struct price_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b)
        : greater_(a.total_price_ + get_additional_price(a, b) >
                   b.total_price_),
          smaller_(a.total_price_ + get_additional_price(a, b) <
                   b.total_price_) {}
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    inline uint16_t get_additional_price(Label const& a, Label const& b) {
      auto price = a.ice_ ? 0
                          : (a.ic_ ? (b.ice_ ? 100 : 0)
                                   : (b.ice_ ? 700 : (b.ic_ ? 600 : 0)));
      return price;
    }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    auto dom_info = domination_info<Label>(a, b);
    return dom_info;
  }
};

}  // namespace routing
}  // namespace motis
