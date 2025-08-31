#include "engine/matching_engine.hpp"
#include <algorithm>

namespace ts {

MatchingEngine::MatchingEngine() {}

bool MatchingEngine::crosses(const Order& taker, double maker_px) {
  if (taker.side == Side::Buy)  return taker.px >= maker_px;  // buy crosses ask
  else                          return taker.px <= maker_px;  // sell crosses bid
}

void MatchingEngine::add_resting(const Order& o) {
  if (o.qty <= 0) return;
  if (o.side == Side::Buy) {
    bids_[o.px].push_back(o);
  } else {
    asks_[o.px].push_back(o);
  }
}

std::vector<Trade> MatchingEngine::match_incoming(Order taker) {
  std::vector<Trade> fills;

  if (taker.side == Side::Buy) {
    // Buyer incoming; match against asks_
    while (taker.qty > 0 && !asks_.empty()) {
      auto it = asks_.begin();
      double maker_px = it->first;
      auto& q = it->second;

      if (taker.px > 0.0 && maker_px > taker.px) break;
      if (q.empty()) { asks_.erase(it); continue; }

      Order maker = q.front();
      int qty = std::min(taker.qty, maker.qty);
      taker.qty -= qty;
      maker.qty -= qty;

      Trade tr;
      tr.maker_id = maker.id;
      tr.taker_id = taker.id;
      tr.qty = qty;
      tr.px = maker_px;
      tr.maker_client = maker.client;
      tr.taker_client = taker.client;
      tr.maker_side = maker.side;
      tr.taker_side = taker.side;
      fills.push_back(tr);

      if (maker.qty == 0) {
        q.pop_front();
        if (q.empty()) asks_.erase(it);
      } else {
        q.front() = maker;
      }
    }
  } else {
    // Seller incoming; match against bids_
    while (taker.qty > 0 && !bids_.empty()) {
      auto it = bids_.begin();
      double maker_px = it->first;
      auto& q = it->second;

      if (taker.px > 0.0 && maker_px < taker.px) break;
      if (q.empty()) { bids_.erase(it); continue; }

      Order maker = q.front();
      int qty = std::min(taker.qty, maker.qty);
      taker.qty -= qty;
      maker.qty -= qty;

      Trade tr;
      tr.maker_id = maker.id;
      tr.taker_id = taker.id;
      tr.qty = qty;
      tr.px = maker_px;
      tr.maker_client = maker.client;
      tr.taker_client = taker.client;
      tr.maker_side = maker.side;
      tr.taker_side = taker.side;
      fills.push_back(tr);

      if (maker.qty == 0) {
        q.pop_front();
        if (q.empty()) bids_.erase(it);
      } else {
        q.front() = maker;
      }
    }
  }

  return fills;
}

std::vector<Trade> MatchingEngine::new_market_order(const std::string& client, Side side, int qty) {
  Order taker;
  taker.id = next_id_++;
  taker.client = client;
  taker.side = side;
  taker.qty = qty;
  taker.px = 0.0; // 0 ==> MARKET (no price constraint)
  return match_incoming(taker);
}

std::vector<Trade> MatchingEngine::new_limit_order(const std::string& client, Side side, int qty, double px) {
  Order taker;
  taker.id = next_id_++;
  taker.client = client;
  taker.side = side;
  taker.qty = qty;
  taker.px = px;

  auto fills = match_incoming(taker);
  if (taker.qty > 0) {
    // remaining becomes maker (resting)
    add_resting(taker);
  }
  return fills;
}

bool MatchingEngine::cancel(uint64_t order_id) {
  // try bids
  for (auto it = bids_.begin(); it != bids_.end(); ++it) {
    auto& dq = it->second;
    for (auto dit = dq.begin(); dit != dq.end(); ++dit) {
      if (dit->id == order_id) {
        dq.erase(dit);
        if (dq.empty()) bids_.erase(it);
        return true;
      }
    }
  }
  // try asks
  for (auto it = asks_.begin(); it != asks_.end(); ++it) {
    auto& dq = it->second;
    for (auto dit = dq.begin(); dit != dq.end(); ++dit) {
      if (dit->id == order_id) {
        dq.erase(dit);
        if (dq.empty()) asks_.erase(it);
        return true;
      }
    }
  }
  return false;
}

TopOfBook MatchingEngine::top() const {
  TopOfBook t;
  if (!bids_.empty()) {
    t.has_bid = true;
    t.bid_px  = bids_.begin()->first;
    int sum = 0; for (auto &o : bids_.begin()->second) sum += o.qty;
    t.bid_qty = sum;
  }
  if (!asks_.empty()) {
    t.has_ask = true;
    t.ask_px  = asks_.begin()->first;
    int sum = 0; for (auto &o : asks_.begin()->second) sum += o.qty;
    t.ask_qty = sum;
  }
  return t;
}

} // namespace ts

