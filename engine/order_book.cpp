#include "engine/order_book.hpp"
#include <algorithm>

namespace ts {

OrderBook::OrderBook() {}

void OrderBook::add_limit(const Order& o) {
  if (o.side == Side::Buy) {
    auto& q = bids_[o.px];
    q.push_back(o);
    loc_[o.id] = Locator{Side::Buy, o.px};
  } else {
    auto& q = asks_[o.px];
    q.push_back(o);
    loc_[o.id] = Locator{Side::Sell, o.px};
  }
}

std::vector<Trade> OrderBook::consume_from_asks(int& qty, uint64_t taker_id) {
  std::vector<Trade> trades;
  while (qty > 0 && !asks_.empty()) {
    auto it = asks_.begin();           // best (lowest) ask price
    auto& q = it->second;              // FIFO queue at that price
    while (qty > 0 && !q.empty()) {
      Order& maker = q.front();
      const int fill = std::min(qty, maker.qty);
      maker.qty -= fill;
      qty       -= fill;
      trades.push_back(Trade{maker.id, taker_id, fill, it->first});
      if (maker.qty == 0) {
        loc_.erase(maker.id);
        q.pop_front();
      }
    }
    if (q.empty()) asks_.erase(it);
  }
  return trades;
}

std::vector<Trade> OrderBook::consume_from_bids(int& qty, uint64_t taker_id) {
  std::vector<Trade> trades;
  while (qty > 0 && !bids_.empty()) {
    auto it = bids_.begin();           // best (highest) bid price
    auto& q = it->second;              // FIFO queue at that price
    while (qty > 0 && !q.empty()) {
      Order& maker = q.front();
      const int fill = std::min(qty, maker.qty);
      maker.qty -= fill;
      qty       -= fill;
      trades.push_back(Trade{maker.id, taker_id, fill, it->first});
      if (maker.qty == 0) {
        loc_.erase(maker.id);
        q.pop_front();
      }
    }
    if (q.empty()) bids_.erase(it);
  }
  return trades;
}

std::vector<Trade> OrderBook::match_market(Side side, int& qty) {
  if (qty <= 0) return {};
  if (side == Side::Buy)  return consume_from_asks(qty, /*taker_id*/ 0);
  else                    return consume_from_bids(qty, /*taker_id*/ 0);
}

std::vector<Trade> OrderBook::match_or_add_limit(Order o) {
  std::vector<Trade> trades;
  int& qty = o.qty;

  if (o.side == Side::Buy) {
    // Cross asks at prices <= o.px
    while (qty > 0 && !asks_.empty()) {
      auto it = asks_.begin();
      const double ask_px = it->first;
      if (ask_px > o.px) break; // no more crossing
      auto& q = it->second;
      while (qty > 0 && !q.empty()) {
        Order& maker = q.front();
        const int fill = std::min(qty, maker.qty);
        maker.qty -= fill;
        qty       -= fill;
        trades.push_back(Trade{maker.id, o.id, fill, ask_px});
        if (maker.qty == 0) { loc_.erase(maker.id); q.pop_front(); }
      }
      if (q.empty()) asks_.erase(it);
    }
    if (qty > 0) add_limit(o);
  } else {
    // Cross bids at prices >= o.px
    while (qty > 0 && !bids_.empty()) {
      auto it = bids_.begin();
      const double bid_px = it->first;
      if (bid_px < o.px) break; // no more crossing
      auto& q = it->second;
      while (qty > 0 && !q.empty()) {
        Order& maker = q.front();
        const int fill = std::min(qty, maker.qty);
        maker.qty -= fill;
        qty       -= fill;
        trades.push_back(Trade{maker.id, o.id, fill, bid_px});
        if (maker.qty == 0) { loc_.erase(maker.id); q.pop_front(); }
      }
      if (q.empty()) bids_.erase(it);
    }
    if (qty > 0) add_limit(o);
  }
  return trades;
}

bool OrderBook::cancel(uint64_t id) {
  auto it = loc_.find(id);
  if (it == loc_.end()) return false;
  const auto loc = it->second;

  if (loc.side == Side::Buy) {
    auto lvl = bids_.find(loc.px);
    if (lvl == bids_.end()) return false;
    auto& q = lvl->second;
    for (auto dq = q.begin(); dq != q.end(); ++dq) {
      if (dq->id == id) {
        q.erase(dq);
        loc_.erase(it);
        if (q.empty()) bids_.erase(lvl);
        return true;
      }
    }
  } else {
    auto lvl = asks_.find(loc.px);
    if (lvl == asks_.end()) return false;
    auto& q = lvl->second;
    for (auto dq = q.begin(); dq != q.end(); ++dq) {
      if (dq->id == id) {
        q.erase(dq);
        loc_.erase(it);
        if (q.empty()) asks_.erase(lvl);
        return true;
      }
    }
  }
  return false;
}

TopOfBook OrderBook::top() const {
  TopOfBook t;
  if (!bids_.empty()) {
    t.has_bid = true;
    t.bid_px  = bids_.begin()->first;
    int sum = 0; for (const auto& o : bids_.begin()->second) sum += o.qty;
    t.bid_qty = sum;
  }
  if (!asks_.empty()) {
    t.has_ask = true;
    t.ask_px  = asks_.begin()->first;
    int sum = 0; for (const auto& o : asks_.begin()->second) sum += o.qty;
    t.ask_qty = sum;
  }
  return t;
}

} // namespace ts

