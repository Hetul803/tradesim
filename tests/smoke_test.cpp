#include "engine/matching_engine.hpp"
#include <cassert>
#include <iostream>

using namespace ts;

int main() {
  MatchingEngine eng;

  // Resting asks
  eng.new_limit_order("asker1", Side::Sell, 50, 10.20);
  eng.new_limit_order("asker2", Side::Sell, 50, 10.25);

  // Market buy 80 -> 50 @10.20 + 30 @10.25
  auto t1 = eng.new_market_order("buyer1", Side::Buy, 80);
  int qty_sum = 0; for (auto& t : t1) qty_sum += t.qty;
  assert(qty_sum == 80);
  assert(t1.size() == 2);

  // Top should be ask 20 @ 10.25
  auto top = eng.top();
  assert(top.has_ask && top.ask_px == 10.25 && top.ask_qty == 20);

  // Add bid, then a crossing sell
  eng.new_limit_order("bidder", Side::Buy, 100, 10.30);
  auto t2 = eng.new_limit_order("seller", Side::Sell, 25, 10.00);
  qty_sum = 0; for (auto& t : t2) qty_sum += t.qty;
  assert(qty_sum == 25);

  auto top2 = eng.top();
  assert(top2.has_bid && top2.bid_px == 10.30);

  std::cout << "SMOKE TEST PASSED\n";
  return 0;
}

