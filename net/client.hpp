#pragma once
#include <string>

namespace ts {

class Client {
public:
  Client();
  ~Client();

  // Connect to host:port (host like "localhost" or "127.0.0.1")
  bool connect(const std::string& host, int port);

  // Send one line (adds newline)
  bool send_line(const std::string& s);

  // Read one line (strips \r\n); false on disconnect/error
  bool read_line(std::string& out);

  // Close socket
  void close();

private:
  int fd_{-1};
};

} // namespace ts

