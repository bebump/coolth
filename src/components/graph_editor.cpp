/*
Copyright (c) 2020 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "graph_editor.h"

// >>> MODEL ==================================================================
void GraphComponent::Model::Clear() {
  points_.clear();
  sorted_indices_.clear();
  sorted_ = true;
}

int GraphComponent::Model::Add(const juce::Point<float>& p) {
  points_.emplace_back(p);
  const auto index = points_.size() - 1;
  sorted_indices_.push_back(index);
  sorted_ = false;
  return index;
}

void GraphComponent::Model::SortByX() {
  if (!sorted_) {
    std::sort(sorted_indices_.begin(), sorted_indices_.end(),
              [this](const size_t& a, const size_t& b) {
                return points_[a].getX() < points_[b].getX();
              });
  }
  sorted_ = true;
}

void GraphComponent::Model::Modify(size_t index,
                                   const juce::Point<float> new_position) {
  points_[index] = new_position;
  sorted_ = false;
}

void GraphComponent::Model::Delete(size_t index) {
  points_.erase(points_.begin() + index);
  sorted_indices_.resize(sorted_indices_.size() - 1);
  for (size_t i = 0; i < points_.size(); ++i) {
    sorted_indices_[i] = i;
  }
  sorted_ = false;
}

int GraphComponent::Model::HitTest(const juce::Point<float>& pointer,
                                   float distance) const {
  for (size_t i = 0; i < points_.size(); ++i) {
    if (points_[i].getDistanceFrom(pointer) < distance) {
      return i;
    }
  }

  return -1;
}

void GraphComponent::Model::IterateOverOrderedPoints(
    const std::function<void(const juce::Point<float>&)>& accessor) {
  SortByX();
  for (auto index : sorted_indices_) {
    accessor(points_[index]);
  }
}
// <<< MODEL ------------------------------------------------------------------

void GraphComponent::paint(juce::Graphics& g) {
  const auto local_bounds = getLocalBounds().reduced(padding_);
  const Transform transform(local_bounds);

  const auto thumb_color =
      colour_.value_or(findColour(juce::Slider::thumbColourId));
  const auto line_color = thumb_color.darker();
  juce::Path path;

  // >>> CONSTRUCT PATH =====================================================
  {
    juce::Point<float> last_point_added;
    const auto left = static_cast<float>(local_bounds.getX());
    const auto right = static_cast<float>(local_bounds.getRight());
    {
      auto lock = std::lock_guard(model_mutex_);
      model_.IterateOverOrderedPoints([&](const juce::Point<float>& point) {
        if (path.isEmpty()) {
          path.startNewSubPath({left, transform.YToImage(point)});
        }
        path.lineTo(transform.ToImage(point));
        last_point_added = point;
      });
    }
    if (!path.isEmpty()) {
      path.lineTo({right, transform.YToImage(last_point_added)});
    }
  }
  // <<< CONSTRUCT PATH -----------------------------------------------------

  const float line_width = 8.0f;
  g.setColour(line_color);
  g.strokePath(path,
               juce::PathStrokeType(line_width, juce::PathStrokeType::curved));

  g.setColour(thumb_color);
  if (enabled_) {
    {
      auto lock = std::lock_guard(model_mutex_);
      model_.IterateOverOrderedPoints([&](const juce::Point<float>& point) {
        g.fillEllipse(
            juce::Rectangle<float>(line_width * 2.0f, line_width * 2.0f)
                .withCentre(transform.ToImage(point)));
      });
    }
  }
}

void GraphComponent::mouseDown(const juce::MouseEvent& e) {
  const float hittest_tolerance = 18.0f;
  const auto local_bounds = getLocalBounds().reduced(padding_);
  const Transform transform(local_bounds);

  if (e.mods.isLeftButtonDown()) {
    mouse_down_ = true;

    position_held = model_.HitTest(
        transform.FromImage(
            local_bounds.toFloat().getConstrainedPoint(e.position)),
        transform.FromImage(hittest_tolerance));
    if (position_held == -1) {
      auto lock = std::lock_guard(model_mutex_);
      position_held = model_.Add(transform.FromImage(
          local_bounds.toFloat().getConstrainedPoint(e.position)));
    }
  } else if (e.mods.isRightButtonDown()) {
    auto maybe_point = model_.HitTest(
        transform.FromImage(
            local_bounds.toFloat().getConstrainedPoint(e.position)),
        transform.FromImage(hittest_tolerance));
    if (maybe_point != -1) {
      auto lock = std::lock_guard(model_mutex_);
      model_.Delete(maybe_point);
    }
  }
  repaint();
}

void GraphComponent::mouseUp(const juce::MouseEvent&) {
  mouse_down_ = false;
  {
    auto lock = std::lock_guard(on_change_mutex_);
    if (on_change_callback_) {
      model_.SortByX();
      on_change_callback_(model_);
    }
  }
}

void GraphComponent::mouseDrag(const juce::MouseEvent& e) {
  const auto local_bounds = getLocalBounds().reduced(padding_);
  const Transform transform(local_bounds);
  if (mouse_down_) {
    {
      auto lock = std::lock_guard(model_mutex_);
      model_.Modify(
          position_held,
          transform.FromImage(
              local_bounds.toFloat().getConstrainedPoint(e.position)));
    }
    repaint();
  }
}

void GraphComponent::AccessModel(const std::function<void(Model&)>& accessor) {
  auto lock = std::lock_guard(model_mutex_);
  accessor(model_);
}
