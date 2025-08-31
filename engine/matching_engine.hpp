#pragma once
#include "common/types.hpp"
#include "engine/order_book.hpp"
#include <atomic>
#include <vector>

namespace ts {

class MatchingEngine {
public:
  MatchingEngine();

  // Returns vector of trades generated.
  std::vector<Trade> new_limit_order(const std::string& client, Side side, int qty, double px);

  // Market order ignores price and matches until qty is filled or book is empty.
  std::vector<Trade> new_market_order(const std::string& client, Side side, int qty);

  bool cancel(uint64_t order_id);

  TopOfBook top() const { return book_.top(); }

private:
  OrderBook book_;
  std::atomic<uint64_t> next_id_{1};
};

} // namespace ts

