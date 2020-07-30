#include "logging.h"
#include <moodycamel/concurrentqueue.h>

namespace bbmp {

struct NonBlockingLogger::Impl {
  Impl();
  ~Impl();

  bool TryLog(LogMessage&& msg) noexcept;

  bool TryDequeue(LogMessage& message) noexcept;

 private:
  moodycamel::ConcurrentQueue<LogMessage> msg_queue_;
};

// >>> NonBlockingLogger::Impl member definitions =============================
NonBlockingLogger::Impl::Impl() : msg_queue_(500) {}

NonBlockingLogger::Impl::~Impl() = default;

bool NonBlockingLogger::Impl::TryLog(LogMessage&& msg) noexcept {
  return msg_queue_.try_enqueue(std::move(msg));
}

bool NonBlockingLogger::Impl::TryDequeue(LogMessage& message) noexcept {
  return msg_queue_.try_dequeue(message);
}

NonBlockingLogger& NonBlockingLogger::GetInstance() noexcept {
  static NonBlockingLogger instance;
  return instance;
}
// <<< NonBlockingLogger::Impl member definitions -----------------------------

// >>> NonBlockingLogger member definitions ===================================
NonBlockingLogger::NonBlockingLogger() : impl_(std::make_unique<Impl>()) {}

bool NonBlockingLogger::TryLog(LogMessage&& msg) noexcept {
  return impl_->TryLog(std::move(msg));
}

bool NonBlockingLogger::TryDequeue(LogMessage& message) noexcept {
  return impl_->TryDequeue(message);
}
// <<< NonBlockingLogger member definitions -----------------------------------

void Log(LogMessage&& message) {
  NonBlockingLogger::GetInstance().TryLog(std::move(message));
}
}  // namespace bbmp
