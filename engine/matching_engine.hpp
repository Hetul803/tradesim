#pragma once
#include "common/types.hpp"
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace ts {

struct Order {
  uint64_t id{0};
  std::string client;
  Side side{Side::Buy};
  int qty{0};
  double px{0.0};  // for LIMITs; ignored for MARKET while matching
};

class MatchingEngine {
public:
  MatchingEngine();

  // place a resting limit order; returns any trades executed immediately
  std::vector<Trade> new_limit_order(const std::string& client, Side side, int qty, double px);

  // execute a market order against the book; returns fills
  std::vector<Trade> new_market_order(const std::string& client, Side side, int qty);

  // cancel a previously resting order by id
  bool cancel(uint64_t order_id);

  // top-of-book summary
  TopOfBook top() const;

private:
  uint64_t next_id_{1};

  // price->queue (best bid = highest price; best ask = lowest price)
  std::map<double, std::deque<Order>, std::greater<double>> bids_;
  std::map<double, std::deque<Order>, std::less<double>> asks_;

  // internal helpers
  std::vector<Trade> match_incoming(Order taker); // for both market and limit that crosses
  void add_resting(const Order& o);               // enqueue remaining qty
  static bool crosses(const Order& taker, double maker_px);
};

} // namespace ts

