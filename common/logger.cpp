#include "common/logger.hpp"
#include <sys/stat.h>
#include <cstring>

namespace ts {

// little helper to check if file exists and has non-zero size
static bool file_has_data(const std::string& path) {
  struct stat st{};
  if (stat(path.c_str(), &st) != 0) return false;
  return st.st_size > 0;
}

CsvLogger::~CsvLogger() {
  if (fp_) { std::fflush(fp_); std::fclose(fp_); fp_ = nullptr; }
}

bool CsvLogger::open(const std::string& path, const std::vector<std::string>& header) {
  std::lock_guard<std::mutex> lk(mu_);
  if (fp_) { std::fflush(fp_); std::fclose(fp_); fp_ = nullptr; }
  fp_ = std::fopen(path.c_str(), "a");
  if (!fp_) return false;

  if (!file_has_data(path) && !header.empty()) {
    // write header
    for (size_t i = 0; i < header.size(); ++i) {
      if (i) std::fputc(',', fp_);
      std::fputs(header[i].c_str(), fp_);
    }
    std::fputc('\n', fp_);
    std::fflush(fp_);
  }
  return true;
}

void CsvLogger::write_row(const std::vector<std::string>& row) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!fp_) return;
  for (size_t i = 0; i < row.size(); ++i) {
    if (i) std::fputc(',', fp_);
    std::fputs(row[i].c_str(), fp_);
  }
  std::fputc('\n', fp_);
}

void CsvLogger::flush() {
  std::lock_guard<std::mutex> lk(mu_);
  if (fp_) std::fflush(fp_);
}

} // namespace ts

