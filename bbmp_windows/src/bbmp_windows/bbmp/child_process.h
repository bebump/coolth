#pragma once

#include <cstdint>
#include <functional>
#include <memory>

class ChildProcess {
 public:
  ChildProcess(const std::string& path_to_exe,
               std::function<void(const char*, size_t)> read_callback);
  ~ChildProcess();

  void IssueRead();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

void WindowsSleepEx(uint32_t timeout_milliseconds, bool alertable);
