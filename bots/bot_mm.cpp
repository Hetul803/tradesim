#include "net/client.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

static bool parse_book_line(const std::string& s,
                            bool& has_bid, int& bid_qty, double& bid_px,
                            bool& has_ask, int& ask_qty, double& ask_px) {
  has_bid = has_ask = false; bid_qty = ask_qty = 0; bid_px = ask_px = 0.0;
  auto p = s.find("BOOK ");
  if (p == std::string::npos) return false;
  std::string rest = s.substr(p + 5);
  auto bar = rest.find('|');
  if (bar == std::string::npos) return false;
  std::string left = rest.substr(0, bar);
  std::string right = rest.substr(bar + 1);
  { std::istringstream iss(left); std::string tag, tok; iss >> tag >> tok;
    if (tag=="BID" && tok!="none") { auto at=tok.find('@'); if (at==std::string::npos) return false;
      bid_qty = std::stoi(tok.substr(0,at)); bid_px = std::stod(tok.substr(at+1)); has_bid=true; } }
  { std::istringstream iss(right); std::string tag, tok; iss >> tag >> tok;
    if (tag=="ASK" && tok!="none") { auto at=tok.find('@'); if (at==std::string::npos) return false;
      ask_qty = std::stoi(tok.substr(0,at)); ask_px = std::stod(tok.substr(at+1)); has_ask=true; } }
  return true;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: bot_mm <host> <port> <client_name> [loops=80] [delay_ms=400]\n";
    return 1;
  }
  std::string host = argv[1];
  int port = std::stoi(argv[2]);
  std::string client = argv[3];
  int loops = (argc >= 5) ? std::stoi(argv[4]) : 80;
  int delay_ms = (argc >= 6) ? std::stoi(argv[5]) : 400;

  ts::Client c;
  if (!c.connect(host, port)) { std::cerr << "connect failed\n"; return 2; }
  std::string line; c.read_line(line); // greeting

  for (int i = 0; i < loops; ++i) {
    c.send_line("BOOK");
    if (!c.read_line(line)) break;
    bool hb=false, ha=false; int bq=0, aq=0; double bp=0, ap=0;
    parse_book_line(line, hb, bq, bp, ha, aq, ap);

    double mid = 10.00;
    if (hb && ha) mid = 0.5*(bp+ap);
    else if (hb)  mid = bp + 0.05;
    else if (ha)  mid = ap - 0.05;

    double bid_px = mid - 0.05;
    double ask_px = mid + 0.05;

    c.send_line("NEW LIMIT BUY  1 @ " + std::to_string(bid_px) + " CLIENT " + client);
    c.read_line(line);
    c.send_line("NEW LIMIT SELL 1 @ " + std::to_string(ask_px) + " CLIENT " + client);
    c.read_line(line);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  }
  c.send_line("QUIT");
  return 0;
}

