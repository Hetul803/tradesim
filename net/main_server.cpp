#include "net/server.hpp"

int main() {
  ts::Server s(5555);
  s.run();
  return 0;
}

