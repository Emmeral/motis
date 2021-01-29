#pragma once

#include "motis/core/schedule/edges.h"

namespace motis {
namespace routing {

constexpr uint16_t MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE = 4200u;
constexpr bool REGIONAL_MAX_PRICE_ENABLED_DEFAULT = false;
constexpr uint16_t MINUTELY_WAGE = 8;
constexpr uint16_t MAX_PRICE = 14000u;
constexpr uint16_t MAX_PRICE_BUCKET = (MAX_PRICE) + 1000;
constexpr uint16_t MAX_PRICE_WAGE_BUCKET =
    (MAX_PRICE + MAX_TRAVEL_TIME_MINUTES * MINUTELY_WAGE) >> 3;

constexpr bool USE_TIME_LOWER_BOUNDS = true;
constexpr bool USE_ADV_RES_DOM_MERGING = false;

struct partial_prices {
  uint16_t local_{0u};
  uint16_t regional_{0u};
  uint16_t ic_{0u};
  uint16_t ice_{0u};
  uint16_t other_{0u};
  uint16_t additional_{0u};
};

struct price {
  uint16_t time_included_price_;
  uint16_t time_included_price_lb_;
  uint16_t total_price_lb_;
  uint16_t total_price_;
  partial_prices prices_;
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

    if (USE_TIME_LOWER_BOUNDS) {
      l.time_included_price_lb_ =
          l.total_price_lb_ + lb.time_from_label(l) * MINUTELY_WAGE;
    } else {
      l.time_included_price_lb_ = l.total_price_lb_;
    }
    l.prices_ = partial_prices{};
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
          if (l.prices_.ice_ == 0) {
            l.ice_ = true;
            l.ic_ = true;
            if (l.prices_.ic_ != 0) {
              l.prices_.ice_ += 100 + con->price_;
            } else {
              l.prices_.ice_ += 700 + con->price_;
            }
          } else {
            l.prices_.ice_ += con->price_;
          }
          break;

        case service_class::IC:
          if (l.prices_.ic_ == 0) {
            l.ic_ = true;
            if (l.prices_.ice_ != 0) {
              l.prices_.ic_ += con->price_;
            } else {
              l.prices_.ic_ += 600 + con->price_;
            }
          } else {
            l.prices_.ic_ += con->price_;
          }
          break;

        case service_class::RE:
        case service_class::RB:
        case service_class::S:
          l.prices_.regional_ += con->price_;
          if (l.prices_.regional_ > MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE &&
              MAX_REGIO_ENABLED) {
            l.prices_.regional_ = MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE;
          }

          break;

        case service_class::N:
        case service_class::U:
        case service_class::STR:
        case service_class::BUS: l.prices_.local_ += con->price_; break;
        default:
          l.prices_.other_ += con->price_;
          break;
      }
    } else {
      l.prices_.additional_ += ec.price_;
    }

    const uint16_t price_sum = l.prices_.local_ + l.prices_.regional_ +
                               l.prices_.ice_ + l.prices_.ic_ +
                               l.prices_.other_;
    l.total_price_ = std::min(price_sum, MAX_PRICE) + l.prices_.additional_;

    uint16_t price_sum_lb;

    if constexpr (MAX_REGIO_ENABLED) {
      const uint16_t remaining_regio =
          MOTIS_MAX_REGIONAL_TRAIN_TICKET_PRICE - l.prices_.regional_;
      price_sum_lb =
          price_sum + std::min(lb.price_from_label(l), remaining_regio);
    } else {
      price_sum_lb = price_sum + lb.price_from_label(l);
    }

    l.total_price_lb_ =
        std::min(price_sum_lb, MAX_PRICE) + l.prices_.additional_;
    l.time_included_price_ = l.total_price_ + l.travel_time_ * MINUTELY_WAGE;
    if (USE_TIME_LOWER_BOUNDS) {
      l.time_included_price_lb_ =
          l.total_price_lb_ + l.travel_time_lb_ * MINUTELY_WAGE;
    } else {
      l.time_included_price_lb_ =
          l.total_price_lb_ + l.travel_time_ * MINUTELY_WAGE;
    }
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
                              MAX_PRICE + a.prices_.additional_);
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
      if (b.prices_.regional_ > a.prices_.regional_) {
        return (b.prices_.regional_ - a.prices_.regional_);
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
    domination_info(Label const& a, Label const& b, bool add_price = true,
                    duration merged_time_lb = 0) {

      auto add_p = 0;

      if (add_price) {
        add_p += get_additional_ice_price(a, b);
        if constexpr (MAX_REGIO_ENABLED) {
          add_p += get_additional_regio_price(a, b);
        }
      }

      auto remaining_to_max = MAX_PRICE - a.total_price_lb_;
      add_p = std::min(remaining_to_max, add_p);
      auto a_price = a.time_included_price_lb_ + add_p;

      if (add_price) {
        if (b.now_ > a.now_) {
          a_price += (b.now_ - a.now_) * MINUTELY_WAGE;
        }
      }

      uint16_t b_merged_time_included_price =
          USE_ADV_RES_DOM_MERGING
              ? b.total_price_lb_ + merged_time_lb * MINUTELY_WAGE
              : 0;
      auto b_price =
          std::max(b_merged_time_included_price, b.time_included_price_lb_);

      greater_ = a_price > b_price;
      smaller_ = a_price < b_price;
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
      if (b.prices_.regional_ > a.prices_.regional_) {
        return (b.prices_.regional_ - a.prices_.regional_);
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
      Label const& a, Label const& b, Label const& opt_result_to_merge) {
    auto dom_info = domination_info<Label>(a, b, false,
                                           opt_result_to_merge.travel_time_lb_);
    return dom_info;
  }

  typedef bool has_result_dominates;
};





}  // namespace routing
}  // namespace motis
