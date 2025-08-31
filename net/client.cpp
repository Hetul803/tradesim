#include "net/client.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace ts {

Client::Client() {}
Client::~Client() { close(); }

bool Client::connect(const std::string& host, int port) {
  close();

  struct addrinfo hints{}, *res=nullptr;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0) return false;

  int fd = -1;
  for (auto p=res; p; p=p->ai_next) {
    if (p->ai_family != AF_INET && p->ai_family != AF_INET6) continue;
    fd = socket(p->ai_family, SOCK_STREAM, 0);
    if (fd < 0) continue;

    if (p->ai_family == AF_INET) {
      sockaddr_in a = *reinterpret_cast<sockaddr_in*>(p->ai_addr);
      a.sin_port = htons((uint16_t)port);
      if (::connect(fd, (sockaddr*)&a, sizeof(a)) == 0) { freeaddrinfo(res); fd_ = fd; return true; }
    } else {
      sockaddr_in6 a6 = *reinterpret_cast<sockaddr_in6*>(p->ai_addr);
      a6.sin6_port = htons((uint16_t)port);
      if (::connect(fd, (sockaddr*)&a6, sizeof(a6)) == 0) { freeaddrinfo(res); fd_ = fd; return true; }
    }
    ::close(fd);
  }
  freeaddrinfo(res);
  return false;
}

bool Client::send_line(const std::string& s) {
  if (fd_ < 0) return false;
  std::string line = s + "\n";
  const char* p = line.data();
  size_t n = line.size();
  while (n > 0) {
    ssize_t w = ::send(fd_, p, n, 0);
    if (w <= 0) return false;
    p += w; n -= (size_t)w;
  }
  return true;
}

bool Client::read_line(std::string& out) {
  if (fd_ < 0) return false;
  out.clear();
  char ch;
  while (true) {
    ssize_t r = ::recv(fd_, &ch, 1, 0);
    if (r <= 0) return false;
    if (ch == '\n') break;
    if (ch != '\r') out.push_back(ch);
    if (out.size() > 4096) return false;
  }
  return true;
}

void Client::close() {
  if (fd_ >= 0) { ::shutdown(fd_, SHUT_RDWR); ::close(fd_); fd_ = -1; }
}

} // namespace ts

