#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace ts {

inline uint64_t now_ns() {
  using namespace std::chrono;
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

enum class Side { Buy=0, Sell=1 };

struct Trade {
  // Order IDs of the resting (maker) and incoming (taker) orders
  uint64_t maker_id{0};
  uint64_t taker_id{0};
  int qty{0};
  double px{0.0};

  // NEW: attribution + direction
  std::string maker_client;  // who provided liquidity
  std::string taker_client;  // who took liquidity
  Side maker_side{Side::Sell}; // if maker sold, taker bought (and vice versa)
  Side taker_side{Side::Buy};
};

// Convenience: top-of-book snapshot
struct TopOfBook {
  bool has_bid{false};
  double bid_px{0.0};
  int bid_qty{0};
  bool has_ask{false};
  double ask_px{0.0};
  int ask_qty{0};
};

} // namespace ts

