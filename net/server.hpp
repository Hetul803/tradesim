#pragma once
#include "common/types.hpp"
#include "common/util.hpp"
#include "common/logger.hpp"
#include "engine/matching_engine.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ts {

class Server {
public:
  explicit Server(int port);
  ~Server();

  void run();   // blocks until stop
  void stop();  // stops listening

private:
  int port_;
  int listen_fd_{-1};
  std::atomic<bool> running_{false};
  std::vector<std::thread> client_threads_;

  // Matching engine + guards
  MatchingEngine engine_;
  std::mutex eng_mu_;

  // Simple in-memory trade cache (still keep for TRADES command)
  std::vector<Trade> trades_;
  std::mutex trades_mu_;

  // Logging
  std::string session_id_;
  CsvLogger log_trades_;
  CsvLogger log_book_;

  void handle_client(int client_fd);
  bool setup_listener();
  bool write_line(int fd, const std::string& s);
  bool read_line(int fd, std::string& out);

  void init_logs();  // open CSVs with headers once
};

} // namespace ts

