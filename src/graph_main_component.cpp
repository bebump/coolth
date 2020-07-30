/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "graph_main_component.h"

#include <fstream>

MainComponent::MainComponent()
    : graph_(30, 90, 0, 100, {"CPU", "GPU"}),
      log_component_(
          [] {
            bbmp::LogMessage message;
            if (bbmp::NonBlockingLogger::GetInstance().TryDequeue(message)) {
              return std::make_optional<bbmp::LogMessage>(std::move(message));
            }
            return std::optional<bbmp::LogMessage>{};
          },
          false),
      temperature_thread_(
          [this](const std::function<bool()>& thread_should_exit,
                 const std::function<void(int)>& wait_ms) {
            data_collection_loop(thread_should_exit, wait_ms);
          }) {
  graph_.SetXTicks({30, 40, 50, 60, 70, 80, 90});
  graph_.SetYTicks({0, 20, 40, 60, 80, 100});
  graph_.SetXLabel(
      juce::CharPointer_UTF8("T [\xc2\xb0"
                             "C]"));
  graph_.SetYLabel("Duty cycle [%]");

  addAndMakeVisible(graph_);
  addAndMakeVisible(log_component_);
  setSize(600, 400);
  temperature_thread_.startThread(0);

  auto settings_file = juce::File("C:\\Users\\ati\\Desktop\\gadajnc.bin");
  if (settings_file.existsAsFile()) {
    std::ifstream is(settings_file.getFullPathName().toStdString(),
                     std::ios::binary);
    cereal::BinaryInputArchive archive(is);
    archive(settings_);
    CoolthSettings::TTempCurves curves;
    settings_.AccessTempCurves(
        [&curves](auto& settings_curves) { curves = settings_curves; });
    graph_.SetState(std::move(curves[0]));
  }

  graph_.SetOnChange([this](size_t i, std::vector<juce::Point<float>> values) {
    settings_.AccessTempCurves(
        [&values, i](CoolthSettings::TTempCurves& curves) {
          if (curves.size() < 1) {
            curves.emplace_back();
          }
          while (curves[0].size() <= i) {
            curves[0].emplace_back();
          }
          curves[0][i] = std::move(values);
        });
    SaveSettings();
  });

  startTimerHz(1);
}

void MainComponent::paint(juce::Graphics& g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() {
  auto local_bounds = getLocalBounds();
  log_component_.setBounds(local_bounds.removeFromBottom(100));
  graph_.setBounds(local_bounds);
}
