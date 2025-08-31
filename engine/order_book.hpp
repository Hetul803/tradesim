#pragma once
#include "common/types.hpp"
#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

namespace ts {

// Price-time priority order book with FIFO queues per price level.
class OrderBook {
public:
  OrderBook();

  // Enqueue a limit order without matching (used for remainders).
  void add_limit(const Order& o);

  // Match a market order (BUY consumes asks; SELL consumes bids).
  // Decrements 'qty' as fills happen; returns generated trades.
  std::vector<Trade> match_market(Side side, int& qty);

  // Try to cross a limit order; if unfilled remainder remains, rest it.
  std::vector<Trade> match_or_add_limit(Order o);

  // Cancel a resting order by its id; true if found & removed.
  bool cancel(uint64_t id);

  // Best bid/ask snapshot.
  TopOfBook top() const;

private:
  using LevelQueue = std::deque<Order>;
  // Highest price first for bids; lowest price first for asks.
  std::map<double, LevelQueue, std::greater<double>> bids_;
  std::map<double, LevelQueue, std::less<double>>    asks_;

  struct Locator { Side side; double px; };
  std::unordered_map<uint64_t, Locator> loc_; // fast id -> (side, price)

  std::vector<Trade> consume_from_asks(int& qty, uint64_t taker_id);
  std::vector<Trade> consume_from_bids(int& qty, uint64_t taker_id);
};

} // namespace ts

