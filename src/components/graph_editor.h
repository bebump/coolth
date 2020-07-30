/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

struct GraphComponent : public juce::Component {
  class Model {
   public:
    void Clear();

    int Add(const juce::Point<float>& p);

    void SortByX();

    void Modify(size_t index, const juce::Point<float> new_position);

    void Delete(size_t index);

    int HitTest(const juce::Point<float>& pointer, float distance) const;

    void IterateOverOrderedPoints(
        const std::function<void(const juce::Point<float>&)>& accessor);

   private:
    std::vector<juce::Point<float>> points_;
    std::vector<int> sorted_indices_;
    bool sorted_ = true;
  };

  struct Transform {
    static juce::Point<float> FlipY(const juce::Point<float>& p) {
      return {p.getX(), -p.getY()};
    }

    Transform(const juce::Rectangle<int>& bounds)
        : local_bounds_(bounds.toFloat()),
          bounds_size_(local_bounds_.getWidth(), local_bounds_.getHeight()) {}

    juce::Point<float> ToImage(const juce::Point<float>& p) const {
      return FlipY(p * bounds_size_) + local_bounds_.getBottomLeft();
    }

    juce::Point<float> FromImage(const juce::Point<float>& p) const {
      return FlipY(p - local_bounds_.getBottomLeft()) / bounds_size_;
    }

    float YToImage(const juce::Point<float>& p) const {
      return -p.getY() * bounds_size_.getY() + local_bounds_.getBottom();
    }

    float YFromImage(const juce::Point<float>& p) const {
      return -(p.getY() - local_bounds_.getBottom()) / bounds_size_.getY();
    }

    float ToImage(float distance) const {
      return distance * bounds_size_.getDistanceFromOrigin();
    }

    float FromImage(float distance) const {
      return distance / bounds_size_.getDistanceFromOrigin();
    }

    juce::Rectangle<float> local_bounds_;
    juce::Point<float> bounds_size_;
  };

 public:
  GraphComponent() {}

  void paint(juce::Graphics& g);

  void mouseDown(const juce::MouseEvent& e) override;

  void mouseUp(const juce::MouseEvent&) override;

  void mouseDrag(const juce::MouseEvent& e) override;

  juce::Rectangle<int> GetDrawableAreaInParent() const {
    return getBoundsInParent().reduced(padding_);
  }

  void SetColor(juce::Colour colour) { colour_ = colour; }

  void SetEnabled(bool enabled) {
    enabled_ = enabled;
    repaint();
  }

  void AccessModel(const std::function<void(Model& model)>& accessor);

  void SetOnChange(std::function<void(Model&)> callback) {
    auto lock = std::lock_guard(on_change_mutex_);
    on_change_callback_ = std::move(callback);
  }

  static std::optional<float> GetYForX(std::vector<juce::Point<float>>& points,
                                       const float x) {
    if (points.size() == 0) {
      return {};
    }

    std::optional<juce::Point<float>> smaller;
    std::optional<juce::Point<float>> larger;
    for (const auto& p : points) {
      if (p.getX() <= x) {
        smaller = p;
      }
      if (!larger.has_value() && x < p.getX()) {
        larger = p;
      }
      if (smaller.has_value() && larger.has_value()) {
        break;
      }
    }

    if (!smaller.has_value()) {
      return larger->getY();
    }
    if (!larger.has_value()) {
      return smaller->getY();
    }

    return {smaller->getY() + (larger->getY() - smaller->getY()) *
                                  (x - smaller->getX()) /
                                  (larger->getX() - smaller->getX())};
  }

 private:
  Model model_;
  std::mutex model_mutex_;
  bool mouse_down_{false};
  int position_held = -1;
  int padding_ = 10;
  std::optional<juce::Colour> colour_;
  bool enabled_ = true;

  std::function<void(Model&)> on_change_callback_;
  std::mutex on_change_mutex_;
};
