#pragma once

#include <memory>
#include <string>

namespace bbmp {
struct LogMessage {
  LogMessage() : message(nullptr) {}

  LogMessage(std::string const* message,
             std::unique_ptr<std::string>&& heap_message)
      : message(message), heap_message(std::move(heap_message)) {}

  LogMessage(std::string message)
      : message(nullptr),
        heap_message(std::make_unique<std::string>(message)) {}

  std::string const* message;
  std::unique_ptr<std::string> heap_message;
};

// All public functions are thread safe, non blocking, non allocating
struct NonBlockingLogger {
 private:
  NonBlockingLogger();

 public:
  static NonBlockingLogger& GetInstance() noexcept;

  bool TryLog(LogMessage&& message) noexcept;

  bool TryDequeue(LogMessage& message) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

void Log(LogMessage&& message);
}  // namespace bbmp
