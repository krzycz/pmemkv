#pragma once

#include <unistd.h>

#include "Utils.hpp"
#include <chrono>
#include <cstdarg>
#include <mutex>

class Logger {
public:
  void Print(const char *format, ...);
  void Init(FILE *fp);
  void Exec(std::string cmd);

private:
  FILE *log_file_ = NULL;
  std::mutex mut_;

  std::chrono::time_point<std::chrono::system_clock> start_ts_;
};

#ifdef DO_LOG
extern Logger GlobalLogger;
#endif