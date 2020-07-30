/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include "components/log_component.h"
#include "components/multi_graph_editor.h"
#include "juce_priorizable_thread.h"
#include "settings.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <vector>

class MainComponent : public juce::Component, public juce::Timer {
 public:
  MainComponent();

  void paint(juce::Graphics&) override;
  void resized() override;

  void timerCallback() override {}

 private:
  LabeledGraphComponent graph_;
  LogComponent log_component_;
  JucePriorizableThread temperature_thread_;
  CoolthSettings settings_;

  void SaveSettings() {
    auto settings_file = juce::File("C:\\Users\\ati\\Desktop\\gadajnc.bin");
    std::ofstream is(settings_file.getFullPathName().toStdString(),
                     std::ios::binary);
    cereal::BinaryOutputArchive archive(is);
    archive(settings_);
  };

  void program_loop(const std::function<bool()>& thread_should_exit,
                    const std::function<void(int)>& wait_ms) {
    juce::int64 time = 0;

    while (!thread_should_exit()) {
      wait_ms(1000);
    }
  }

  void read(const char* data, size_t length) {
    bbmp::NonBlockingLogger::GetInstance().TryLog(
        {nullptr, std::make_unique<std::string>(data, length)});
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
