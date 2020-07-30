#pragma once

#include "logging.h"

#include <chrono>
#include <functional>
#include <memory>
#include <thread>

template <typename T>
class RecreateOnFailure {
 public:
  RecreateOnFailure(std::function<std::unique_ptr<T>()> instantiate)
      : instantiate_(std::move(instantiate)), instance_(instantiate_()) {}

  void Execute(std::function<void(T&)> action) {
    bool recreate = false;

    try {
      action(*instance_);
    } catch (std::runtime_error& error) {
      bbmp::NonBlockingLogger::GetInstance().TryLog(
          {nullptr, std::make_unique<std::string>(error.what())});
      recreate = true;
    }

    if (recreate) {
      bool create_success = false;

      while (!create_success) {
        try {
          instance_ = instantiate_();
          create_success = true;
        } catch (std::runtime_error& error) {
          bbmp::NonBlockingLogger::GetInstance().TryLog(
              {nullptr, std::make_unique<std::string>(error.what())});
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
      }
    }
  }

 private:
  std::function<std::unique_ptr<T>()> instantiate_;
  std::unique_ptr<T> instance_;
};
