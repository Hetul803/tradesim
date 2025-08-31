#include "engine/matching_engine.hpp"

namespace ts {

MatchingEngine::MatchingEngine() {}

std::vector<Trade> MatchingEngine::new_limit_order(const std::string& client, Side side, int qty, double px) {
  Order o;
  o.id = next_id_.fetch_add(1);
  o.client = client;
  o.side = side;
  o.qty = qty;
  o.px = px;
  o.is_limit = true;
  return book_.match_or_add_limit(o);
}

std::vector<Trade> MatchingEngine::new_market_order(const std::string& client, Side side, int qty) {
  // Assign a taker id for audit; annotate all generated trades with it.
  const uint64_t taker_id = next_id_.fetch_add(1);
  int remaining = qty;
  std::vector<Trade> trades;
  if (side == Side::Buy) {
    auto t = book_.match_market(Side::Buy, remaining);
    for (auto& tr : t) tr.taker_id = taker_id;
    trades.insert(trades.end(), t.begin(), t.end());
  } else {
    auto t = book_.match_market(Side::Sell, remaining);
    for (auto& tr : t) tr.taker_id = taker_id;
    trades.insert(trades.end(), t.begin(), t.end());
  }
  (void)client; // placeholder for future per-client risk/accounting
  return trades;
}

bool MatchingEngine::cancel(uint64_t order_id) {
  return book_.cancel(order_id);
}

} // namespace ts

