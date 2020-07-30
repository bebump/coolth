/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include "bbmp/child_process.h"
#include "bbmp/fan_controller_communicator.h"
#include "bbmp/line_reader.h"
#include "bbmp/recreate_on_failure.h"
#include "bbmp/stringstream.h"

#include "components/custom_slider.h"
#include "components/log_component.h"
#include "components/multi_graph_editor.h"
#include "juce_priorizable_thread.h"
#include "settings.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>

template <typename T>
class AsyncAccessor : private juce::AsyncUpdater {
 public:
  AsyncAccessor(T& component) : component_(&component) {}

  void Access(std::function<void(T&)> accessor) {
    auto lock = std::lock_guard(m_accessor_);
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      accessor(*component_);
    } else {
      accessor_ = std::move(accessor);
      triggerAsyncUpdate();
    }
  }

 private:
  T* component_;
  std::function<void(T&)> accessor_;
  std::mutex m_accessor_;

  void handleAsyncUpdate() override {
    auto lock = std::lock_guard(m_accessor_);
    if (accessor_) {
      accessor_(*component_);
      accessor_ = {};
    }
  }
};

struct TemperatureComponent : public juce::Component,
                              public juce::AsyncUpdater {
  TemperatureComponent();

  void resized() override;

  void SetTemps(const std::optional<float>& cpu,
                const std::optional<float>& gpu);

  void handleAsyncUpdate() override {
    if (repaint_.load(std::memory_order_relaxed)) {
      repaint_.store(false, std::memory_order_relaxed);
      auto cpu = cpu_temp_.load(std::memory_order_relaxed);
      cpu_.setText("CPU: " + cpu_display_,
                   juce::NotificationType::dontSendNotification);
      auto gpu = gpu_temp_.load(std::memory_order_relaxed);
      gpu_.setText("GPU: " + gpu_display_,
                   juce::NotificationType::dontSendNotification);
      repaint();
    }
  }

  bool GetAverage() { return button_average_.getToggleState(); }

  void SetAverage(bool value) {
    button_average_accessor_.Access([value](auto& button) {
      button.setToggleState(value,
                            juce::NotificationType::dontSendNotification);
    });
  }

  void SetCpuDisplay(juce::String value) {
    cpu_display_accesor_.Access([value = std::move(value), this](auto& s) {
      if (s != value) {
        s = std::move(value);
        repaint_.store(true, std::memory_order_release);
      }
    });
    triggerAsyncUpdate();
  }

  void SetGpuDisplay(juce::String value) {
    gpu_display_accesor_.Access([value = std::move(value), this](auto& s) {
      if (s != value) {
        s = std::move(value);
        repaint_.store(true, std::memory_order_release);
      }
    });
    triggerAsyncUpdate();
  }

 public:
  std::atomic<std::optional<float>> cpu_temp_;
  std::atomic<std::optional<float>> gpu_temp_;
  juce::ToggleButton button_average_;

 private:
  std::atomic<bool> repaint_{false};
  AsyncAccessor<juce::ToggleButton> button_average_accessor_;

  juce::String cpu_display_{"N/A"};
  juce::String gpu_display_{"N/A"};
  AsyncAccessor<juce::String> cpu_display_accesor_;
  AsyncAccessor<juce::String> gpu_display_accesor_;

  juce::Label cpu_;
  juce::Label gpu_;
};

class SliderComponent : public juce::Component {
 public:
  SliderComponent();

  void resized() override;

  std::array<std::unique_ptr<SliderWithNumberInTheMiddle>,
             CoolthSettings::kNumFans>
      sliders_;
  std::mutex mutex_values_to_set_;
};

class MainComponent : public juce::Component {
 public:
  MainComponent();

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  SliderComponent slider_component_;
  TemperatureComponent temperature_component_;
  LogComponent log_component_;
  bool show_log_ = false;
  juce::TextButton button_log_;
  std::unique_ptr<ChildProcess> temperature_reader_;
  JucePriorizableThread temperature_thread_;
  CoolthSettings settings_;
  juce::TabbedComponent tabs_;
  std::array<std::unique_ptr<MultiGraphComponent>, CoolthSettings::kNumFans>
      fan_graphs_;
  static constexpr int log_width = 400;
  juce::ImageButton button_info_;

  void program_loop(const std::function<bool()>& thread_should_exit,
                    const std::function<void(int)>& wait_ms);

  void read(const char* data, size_t length) {
    bbmp::NonBlockingLogger::GetInstance().TryLog(
        {nullptr, std::make_unique<std::string>(data, length)});
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
