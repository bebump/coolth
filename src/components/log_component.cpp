/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "log_component.h"

LogComponent::LogComponent(
    std::function<std::optional<bbmp::LogMessage>()> log_message_getter,
    bool has_hide_button)
    : log_message_getter_(std::move(log_message_getter)), message_counter_(0) {
  editor_.setColour(
      juce::TextEditor::backgroundColourId,
      editor_.findColour(juce::TextEditor::backgroundColourId).withAlpha(0.7f));
  editor_.setMultiLine(true, true);
  editor_.setReadOnly(true);
  addAndMakeVisible(editor_);

  if (has_hide_button) {
    button_show_hide_ = std::make_unique<juce::TextButton>("Log");
    button_show_hide_->setTooltip("Show/hide log");
    button_show_hide_->onClick = [this]() {
      const auto isVisible = editor_.isVisible();
      editor_.setVisible(!isVisible);
      setInterceptsMouseClicks(!isVisible, true);
      editor_.setInterceptsMouseClicks(!isVisible, !isVisible);
    };
    addAndMakeVisible(*button_show_hide_);
  }

  startTimerHz(10);
}

void LogComponent::Update() {
  bool new_message_arrived = false;
  while (auto messageOpt = log_message_getter_()) {
    bbmp::LogMessage message{std::move(*messageOpt)};
    if (message.message != nullptr) {
      log_messages_.add(juce::String(message_counter_++) + ">" +
                        *message.message);
    }
    if (message.heap_message != nullptr) {
      log_messages_.add(juce::String(message_counter_++) + ">" +
                        *message.heap_message);
    }
    new_message_arrived = true;
  }
  if (!new_message_arrived) {
    return;
  }

  const auto excess_messages = log_messages_.size() - 50;
  if (excess_messages > 0) {
    log_messages_.removeRange(0, excess_messages);
  }

  const auto last_messages_string = log_messages_.joinIntoString("\n");
  editor_.clear();
  editor_.setText(last_messages_string);
  editor_.moveCaretToEnd();
  editor_.scrollEditorToPositionCaret(0, 0);
}

void LogComponent::resized() {
  auto editorBounds = getLocalBounds().reduced(4);
  editor_.setBounds(editorBounds);

  if (button_show_hide_)
    button_show_hide_->setBounds(
        editorBounds.removeFromRight(30).removeFromTop(30).reduced(4));
}

void LogComponent::timerCallback() { Update(); }
