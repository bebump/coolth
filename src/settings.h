/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include "bbmp/logging.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/atomic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <atomic>
#include <fstream>
#include <mutex>

// >>> SETTINGS / MODEL =======================================================
class CoolthSettings {
 public:
  static constexpr int kNumFans = 4;
  static constexpr int kCurvesPerFan = 2;

  CoolthSettings() : smooth_temps(true) {
    for (auto i_fan = 0u; i_fan < temp_curves.size(); ++i_fan) {
      for (auto i_cpu_or_gpu = 0u; i_cpu_or_gpu < temp_curves[i_fan].size();
           ++i_cpu_or_gpu) {
        temp_curves[i_fan][i_cpu_or_gpu].push_back({60.0f, 40.0f});
      }
    }
  }

  using TTempCurves =
      std::array<std::array<std::vector<juce::Point<float>>, kCurvesPerFan>,
                 kNumFans>;

  void AccessLastComPort(const std::function<void(std::string&)>& accessor) {
    auto lock = std::lock_guard(m_);
    accessor(last_com_port);
  }

  /* [i_fan][i_cpu_or_gpu][i_point] */
  void AccessTempCurves(const std::function<void(TTempCurves&)>& accessor) {
    auto lock = std::lock_guard(m_);
    accessor(temp_curves);
  }

  void Load(juce::File file) {
    juce::String fullPathName;
    {
      auto lock = std::lock_guard(m_);
      file_ = std::move(file);
      if (file_.existsAsFile()) {
        fullPathName = file_.getFullPathName();
      }
    }
    if (!fullPathName.isEmpty()) {
      std::ifstream is(fullPathName.toStdString(), std::ios::binary);
      cereal::BinaryInputArchive archive(is);
      archive(*this);
      should_save_.store(false, std::memory_order_release);
    } else {
      should_save_.store(true, std::memory_order_release);
    }
  }

  void SaveChanges() {
    juce::String fullPathName;
    {
      auto lock = std::lock_guard(m_);
      fullPathName = file_.getFullPathName();
    }
    if (should_save_.load(std::memory_order_acquire)) {
      if (!fullPathName.isEmpty()) {
        std::ofstream os(fullPathName.toStdString(), std::ios::binary);
        cereal::BinaryOutputArchive archive(os);
        archive(*this);
      }
      should_save_.store(false, std::memory_order_relaxed);
    }
  }

  std::atomic<bool> should_save_{false};
  std::array<std::atomic<float>, kNumFans> manual_duty_cycles{0.0f};

  bool GetSmoothTemps() {
    auto lock = std::lock_guard(m_);
    return smooth_temps;
  }

  void SetSmoothTemps(bool value) {
    auto lock = std::lock_guard(m_);
    smooth_temps = value;
    should_save_.store(true);
  }

 private:
  std::string last_com_port;

  // [i_fan][i_cpu_or_gpu][i_point]
  TTempCurves temp_curves;

  bool smooth_temps;

  juce::File file_;
  std::mutex m_;

  friend class cereal::access;

  template <class Archive>
  void serialize(Archive& archive) {
    auto lock = std::lock_guard(m_);
    archive(last_com_port, temp_curves, smooth_temps, manual_duty_cycles);
  }
};

namespace cereal {
template <class Archive>
void save(Archive& archive, juce::Point<float> const& m) {
  archive(m.x, m.y);
}

template <class Archive>
void load(Archive& archive, juce::Point<float>& m) {
  archive(m.x, m.y);
}
}  // namespace cereal
// <<< SETTINGS / MODEL -------------------------------------------------------
