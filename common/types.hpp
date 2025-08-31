#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace ts {

enum class Side { Buy, Sell };

struct Order {
  uint64_t id;         // unique order id
  std::string client;  // client name
  Side side;           // buy or sell
  int qty;             // remaining quantity
  double px;           // limit price (ignored for market orders)
  bool is_limit;       // true for limit, false for market
};

struct Trade {
  uint64_t maker_id;   // resting order id
  uint64_t taker_id;   // incoming order id
  int qty;
  double px;
};

struct TopOfBook {
  bool has_bid{false};
  bool has_ask{false};
  double bid_px{0.0};
  int bid_qty{0};
  double ask_px{0.0};
  int ask_qty{0};
};

} // namespace ts

