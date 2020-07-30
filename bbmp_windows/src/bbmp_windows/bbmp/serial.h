#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bbmp {
class Serial {
 public:
  static void WindowsSleepEx(uint32_t timeout_milliseconds, bool alertable);

  Serial(const char* port_name,
         std::function<void(const char*, size_t)> read_callback);
  ~Serial();

  void IssueRead();

  // Tries to issue an asynchronous write. Fails if there is an outstanding
  // write. In this case you probably want to call WindowsSleepEx until the
  // write completes.
  bool TryIssueWrite(const char* data, size_t length);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

std::vector<std::string> GetComPortNames();
}  // namespace bbmp
