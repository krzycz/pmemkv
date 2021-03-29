#include "Logger.hpp"

#include <array>
#include <memory>

void Logger::Print(const char *format, ...) {
  if (log_file_ != NULL) {
    std::lock_guard<std::mutex> lg(mut_);
    auto now = std::chrono::system_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_ts_);
    fprintf(log_file_, "%ld: ", duration.count());
    va_list args;
    va_start(args, format);
    vfprintf(log_file_, format, args);
    va_end(args);
    fflush(log_file_);
    fsync(fileno(log_file_));
  }
}

void Logger::Exec(std::string cmd) {
  std::array<char, 10000> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  Print("%s \n", result.c_str());
}

void Logger::Init(FILE *fp) {
  log_file_ = fp;
  start_ts_ = std::chrono::system_clock::now();
}

#ifdef DO_LOG
Logger GlobalLogger;
#endif