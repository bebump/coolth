#pragma once

#define NOMINMAX
#define NOGDI
#include <strsafe.h>
#include <windows.h>
#undef NOMINMAX
#undef NOGDI

#include <functional>
#include <string>

std::string GetLastErrorAsString();

/*
 * The (sad) reason for the templated approach:
 *
 * Some Win32 API functions return INVALID_HANDLE_VALUE (-1) to signal a failed
 * attempt. I actually have seen 0 returned by them as a valid, working handle.
 *
 * Others return NULL.
 */
template <int V_INVALID>
class WindowsHandle final {
 public:
  WindowsHandle() : handle_(reinterpret_cast<HANDLE>(V_INVALID)) {}

  WindowsHandle(HANDLE raw_handle) : handle_(raw_handle) {
    ThrowIfHandleIsInvalid();
  }

  WindowsHandle(const WindowsHandle<V_INVALID>& other) = delete;

  WindowsHandle(WindowsHandle<V_INVALID>&& other) {
    if (handle_ != reinterpret_cast<HANDLE>(V_INVALID)) {
      const auto success = CloseHandle(handle_);
      if (!success) {
        throw std::runtime_error("Failed to close WindowsHandle");
      }
    }

    handle_ = other.Get();
    other.handle_ = reinterpret_cast<HANDLE>(V_INVALID);
  }

  WindowsHandle& operator=(WindowsHandle<V_INVALID>&& other) {
    if (handle_ != reinterpret_cast<HANDLE>(V_INVALID)) {
      const auto success = CloseHandle(handle_);
      if (!success) {
        throw std::runtime_error("Failed to close WindowsHandle");
      }
    }

    handle_ = other.Get();
    other.handle_ = reinterpret_cast<HANDLE>(V_INVALID);
    return *this;
  }

  ~WindowsHandle() noexcept {
    // The handle can become invalid after a move operation
    if (handle_ != reinterpret_cast<HANDLE>(V_INVALID)) {
      const auto success = CloseHandle(handle_);
      if (!success) {
        // TODO: log error as we can't throw from destructor
      }
    }
  }

  HANDLE& Get() {
    ThrowIfHandleIsInvalid();
    return handle_;
  }

 private:
  HANDLE handle_;

  void ThrowIfHandleIsInvalid() {
    if (handle_ == reinterpret_cast<HANDLE>(V_INVALID)) {
      throw std::runtime_error("Invalid handle encountered in WindowsHandle");
    }
  }
};

class ScopeGuard final {
 public:
  void Add(std::function<void()>&& deferred_function) noexcept {
    deferred_functions_.push_back(std::move(deferred_function));
  }

  void CancelAll() noexcept { deferred_functions_.clear(); }

  ~ScopeGuard() noexcept {
    try {
      for (auto& f : deferred_functions_) {
        f();
      }
    } catch (...) {
    }
  }

 private:
  std::vector<std::function<void()>> deferred_functions_;
};