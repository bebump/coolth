#pragma once

#include "line_reader.h"
#include "serial.h"
#include "settings.h"
#include "stringstream.h"

#include <array>
#include <functional>
#include <mutex>

class FanControllerCommunicator {
 public:
  FanControllerCommunicator(CoolthSettings& settings,
                            const std::function<bool()>& should_exit,
                            const std::function<void(int)>& wait_ms)
      : settings_(&settings),
        line_reader_(256, [this](const char* data, size_t length) {
          StringStream stream(data, length);
          for (int i = 0; i < 4; ++i) {
            if (auto rpm = stream.GetInt()) {
              auto lock = std::lock_guard(mutex_rpm_);
              rpm_[i] = *rpm;
            } else {
              valid_message_received_ = false;
              return;
            }
          }
          valid_message_received_ = true;
        }) {
    while (!should_exit() && !valid_message_received_) {
      auto com_port_names = bbmp::GetComPortNames();

      // Check if we have a last working port stored in settings
      if (com_port_names.size() > 0) {
        juce::String last_working_port;
        settings.AccessLastComPort(
            [&last_working_port](auto& value) { last_working_port = value; });

        if (!last_working_port.isEmpty()) {
          // If there is such a setting and it is one of the currently
          // available port names, then bring this port name to the 0th
          // element of the list. Consequently this will be the first port
          // that we will try to connect to.
          for (size_t i = 0; i < com_port_names.size(); ++i) {
            if (com_port_names[i].compare(last_working_port.toStdString()) ==
                0) {
              if (i != 0) {
                std::swap(com_port_names[0], com_port_names[i]);
              }
              break;
            }
          }
        }

        for (size_t i = 0; i < com_port_names.size() && !should_exit(); ++i) {
          bbmp::Log({"Connecting to port " + com_port_names[i] + "..."});
          try {
            serial_ = nullptr;
            serial_ = std::make_unique<bbmp::Serial>(
                com_port_names[i].c_str(),
                [this](const char* data, size_t size) {
                  line_reader_.Read(data, size);
                });
            const int wait_for_ms = 50;
            for (int i = 0; i < 4000 / wait_for_ms && !should_exit(); ++i) {
              serial_->IssueRead();
              WindowsSleepEx(wait_for_ms, true);
              if (valid_message_received_) {
                break;
              }
            }
            if (valid_message_received_) {
              bbmp::Log({"Fan controller found on port " + com_port_names[i]});
              settings.AccessLastComPort(
                  [&port = com_port_names[i]](auto& value) { value = port; });
              settings.should_save_.store(true, std::memory_order_release);
              break;
            } else {
              bbmp::Log(
                  {"No valid message was received on " + com_port_names[i]});
            }
          } catch (std::runtime_error& error) {
            bbmp::Log(std::string(error.what()));
          }
        }

        if (!valid_message_received_) {
          serial_ = nullptr;
          bbmp::Log({"Fan controller not found. Retrying in 2 seconds..."});
          if (!should_exit()) {
            wait_ms(2000);
          }
        }
      } else {
        bbmp::Log({"No serial ports are available. Retrying in 2 seconds..."});
        if (!should_exit()) {
          wait_ms(2000);
        }
      }
    }
  }

  void IssueRead() { serial_->IssueRead(); }

  std::array<int, 4> GetRpms() { return rpm_; }

 public:
  std::unique_ptr<bbmp::Serial> serial_;
  CoolthSettings* settings_;
  bool valid_message_received_ = false;
  LineReader line_reader_;
  std::array<int, 4> rpm_{};
  std::mutex mutex_rpm_;
};
