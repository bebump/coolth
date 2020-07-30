/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include "graph_editor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class MultiGraphComponent : public juce::Component, public juce::AsyncUpdater {
 public:
  MultiGraphComponent(float xmin, float xmax, float ymin, float ymax,
                      std::vector<juce::String> legend = {});

  void EnableGraph(size_t i_graph);

  void OnChangeCallback(size_t i_graph, GraphComponent::Model& model);

  template <size_t N>
  void SetState(std::array<std::vector<juce::Point<float>>, N>& points) {
    for (auto& a : points) {
      for (auto& b : a) {
        b = Normalize(b);
      }
    }

    const auto num_graphs = std::min(points.size(), graphs_.size());
    for (auto i = 0u; i < num_graphs; ++i) {
      graphs_[i]->AccessModel(
          [&graph_points = points[i]](GraphComponent::Model& model) {
            model.Clear();
            for (const auto& p : graph_points) {
              model.Add(p);
            }
          });
    }
  }

  void resized() override;

  void paint(juce::Graphics& g);

  void SetXTicks(std::vector<float> xlabels) { xticks_ = std::move(xlabels); }

  void SetYTicks(std::vector<float> ylabels) { yticks_ = std::move(ylabels); }

  void SetXLabel(juce::String xlabel) { xlabel_ = std::move(xlabel); }

  void SetYLabel(juce::String ylabel) { ylabel_ = std::move(ylabel); }

  juce::Point<float> Normalize(const juce::Point<float>& value);

  juce::Point<float> Denormalize(const juce::Point<float>& value);

  // Not really a generally applicable function. It's only really for Coolth.
  void SetPoint(const std::optional<juce::Point<float>>& p) {
    point_.store(p);
    triggerAsyncUpdate();
  }

  void SetOnChange(
      std::function<void(size_t i, std::vector<juce::Point<float>>)> callback);

  void handleAsyncUpdate() override { repaint(); }

 private:
  std::vector<std::unique_ptr<GraphComponent>> graphs_;
  std::vector<std::unique_ptr<juce::TextButton>> graph_selector_buttons_;
  float xmin_, xmax_, ymin_, ymax_;
  std::vector<float> xticks_, yticks_;
  juce::String xlabel_, ylabel_;
  std::vector<juce::Colour> colours_;
  std::vector<juce::String> legend_;

  std::function<void(size_t i, std::vector<juce::Point<float>>)>
      on_change_callback_;
  std::mutex on_change_mutex_;

  std::atomic<std::optional<juce::Point<float>>> point_;
};
