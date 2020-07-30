/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "multi_graph_editor.h"

MultiGraphComponent::MultiGraphComponent(float xmin, float xmax, float ymin,
                                         float ymax,
                                         std::vector<juce::String> legend)
    : xmin_(xmin),
      xmax_(xmax),
      ymin_(ymin),
      ymax_(ymax),
      legend_(std::move(legend)) {
  colours_.push_back(findColour(juce::Slider::thumbColourId));
  float color_shift = 0.25f;

  const auto num_graphs = legend_.size() == 0 ? 1 : legend_.size();
  for (int i = 0; i < num_graphs; ++i) {
    graphs_.push_back(std::make_unique<GraphComponent>());
    graphs_.back()->SetOnChange(
        [this, i_graph = i](GraphComponent::Model& model) {
          OnChangeCallback(i_graph, model);
        });
    addAndMakeVisible(*graphs_.back());

    if (i > 0) {
      if (i % 2 == 0) {
        colours_.push_back(colours_.back().withRotatedHue(color_shift));
        color_shift /= 2.0f;
      } else {
        colours_.push_back(colours_.back().withRotatedHue(0.5f));
      }
    }
    graphs_.back()->SetColor(colours_.back());
  }
  if (graphs_.size() > 1) {
    for (size_t i = 0; i < graphs_.size(); ++i) {
      graph_selector_buttons_.push_back(
          std::make_unique<juce::TextButton>(legend_[i]));
      addAndMakeVisible(*graph_selector_buttons_.back());
      graph_selector_buttons_.back()->setColour(
          juce::TextButton::ColourIds::textColourOffId, colours_[i]);
      graph_selector_buttons_.back()->onClick = [this, i]() { EnableGraph(i); };
    }
  }
  EnableGraph(0);
}

void MultiGraphComponent::EnableGraph(size_t i_graph) {
  for (size_t i = 0; i < graphs_.size(); ++i) {
    graphs_[i]->SetEnabled(i == i_graph);
    if (i == i_graph) {
      graphs_[i]->toFront(true);
    }
    graph_selector_buttons_[i]->setEnabled(i != i_graph);
    if (legend_.size() > 0) {
      graph_selector_buttons_[i]->setButtonText(
          i == i_graph ? "Editing: " + legend_[i] : "Edit: " + legend_[i]);
    }
  }
}

void MultiGraphComponent::OnChangeCallback(size_t i_graph,
                                           GraphComponent::Model& model) {
  auto lock = std::lock_guard(on_change_mutex_);
  if (on_change_callback_) {
    std::vector<juce::Point<float>> denormalized_points;

    model.IterateOverOrderedPoints(
        [&denormalized_points, this](const juce::Point<float>& p) {
          denormalized_points.push_back(Denormalize(p));
        });

    on_change_callback_(i_graph, std::move(denormalized_points));
  }
}

void MultiGraphComponent::resized() {
  auto area = getLocalBounds();
  area.removeFromLeft(30);
  area.removeFromBottom(40);

  if (graphs_.size() > 1) {
    auto edit_graph_buttons_area = area.removeFromTop(40);
    edit_graph_buttons_area.removeFromTop(10);
    edit_graph_buttons_area.removeFromRight(10);
    for (int i = static_cast<int>(graph_selector_buttons_.size()) - 1; i >= 0;
         --i) {
      graph_selector_buttons_[i]->setBounds(
          edit_graph_buttons_area.removeFromRight(100));
      edit_graph_buttons_area.removeFromRight(10);
    }
  }

  area = area.reduced(12);
  for (auto& graph : graphs_) {
    graph->setBounds(area);
  }
}

void MultiGraphComponent::paint(juce::Graphics& g) {
  const auto graph_bounds = graphs_[0]->GetDrawableAreaInParent().toFloat();
  auto left = static_cast<float>(graph_bounds.getX());
  auto right = static_cast<float>(graph_bounds.getRight());

  auto top = static_cast<float>(graph_bounds.getY());
  const auto bottom = static_cast<float>(graph_bounds.getBottom());

  g.setColour(findColour(juce::Slider::textBoxOutlineColourId));
  const int font_size = g.getCurrentFont().getHeightInPoints();
  const int label_width = 4 * font_size;
  const float line_spacing = 40.0;

  const auto width = static_cast<float>(graph_bounds.getWidth());
  const auto height = static_cast<float>(graph_bounds.getHeight());

  {
    const float vertical_line_spacing =
        width > height ? line_spacing : line_spacing * (height / width);
    float f = 0.0f;
    auto draw_horizontal_line_and_text_at = [&](float ypos,
                                                std::optional<float> value) {
      g.drawLine(left, bottom - ypos, right, bottom - ypos, 1.0f);
      g.drawText(
          juce::String(value.value_or(ymin_ + ypos / height * (ymax_ - ymin_)),
                       0, false),
          left - label_width - font_size, bottom - ypos - font_size * 0.4f,
          label_width, font_size, juce::Justification::centredRight);
    };

    if (yticks_.size() > 0) {
      for (auto v : yticks_) {
        draw_horizontal_line_and_text_at((v - ymin_) / (ymax_ - ymin_) * height,
                                         v);
      }
    } else {
      for (; f <= height - vertical_line_spacing * 0.6;) {
        draw_horizontal_line_and_text_at(f, {});
        f += vertical_line_spacing;
      }

      draw_horizontal_line_and_text_at(height, {});
    }
  }

  {
    const float horizontal_line_spacing =
        height > width ? line_spacing : line_spacing * (width / height);
    float f = 0.0f;

    auto draw_vertical_line_and_text_at = [&](float xpos,
                                              std::optional<float> value) {
      g.drawLine(left + xpos, bottom, left + xpos, top, 1.0f);
      g.drawText(
          juce::String(value.value_or(xmin_ + xpos / width * (xmax_ - xmin_)),
                       0, false),
          left + xpos - label_width * 0.5, bottom + font_size, label_width,
          font_size, juce::Justification::centred);
    };

    if (xticks_.size() > 0) {
      for (auto v : xticks_) {
        draw_vertical_line_and_text_at((v - xmin_) / (xmax_ - xmin_) * width,
                                       v);
      }
    } else {
      for (; f <= width - horizontal_line_spacing * 0.5;) {
        draw_vertical_line_and_text_at(f, {});
        f += horizontal_line_spacing;
      }

      draw_vertical_line_and_text_at(width, {});
    }
  }

  if (!xlabel_.isEmpty()) {
    const auto label_width = xlabel_.length() * font_size;
    g.drawText(xlabel_, left + (right - left) / 2.0f - label_width * 0.5,
               bottom + 3.0f * font_size, label_width, font_size,
               juce::Justification::centred);
  }

  if (!ylabel_.isEmpty()) {
    juce::Graphics::ScopedSaveState state(g);
    const auto ylabel_width = static_cast<float>(8 * ylabel_.length());
    juce::Point<float> ylabel_centre(left - 3 * font_size,
                                     bottom - (bottom - top) / 2.0f);
    g.addTransform(juce::AffineTransform::rotation(
        juce::MathConstants<float>::halfPi, ylabel_centre.getX(),
        ylabel_centre.getY()));
    g.drawText(ylabel_, ylabel_centre.getX() - ylabel_width / 2.0f,
               ylabel_centre.getY(), ylabel_width, font_size,
               juce::Justification::centred);
    const auto label_width = xlabel_.length() * font_size;
  }

  auto asdf = [this, width, height](const juce::Point<float>& p) {
    return juce::Point<float>{(p.getX() - xmin_) / (xmax_ - xmin_) * width,
                              -((p.getY() - ymin_) / (ymax_ - ymin_) * height)};
  };

  if (const auto maybe_point = point_.load()) {
    g.fillEllipse(juce::Rectangle<float>(30, 30).withCentre(
        graph_bounds.getBottomLeft() + asdf(*maybe_point)));
  }
}

juce::Point<float> MultiGraphComponent::Normalize(
    const juce::Point<float>& value) {
  return {(value.getX() - xmin_) / (xmax_ - xmin_),
          (value.getY() - ymin_) / (ymax_ - ymin_)};
}

juce::Point<float> MultiGraphComponent::Denormalize(
    const juce::Point<float>& value) {
  return {value.getX() * (xmax_ - xmin_) + xmin_,
          value.getY() * (ymax_ - ymin_) + ymin_};
}

void MultiGraphComponent::SetOnChange(
    std::function<void(size_t, std::vector<juce::Point<float>>)> callback) {
  auto lock = std::lock_guard(on_change_mutex_);
  on_change_callback_ = std::move(callback);
}
