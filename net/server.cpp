#include "net/server.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace ts {

static bool set_reuseaddr(int fd) {
  int opt = 1;
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

Server::Server(int port) : port_(port) {}
Server::~Server() { stop(); }

bool Server::setup_listener() {
  listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) { perror("socket"); return false; }
  set_reuseaddr(listen_fd_);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
  addr.sin_port = htons(static_cast<uint16_t>(port_));
  if (::bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind"); return false;
  }
  if (::listen(listen_fd_, 16) < 0) { perror("listen"); return false; }
  return true;
}

void Server::init_logs() {
  // session id based on monotonic time (ns) to avoid collisions
  session_id_ = std::to_string(now_ns());
  // Open logs/* files with headers
  (void)log_trades_.open("logs/" + session_id_ + "_trades.csv",
                         {"ts_ns","maker_id","taker_id","qty","px"});
  (void)log_book_.open("logs/" + session_id_ + "_book.csv",
                       {"ts_ns","has_bid","bid_px","bid_qty","has_ask","ask_px","ask_qty"});
}

void Server::run() {
  if (!setup_listener()) return;
  init_logs();

  running_.store(true);
  std::cout << "Server listening on port " << port_ << "  (session " << session_id_ << ")\n";

  while (running_.load()) {
    int cfd = ::accept(listen_fd_, nullptr, nullptr);
    if (cfd < 0) {
      if (running_.load()) perror("accept");
      break;
    }
    client_threads_.emplace_back([this, cfd]() { handle_client(cfd); });
  }
  for (auto& t : client_threads_) if (t.joinable()) t.join();
}

void Server::stop() {
  running_.store(false);
  if (listen_fd_ >= 0) {
    ::shutdown(listen_fd_, SHUT_RDWR);
    ::close(listen_fd_);
    listen_fd_ = -1;
  }
}

bool Server::write_line(int fd, const std::string& s) {
  std::string line = s + "\n";
  return ::send(fd, line.data(), line.size(), 0) == (ssize_t)line.size();
}

bool Server::read_line(int fd, std::string& out) {
  out.clear();
  char ch;
  while (true) {
    ssize_t r = ::recv(fd, &ch, 1, 0);
    if (r <= 0) return false;
    if (ch == '\n') break;
    if (ch != '\r') out.push_back(ch);
    if (out.size() > 4096) return false;
  }
  return true;
}

void Server::handle_client(int cfd) {
  write_line(cfd, "WELCOME AUM TradeSim. Type HELP for commands.");
  std::string line;

  while (read_line(cfd, line)) {
    line = ts::trim(line);
    if (line.empty()) continue;
    auto toks = ts::tokens(line);
    if (toks.empty()) continue;

    std::string cmd = toks[0];

    if (cmd == "QUIT" || cmd == "EXIT") break;

    if (cmd == "HELP") {
      write_line(cfd, "Commands: NEW LIMIT/NEW MARKET/BOOK/TRADES/CANCEL/QUIT");
      continue;
    }

    if (cmd == "BOOK") {
      TopOfBook top;
      {
        std::lock_guard<std::mutex> lk(eng_mu_);
        top = engine_.top();
      }
      // log top-of-book snapshot
      log_book_.write_row({
        std::to_string(now_ns()),
        top.has_bid ? "1" : "0",
        top.has_bid ? std::to_string(top.bid_px) : "",
        top.has_bid ? std::to_string(top.bid_qty) : "",
        top.has_ask ? "1" : "0",
        top.has_ask ? std::to_string(top.ask_px) : "",
        top.has_ask ? std::to_string(top.ask_qty) : ""
      });

      std::ostringstream msg;
      msg << "BOOK ";
      if (top.has_bid) msg << "BID " << top.bid_qty << "@" << std::fixed << std::setprecision(2) << top.bid_px;
      else msg << "BID none";
      msg << " | ";
      if (top.has_ask) msg << "ASK " << top.ask_qty << "@" << std::fixed << std::setprecision(2) << top.ask_px;
      else msg << "ASK none";
      write_line(cfd, msg.str());
      continue;
    }

    if (cmd == "TRADES") {
      std::lock_guard<std::mutex> lk(trades_mu_);
      if (trades_.empty()) write_line(cfd, "(no trades)");
      else {
        for (auto& tr : trades_) {
          write_line(cfd, "TRADE " + std::to_string(tr.qty) + "@" + std::to_string(tr.px));
        }
      }
      continue;
    }

    // Order entry (same command shapes as CLI)
    try {
      if (toks.size() >= 8 && toks[0]=="NEW" && toks[1]=="LIMIT" && toks[6]=="CLIENT") {
        Side side = (toks[2]=="BUY") ? Side::Buy : Side::Sell;
        int qty = std::stoi(toks[3]);
        double px = std::stod(toks[5]);
        std::string client = toks[7];

        std::vector<Trade> trades;
        {
          std::lock_guard<std::mutex> lk(eng_mu_);
          trades = engine_.new_limit_order(client, side, qty, px);
        }
        // log trades
        if (!trades.empty()) {
          std::lock_guard<std::mutex> lk(trades_mu_);
          for (auto& tr : trades_) { (void)tr; } // keep vector warm
          for (auto& tr : trades) {
            trades_.push_back(tr);
            log_trades_.write_row({
              std::to_string(now_ns()),
              std::to_string(tr.maker_id),
              std::to_string(tr.taker_id),
              std::to_string(tr.qty),
              std::to_string(tr.px)
            });
          }
        }
        write_line(cfd, "OK");
      } else if (toks.size() >= 6 && toks[0]=="NEW" && toks[1]=="MARKET" && toks[4]=="CLIENT") {
        Side side = (toks[2]=="BUY") ? Side::Buy : Side::Sell;
        int qty = std::stoi(toks[3]);
        std::string client = toks[5];

        std::vector<Trade> trades;
        {
          std::lock_guard<std::mutex> lk(eng_mu_);
          trades = engine_.new_market_order(client, side, qty);
        }
        // log trades
        if (!trades.empty()) {
          std::lock_guard<std::mutex> lk(trades_mu_);
          for (auto& tr : trades) {
            trades_.push_back(tr);
            log_trades_.write_row({
              std::to_string(now_ns()),
              std::to_string(tr.maker_id),
              std::to_string(tr.taker_id),
              std::to_string(tr.qty),
              std::to_string(tr.px)
            });
          }
        }
        write_line(cfd, "OK");
      } else if (cmd == "CANCEL" && toks.size() >= 2) {
        uint64_t oid = std::stoull(toks[1]);
        bool ok;
        {
          std::lock_guard<std::mutex> lk(eng_mu_);
          ok = engine_.cancel(oid);
        }
        write_line(cfd, ok ? "CANCELLED" : "NOT FOUND");
      } else {
        write_line(cfd, "ERROR unknown command");
      }
    } catch (...) {
      write_line(cfd, "ERROR parsing command");
    }
  }

  ::close(cfd);
}

} // namespace ts

