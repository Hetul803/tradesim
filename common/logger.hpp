#pragma once
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace ts {

// Simple CSV logger (thread-safe). Opens a file in append mode.
// If the file is empty, writes a header as the first row.
class CsvLogger {
public:
  CsvLogger() = default;
  ~CsvLogger();

  // Open (or reopen) a CSV file at 'path'. If header.size()>0 and file is empty, write header.
  bool open(const std::string& path, const std::vector<std::string>& header);

  // Append a row; items are written as plain text separated by commas.
  void write_row(const std::vector<std::string>& row);

  // Flush buffered data to disk.
  void flush();

private:
  std::FILE* fp_{nullptr};
  std::mutex mu_;
};

} // namespace ts

