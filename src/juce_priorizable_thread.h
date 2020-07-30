/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>

struct JucePriorizableThread : public juce::Thread {
  JucePriorizableThread(std::function<void(const std::function<bool()>&,
                                           const std::function<void(int)>&)>
                            runnable)
      : juce::Thread("JucePriorizableThread"), runnable_(std::move(runnable)) {}

  JucePriorizableThread(const juce::String& name,
                        std::function<void(const std::function<bool()>&,
                                           const std::function<void(int)>&)>
                            runnable)
      : juce::Thread(name), runnable_(std::move(runnable)) {}

  void run() {
    runnable_([this]() { return threadShouldExit(); },
              [this](int millisec) { wait(millisec); });
  }

  void startThread() { juce::Thread::startThread(); }

  // 0 is lowest and 10 is the highest thread priority
  void startThread(const int priority) { juce::Thread::startThread(priority); }

  ~JucePriorizableThread() { stopThread(5000); }

  void notify() { notify(); }

  std::function<void(const std::function<bool()>&,
                     const std::function<void(int)>&)>
      runnable_;
};
