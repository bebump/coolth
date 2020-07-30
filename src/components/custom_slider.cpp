/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "custom_slider.h"

void SliderWithNumberInTheMiddle::LookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle,
    juce::Slider& slider) {
  auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
  auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);

  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  auto toAngle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
  auto lineW = juce::jmin(8.0f, radius * 0.5f);
  auto arcRadius = radius - lineW * 0.5f;

  juce::Path backgroundArc;
  backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                              arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                              rotaryEndAngle, true);

  g.setColour(outline);
  g.strokePath(backgroundArc,
               juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  g.setColour(slider.findColour(juce::Slider::textBoxTextColourId));
  g.drawText(
      juce::String(number_in_the_middle_.load(std::memory_order_relaxed)),
      bounds, juce::Justification::centred);

  if (slider.isEnabled()) {
    juce::Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius,
                           arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

    g.setColour(fill);
    g.strokePath(valueArc,
                 juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }

  auto thumbWidth = lineW * 2.0f;
  juce::Point<float> thumbPoint(
      bounds.getCentreX() +
          arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
      bounds.getCentreY() +
          arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));

  g.setColour(slider.findColour(juce::Slider::thumbColourId));
  g.fillEllipse(
      juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

SliderWithNumberInTheMiddle::SliderWithNumberInTheMiddle()
    : slider_(juce::Slider::SliderStyle::RotaryVerticalDrag,
              juce::Slider::TextEntryBoxPosition::TextBoxBelow) {
  slider_.setLookAndFeel(&look_and_feel_);
  addAndMakeVisible(slider_);
  slider_.onDragStart = [this]() { is_user_holding_.store(true); };
  slider_.onDragEnd = [this]() {
    is_user_holding_.store(false);
    if (on_drag_end_callback_) {
      on_drag_end_callback_();
    }
  };
}

void SliderWithNumberInTheMiddle::SetNumber(int number) {
  look_and_feel_.number_in_the_middle_.store(number, std::memory_order_relaxed);
  triggerAsyncUpdate();
}

void SliderWithNumberInTheMiddle::SetValue(float value) {
  if (!is_user_holding_.load()) {
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      slider_.setValue(value, juce::NotificationType::dontSendNotification);
    } else {
      value_to_set_.store(value);
      triggerAsyncUpdate();
    }
  }
}

void SliderWithNumberInTheMiddle::handleAsyncUpdate() {
  if (const auto value_to_set = value_to_set_.load()) {
    slider_.setValue(*value_to_set);
    value_to_set_.store({});
  }
  repaint();
}
