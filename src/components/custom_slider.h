/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <optional>

class SliderWithNumberInTheMiddle : public juce::Component,
                                    private juce::AsyncUpdater {
  struct LookAndFeel : public juce::LookAndFeel_V4 {
    LookAndFeel() : number_in_the_middle_(0) {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width,
                          int height, float sliderPos,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          juce::Slider& slider) override;

    std::atomic<int> number_in_the_middle_;
  };

 public:
  SliderWithNumberInTheMiddle();

  void SetNumber(int number);

  void SetValue(float value);

  void handleAsyncUpdate() override;

  juce::Slider& Get() { return slider_; }

  void resized() override { slider_.setBounds(getLocalBounds()); }

  bool IsUserHolding() const { return is_user_holding_.load(); }

  std::function<void()> on_drag_end_callback_;

 private:
  LookAndFeel look_and_feel_;
  juce::Slider slider_;
  std::atomic<std::optional<float>> value_to_set_;
  std::atomic<bool> is_user_holding_{false};
};
