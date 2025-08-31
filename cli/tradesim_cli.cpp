#include "common/types.hpp"
#include "common/util.hpp"
#include "engine/matching_engine.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace ts;

static void print_help() {
  std::cout << "Commands:\n"
            << "  NEW LIMIT BUY  <qty> @ <price> CLIENT <name>\n"
            << "  NEW LIMIT SELL <qty> @ <price> CLIENT <name>\n"
            << "  NEW MARKET BUY  <qty> CLIENT <name>\n"
            << "  NEW MARKET SELL <qty> CLIENT <name>\n"
            << "  CANCEL <order_id>\n"
            << "  BOOK\n"
            << "  TRADES\n"
            << "  HELP\n"
            << "  QUIT\n";
}

struct TradeLog {
  std::vector<Trade> recent;
  void add_all(const std::vector<Trade>& t) {
    recent.insert(recent.end(), t.begin(), t.end());
    if (recent.size() > 1000)
      recent.erase(recent.begin(), recent.begin() + (recent.size() - 1000));
  }
  void print() const {
    if (recent.empty()) {
      std::cout << "(no trades)\n";
      return;
    }
    for (const auto &tr : recent) {
      std::cout << "TRADE maker=" << tr.maker_id
                << " taker=" << tr.taker_id
                << " qty=" << tr.qty
                << " px=" << std::fixed << std::setprecision(2) << tr.px
                << "\n";
    }
  }
};

int main() {
  std::cout << "AUM Trading Simulator — CLI (Step 1)\n";
  MatchingEngine eng;
  TradeLog tlog;
  print_help();

  std::string line;
  while (true) {
    std::cout << "\n> ";
    if (!std::getline(std::cin, line))
      break;
    line = ts::trim(line);
    if (line.empty())
      continue;

    auto toks = ts::tokens(line);
    if (toks.empty())
      continue;
    std::string cmd = toks[0];

    try {
      if (cmd == "HELP") {
        print_help();
      } else if (cmd == "QUIT" || cmd == "EXIT") {
        break;
      } else if (cmd == "BOOK") {
        auto top = eng.top();
        std::cout << "BID: ";
        if (top.has_bid)
          std::cout << top.bid_qty << " @ " << std::fixed
                    << std::setprecision(2) << top.bid_px;
        else
          std::cout << "(none)";
        std::cout << "    |    ASK: ";
        if (top.has_ask)
          std::cout << top.ask_qty << " @ " << std::fixed
                    << std::setprecision(2) << top.ask_px;
        else
          std::cout << "(none)";
        std::cout << "\n";
      } else if (cmd == "TRADES") {
        tlog.print();
      } else if (cmd == "CANCEL") {
        if (toks.size() < 2) {
          std::cout << "usage: CANCEL <order_id>\n";
          continue;
        }
        uint64_t id = std::stoull(toks[1]);
        bool ok = eng.cancel(id);
        std::cout << (ok ? "CANCELLED\n" : "NOT FOUND\n");
      } else if (cmd == "NEW") {
        if (toks.size() < 3) {
          std::cout << "usage: NEW ...\n";
          continue;
        }
        std::string kind = toks[1];
        std::string side_s = toks[2];
        Side side = (side_s == "BUY") ? Side::Buy : Side::Sell;

        if (kind == "LIMIT") {
          if (toks.size() < 8 || toks[4] != "@" || toks[6] != "CLIENT") {
            std::cout
                << "usage: NEW LIMIT BUY <qty> @ <price> CLIENT <name>\n";
            continue;
          }
          int qty = std::stoi(toks[3]);
          double px = std::stod(toks[5]);
          std::string client = toks[7];
          auto trades = eng.new_limit_order(client, side, qty, px);
          tlog.add_all(trades);
          std::cout << "OK (" << trades.size() << " trades)\n";
        } else if (kind == "MARKET") {
          if (toks.size() < 6 || toks[4] != "CLIENT") {
            std::cout << "usage: NEW MARKET BUY <qty> CLIENT <name>\n";
            continue;
          }
          int qty = std::stoi(toks[3]);
          std::string client = toks[5];
          auto trades = eng.new_market_order(client, side, qty);
          tlog.add_all(trades);
          std::cout << "OK (" << trades.size() << " trades)\n";
        } else {
          std::cout << "Order type must be LIMIT or MARKET\n";
        }
      } else {
        std::cout << "Unknown command. Type HELP.\n";
      }
    } catch (const std::exception &e) {
      std::cout << "error: " << e.what() << "\n";
    }
  }

  std::cout << "bye.\n";
  return 0;
}

