#include "net/client.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>

using namespace std::chrono_literals;

static int to_int(const std::string& s) { return std::stoi(s); }

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: bot_random <host> <port> <client_name> [loops=50] [delay_ms=300]\n";
    return 1;
  }

  std::string host = argv[1];
  int port = to_int(argv[2]);
  std::string client = argv[3];
  int loops = (argc >= 5) ? to_int(argv[4]) : 50;
  int delay_ms = (argc >= 6) ? to_int(argv[5]) : 300;

  ts::Client c;
  if (!c.connect(host, port)) {
    std::cerr << "connect failed\n";
    return 2;
  }

  std::string line;
  c.read_line(line); // server greeting

  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> side_dist(0, 1);
  std::uniform_int_distribution<int> qty_dist(1, 5);

  for (int i = 0; i < loops; ++i) {
    // optional: peek at book
    c.send_line("BOOK");
    c.read_line(line);

    bool buy = side_dist(rng) == 1;
    int qty = qty_dist(rng);

    if (buy)
      c.send_line("NEW MARKET BUY " + std::to_string(qty) + " CLIENT " + client);
    else
      c.send_line("NEW MARKET SELL " + std::to_string(qty) + " CLIENT " + client);

    c.read_line(line); // "OK" or error
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  }

  c.send_line("QUIT");
  return 0;
}

