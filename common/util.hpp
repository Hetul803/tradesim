#pragma once
#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace ts {

inline std::string trim(const std::string& s) {
  size_t start = 0, end = s.size();
  while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
  return s.substr(start, end - start);
}

inline std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) out.push_back(item);
  return out;
}

inline std::vector<std::string> tokens(const std::string& line) {
  std::vector<std::string> toks;
  std::string cur;
  for (char c : line) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) toks.push_back(cur);
  return toks;
}

inline long long now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
           std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace ts

