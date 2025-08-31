#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ts {

// trim leading & trailing whitespace
inline std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b-1]))) --b;
  return s.substr(a, b - a);
}

// split on whitespace into tokens
inline std::vector<std::string> tokens(const std::string& s) {
  std::istringstream iss(s);
  std::vector<std::string> out;
  std::string t;
  while (iss >> t) out.push_back(t);
  return out;
}

// (Intentionally no now_ns() here; it lives in common/types.hpp)

} // namespace ts

