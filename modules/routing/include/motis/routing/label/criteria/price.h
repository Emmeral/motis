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
constexpr uint16_t MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE = 4200u;
constexpr bool REGIONAL_MAX_PRICE_ENABLED_DEFAULT = false;
constexpr uint16_t MINUTELY_WAGE = 8;
constexpr uint16_t MAX_PRICE = 14000u;
constexpr uint16_t MAX_PRICE_BUCKET = (MAX_PRICE) + 1000;
constexpr uint16_t MAX_PRICE_WAGE_BUCKET = (MAX_PRICE + MAX_TRAVEL_TIME_MINUTES * MINUTELY_WAGE) >> 3;

struct price {
  uint16_t time_included_price_;
  uint16_t time_included_price_lb_;
  uint16_t total_price_lb_;
  uint16_t total_price_;
  uint16_t prices_[5];
  bool ic_, ice_;
};

struct get_total_price {
  template <typename Label>
  duration operator()(Label const* l) {
    return l->total_price_;
  }
};
struct get_price_bucket {
  template <typename Label>
  uint16_t operator()(Label const* l) {
    uint16_t p = (l->total_price_lb_);
    return std::min(p, MAX_PRICE_BUCKET);
  }
};
struct get_price_wage_bucket {
  template <typename Label>
  uint16_t operator()(Label const* l) {
    uint16_t p = (l->time_included_price_lb_);
    uint16_t shifted = p >> 3;
    return std::min(shifted, MAX_PRICE_WAGE_BUCKET);
  }
};

template<bool MAX_REGIO_ENABLED = REGIONAL_MAX_PRICE_ENABLED_DEFAULT>
struct price_initializer {
  template <typename Label, typename LowerBounds>
  static void init(Label& l, LowerBounds& lb) {
    l.total_price_ = 0;
    l.time_included_price_ = 0;
    if (MAX_REGIO_ENABLED) {
      l.total_price_lb_ =
          l.total_price_ + std::min(lb.price_from_label(l),
                                    MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE);
    } else {
      l.total_price_lb_ = l.total_price_ + lb.price_from_label(l);
    }
    l.time_included_price_lb_ =
        l.total_price_lb_ + lb.time_from_label(l) * MINUTELY_WAGE;
    l.prices_[0] = 0;
    l.prices_[1] = 0;
    l.prices_[2] = 0;
    l.prices_[3] = 0;
    l.prices_[4] = 0;
    l.ic_ = false;
    l.ice_ = false;
  }
};

template<bool MAX_REGIO_ENABLED = REGIONAL_MAX_PRICE_ENABLED_DEFAULT>
struct price_updater {
  template <typename Label, typename LowerBounds>
  static void update(Label& l, edge_cost const& ec, LowerBounds& lb) {
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
                  MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE &&
              MAX_REGIO_ENABLED) {
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

    const uint16_t price_sum = l.prices_[MOTIS_LOCAL_TRANSPORT_PRICE] +
                               l.prices_[MOTIS_REGIONAL_TRAIN_PRICE] +
                               l.prices_[MOTIS_ICE_PRICE] +
                               l.prices_[MOTIS_IC_PRICE];
    l.total_price_ =
        std::min(price_sum, MAX_PRICE) + l.prices_[ADDITIONAL_PRICE];

    uint16_t price_sum_lb;

    if constexpr (MAX_REGIO_ENABLED) {
      const uint16_t remaining_regio = MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE -
                                       l.prices_[MOTIS_REGIONAL_TRAIN_PRICE];
      price_sum_lb =
          price_sum + std::min(lb.price_from_label(l), remaining_regio);
    } else {
      price_sum_lb = price_sum + lb.price_from_label(l);
    }

    l.total_price_lb_ =
        std::min(price_sum_lb, MAX_PRICE) + l.prices_[ADDITIONAL_PRICE];
    l.time_included_price_ = l.total_price_ + l.travel_time_ * MINUTELY_WAGE;
    l.time_included_price_lb_ =
        l.total_price_lb_ + l.travel_time_lb_ * MINUTELY_WAGE;
  }
};

template<bool MAX_REGIO_ENABLED = REGIONAL_MAX_PRICE_ENABLED_DEFAULT>
struct price_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b, bool add_price = true) {

      auto add_p = 0;

      if (add_price) {
        add_p += get_additional_ice_price(a, b);
        if constexpr (MAX_REGIO_ENABLED) {
          add_p += get_additional_regio_price(a, b);
        }
      }

      auto a_price = std::min(a.total_price_lb_ + add_p,
                              MAX_PRICE + a.prices_[ADDITIONAL_PRICE]);
      greater_ = a_price > b.total_price_lb_;
      smaller_ = a_price < b.total_price_lb_;
    }
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    inline uint16_t get_additional_ice_price(Label const& a, Label const& b) {

      if (a.ice_) {
        return 0;
      }
      if (b.ice_) {
        if (a.ic_) {
          return 100;
        } else {
          return 700;
        }
      }
      if (b.ic_ && !a.ic_) {
        return 600;
      }

      return 0;
    }
    inline uint16_t get_additional_regio_price(Label const& a, Label const& b) {
      // if the max regio price is used then the other label might have an
      // advantage if it is already at a higher price
      if (b.prices_[MOTIS_REGIONAL_TRAIN_PRICE] >
          a.prices_[MOTIS_REGIONAL_TRAIN_PRICE]) {
        return (b.prices_[MOTIS_REGIONAL_TRAIN_PRICE] -
                a.prices_[MOTIS_REGIONAL_TRAIN_PRICE]);
      }
      return 0;
    }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    auto dom_info = domination_info<Label>(a, b);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(Label const& a,
                                                 Label const& b) {
    auto dom_info = domination_info<Label>(a, b, false);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(
      Label const& a, Label const& b, Label const& /* opt_result_to_merge */) {
    return result_dominates(a, b);
  }

  typedef bool has_result_dominates;
};

template<bool MAX_REGIO_ENABLED = REGIONAL_MAX_PRICE_ENABLED_DEFAULT>
struct price_wage_dominance {
  template <typename Label>
  struct domination_info {
    domination_info(Label const& a, Label const& b, bool add_price = true) {

      auto add_p = 0;

      if (add_price) {
        add_p += get_additional_ice_price(a, b);
        if constexpr (MAX_REGIO_ENABLED) {
          add_p += get_additional_regio_price(a, b);
        }
      }

      add_p = std::min(MAX_PRICE - a.total_price_lb_, add_p);
      auto a_price = a.time_included_price_lb_ + add_p;

      if(add_price){
        if (b.now_ > a.now_) {
          a_price += (b.now_ - a.now_) * MINUTELY_WAGE;
        }
      }

      greater_ = a_price > b.time_included_price_lb_;
      smaller_ = a_price < b.time_included_price_lb_;
    }
    inline bool greater() const { return greater_; }
    inline bool smaller() const { return smaller_; }
    inline uint16_t get_additional_ice_price(Label const& a, Label const& b) {

      if (a.ice_) {
        return 0;
      }
      if (b.ice_) {
        if (a.ic_) {
          return 100;
        } else {
          return 700;
        }
      }
      if (b.ic_ && !a.ic_) {
        return 600;
      }

      return 0;
    }
    inline uint16_t get_additional_regio_price(Label const& a, Label const& b) {
      // if the max regio price is used then the other label might have an
      // advantage if it is already at a higher price
      if (b.prices_[MOTIS_REGIONAL_TRAIN_PRICE] >
          a.prices_[MOTIS_REGIONAL_TRAIN_PRICE]) {
        return (b.prices_[MOTIS_REGIONAL_TRAIN_PRICE] -
                a.prices_[MOTIS_REGIONAL_TRAIN_PRICE]);
      }
      return 0;
    }
    bool greater_, smaller_;
  };

  template <typename Label>
  static domination_info<Label> dominates(Label const& a, Label const& b) {
    auto dom_info = domination_info<Label>(a, b);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(Label const& a,
                                                 Label const& b) {
    auto dom_info = domination_info<Label>(a, b, false);
    return dom_info;
  }
  template <typename Label>
  static domination_info<Label> result_dominates(
      Label const& a, Label const& b, Label const& /* opt_result_to_merge */) {
    return result_dominates(a, b);
  }

  typedef bool has_result_dominates;
};





}  // namespace routing
}  // namespace motis
