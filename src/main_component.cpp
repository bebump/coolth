/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "main_component.h"
#include <BinaryData.h>

MainComponent::MainComponent()
    : log_component_(
          [] {
            bbmp::LogMessage message;
            if (bbmp::NonBlockingLogger::GetInstance().TryDequeue(message)) {
              return std::make_optional<bbmp::LogMessage>(std::move(message));
            }
            return std::optional<bbmp::LogMessage>{};
          },
          false),
      button_log_("Show log >"),
      temperature_thread_(
          [this](const std::function<bool()>& thread_should_exit,
                 const std::function<void(int)>& wait_ms) {
            program_loop(thread_should_exit, wait_ms);
          }),
      tabs_(juce::TabbedButtonBar::Orientation::TabsAtBottom),
      button_info_("info") {
  addAndMakeVisible(temperature_component_);
  addAndMakeVisible(button_log_);
  addAndMakeVisible(slider_component_);
  addAndMakeVisible(log_component_);
  addAndMakeVisible(tabs_);
  addAndMakeVisible(button_info_);

  button_info_.setImages(
      false, true, true,
      juce::ImageFileFormat::loadFrom(BinaryData::button_info_png,
                                      BinaryData::button_info_pngSize),
      1.0f, juce::Colours::transparentWhite, juce::Image(), 1.0f,
      juce::Colours::transparentWhite, juce::Image(), 1.0f,
      juce::Colours::transparentWhite);
  button_info_.onClick = []() {
    juce::AlertWindow alert_window(
        "About Bebump Coolth",
        "Copyright 2020-2021 (c) Attila Szarvas <attila.szarvas@gmail.com>\n\n"
        "https://github.com/bebump/coolth\n\n\n"
        "Temperatures are read using Open Hardware Monitor 0.9.6\n\n"
        "https://openhardwaremonitor.org",
        juce::AlertWindow::AlertIconType::InfoIcon, nullptr);
    alert_window.addButton("Ok", 0);
    alert_window.runModalLoop();
  };

  auto set_size = [this]() {
    if (show_log_) {
      setSize(450 + log_width, 580);
    } else {
      setSize(450, 580);
    }
  };

  button_log_.onClick = [this, set_size]() {
    show_log_ = !show_log_;
    button_log_.setButtonText(show_log_ ? "Hide log <" : "Show log >");
    set_size();
  };

  for (auto i_fan = 0u; i_fan < fan_graphs_.size(); ++i_fan) {
    fan_graphs_[i_fan] = std::make_unique<MultiGraphComponent>(
        30.0f, 90.0f, 0.0f, 100.0f, std::vector<juce::String>{"CPU", "GPU"});
    auto& graph = fan_graphs_[i_fan];
    graph->SetXTicks({30, 40, 50, 60, 70, 80, 90});
    graph->SetYTicks({0, 20, 40, 60, 80, 100});
    graph->SetXLabel(
        juce::CharPointer_UTF8("T [\xc2\xb0"
                               "C]"));
    graph->SetYLabel("Duty cycle [%]");
    graph->SetOnChange(
        [this, i_fan](size_t i_cpu_or_gpu,
                      std::vector<juce::Point<float>> new_values) {
          settings_.AccessTempCurves([i_fan, i_cpu_or_gpu, &new_values](
                                         CoolthSettings::TTempCurves& curves) {
            if (curves.size() > i_fan && curves[i_fan].size() > i_cpu_or_gpu) {
              curves[i_fan][i_cpu_or_gpu] = std::move(new_values);
            }
          });
          settings_.should_save_.store(true, std::memory_order_release);
        });
    tabs_.addTab(
        "Fan " + juce::String(i_fan),
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        fan_graphs_[i_fan].get(), false);
  }

  auto settings_file_dir =
      juce::File::getSpecialLocation(
          juce::File::SpecialLocationType::userApplicationDataDirectory)
          .getChildFile(
              juce::JUCEApplication::getInstance()->getApplicationName());
  if (!settings_file_dir.exists()) {
    settings_file_dir.createDirectory();
  }
  auto settings_file = settings_file_dir.getChildFile("settings.bin");

  try {
    settings_.Load(settings_file);
    bbmp::Log({"Loaded settings from: " +
               settings_file.getFullPathName().toStdString()});
  } catch (...) {
    bbmp::Log({"Failed to load settings from: " +
               settings_file.getFullPathName().toStdString()});
  }

  {
    CoolthSettings::TTempCurves curves;
    settings_.AccessTempCurves([&curves](auto& c) { curves = c; });
    const auto i_fan_end = std::min(curves.size(), fan_graphs_.size());
    for (auto i_fan = 0u; i_fan < i_fan_end; ++i_fan) {
      fan_graphs_[i_fan]->SetState(curves[i_fan]);
    }
  }

  temperature_component_.SetAverage(settings_.GetSmoothTemps());
  temperature_component_.button_average_.onClick =
      [&button = temperature_component_.button_average_, this]() {
        settings_.SetSmoothTemps(button.getToggleState());
      };

  {
    static_assert(
        std::tuple_size<decltype(settings_.manual_duty_cycles)>::value ==
        std::tuple_size<decltype(slider_component_.sliders_)>::value);

    for (auto i_fan = 0u; i_fan < slider_component_.sliders_.size(); ++i_fan) {
      slider_component_.sliders_[i_fan]->Get().setValue(
          settings_.manual_duty_cycles[i_fan].load());

      slider_component_.sliders_[i_fan]->on_drag_end_callback_ =
          [&settings = settings_,
           &slider = slider_component_.sliders_[i_fan]->Get(), i_fan] {
            settings.manual_duty_cycles[i_fan].store(slider.getValue());
            settings.should_save_.store(true, std::memory_order_release);
          };
    }
  }

  set_size();
  temperature_thread_.startThread(0);
}

void MainComponent::paint(juce::Graphics& g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() {
  auto local_bounds = getLocalBounds();
  if (show_log_) {
    log_component_.setVisible(true);
    log_component_.setBounds(local_bounds.removeFromRight(log_width));
  } else {
    log_component_.setVisible(false);
    auto top_row = local_bounds.removeFromTop(46).reduced(8);
    button_log_.setBounds(top_row.removeFromRight(88));
    temperature_component_.setBounds(top_row);
    slider_component_.setBounds(local_bounds.removeFromTop(110));
    local_bounds.removeFromTop(16);
    tabs_.setBounds(local_bounds);
  }
  button_info_.setBounds(
      getLocalBounds().removeFromRight(30).removeFromBottom(30).reduced(8));
}

void MainComponent::program_loop(
    const std::function<bool()>& thread_should_exit,
    const std::function<void(int)>& wait_ms) {
  juce::int64 time = 0;

  while (!thread_should_exit()) {
    try {
      FanControllerCommunicator fan_controller_communicator(
          settings_, thread_should_exit, wait_ms);

      LineReader temp_stream_reader(
          32, [this](const char* data, size_t length) {
            const auto AsFloat =
                [](const std::optional<int>& opt) -> std::optional<float> {
              return opt ? std::make_optional<float>(
                               static_cast<float>(opt.value()))
                         : std::nullopt;
            };

            StringStream stream(data, length);
            std::optional<float> cpu = AsFloat(stream.GetInt());
            if (!cpu) {
              stream.ForwardToNextToken();
            }
            std::optional<float> gpu = AsFloat(stream.GetInt());
            temperature_component_.SetTemps(cpu, gpu);
          });

      RecreateOnFailure<ChildProcess> temp_reader_process{
          [this, &temp_stream_reader]() {
            auto temp_reader_path = juce::File::getSpecialLocation(
                                        juce::File::currentExecutableFile)
                                        .getParentDirectory()
                                        .getChildFile("temperature_reader.exe");

            return std::make_unique<ChildProcess>(
                temp_reader_path.getFullPathName().toStdString(),
                [this, &temp_stream_reader](const char* data, size_t length) {
                  temp_stream_reader.Read(data, length);
                });
          }};

      std::array<std::optional<float>, CoolthSettings::kCurvesPerFan> temps;

      while (!thread_should_exit()) {
        temp_reader_process.Execute([](auto& p) { p.IssueRead(); });
        fan_controller_communicator.IssueRead();
        const auto current_time = juce::Time::currentTimeMillis();
        if (current_time - time > 1000) {
          // >>> AUTO DUTY CYCLE LOGIC ======================================
          const auto smooth_temps = temperature_component_.GetAverage();
          const auto cpu_temp =
              temperature_component_.cpu_temp_.load(std::memory_order_relaxed);
          const auto gpu_temp =
              temperature_component_.gpu_temp_.load(std::memory_order_relaxed);

          temps[0] = (temps[0] && cpu_temp && smooth_temps)
                         ? 0.1f * cpu_temp.value() + 0.9f * temps[0].value()
                         : cpu_temp;
          temps[1] = (temps[1] && gpu_temp && smooth_temps)
                         ? 0.1f * gpu_temp.value() + 0.9f * temps[1].value()
                         : gpu_temp;

          if (temps[0]) {
            temperature_component_.SetCpuDisplay(
                smooth_temps ? juce::String(temps[0].value(), 1, false)
                             : juce::String(temps[0].value()));
          } else {
            temperature_component_.SetCpuDisplay("N/A");
          }

          if (temps[1]) {
            temperature_component_.SetGpuDisplay(
                smooth_temps ? juce::String(temps[1].value(), 1, false)
                             : juce::String(temps[1].value()));
          } else {
            temperature_component_.SetGpuDisplay("N/A");
          }

          std::array<std::optional<juce::Point<float>>,
                     CoolthSettings::kNumFans>
              duty_point_on_graph;

          std::array<float, CoolthSettings::kNumFans> duty_cycles{0.0f};

          settings_.AccessTempCurves(
              [this, &temps, &duty_point_on_graph,
               &duty_cycles](CoolthSettings::TTempCurves& curves) {
                for (auto i_fan = 0u; i_fan < curves.size(); ++i_fan) {
                  auto& duty_cycle = duty_cycles[i_fan];
                  for (auto i_cpu_or_gpu = 0u; i_cpu_or_gpu < temps.size();
                       ++i_cpu_or_gpu) {
                    if (auto temp = temps[i_cpu_or_gpu]) {
                      if (auto dc = GraphComponent::GetYForX(
                              curves[i_fan][i_cpu_or_gpu], *temp)) {
                        if (*dc >= duty_cycle) {
                          duty_cycle = *dc;
                          duty_point_on_graph[i_fan] = {
                              static_cast<float>(*temp), duty_cycle};
                        }
                      }
                    }
                  }
                }
              });

          static_assert(
              std::tuple_size<decltype(slider_component_.sliders_)>::value ==
              std::tuple_size<decltype(duty_cycles)>::value);

          for (auto i_fan = 0u; i_fan < duty_cycles.size(); ++i_fan) {
            auto& duty_cycle = duty_cycles[i_fan];
            if (!duty_point_on_graph[i_fan] ||
                slider_component_.sliders_[i_fan]->IsUserHolding()) {
              duty_cycle = slider_component_.sliders_[i_fan]->Get().getValue();
              duty_point_on_graph[i_fan] = {};
            } else {
              slider_component_.sliders_[i_fan]->SetValue(duty_cycle);
            }

            if (!duty_point_on_graph[i_fan] &&
                !slider_component_.sliders_[i_fan]->IsUserHolding()) {
              slider_component_.sliders_[i_fan]->SetValue(
                  settings_.manual_duty_cycles[i_fan]);
            }

            fan_graphs_[i_fan]->SetPoint(duty_point_on_graph[i_fan]);
          }

          // <<< AUTO DUTY CYCLE LOGIC
          // --------------------------------------

          const auto rpms = fan_controller_communicator.GetRpms();

          static_assert(
              std::tuple_size<decltype(rpms)>::value ==
              std::tuple_size<decltype(slider_component_.sliders_)>::value);

          for (int i_fan = 0; i_fan < rpms.size(); ++i_fan) {
            slider_component_.sliders_[i_fan]->SetNumber(rpms[i_fan]);
          }
          char msg[100];
          snprintf(
              msg, 100, "c 1 %d %d %d %d\n",
              static_cast<int>(std::round(duty_cycles[0] / 100.0f * 255.0f)),
              static_cast<int>(std::round(duty_cycles[1] / 100.0f * 255.0f)),
              static_cast<int>(std::round(duty_cycles[2] / 100.0f * 255.0f)),
              static_cast<int>(std::round(duty_cycles[3] / 100.0f * 255.0f)));
          fan_controller_communicator.serial_->TryIssueWrite(msg, strlen(msg));
          time = current_time;
        }
        settings_.SaveChanges();

        WindowsSleepEx(50, true);
      }
    } catch (std::runtime_error& error) {
      bbmp::Log({error.what()});
    }
    wait_ms(1000);
  }
  try {
    settings_.SaveChanges();
  } catch (std::runtime_error& error) {
    bbmp::Log({error.what()});
  }
}

SliderComponent::SliderComponent() {
  for (auto& slider : sliders_) {
    slider = std::make_unique<SliderWithNumberInTheMiddle>();
    slider->Get().setRange(0.0, 100.0, 1.0 / 255.0);
    slider->Get().setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(*slider);
  }
}

void SliderComponent::resized() {
  auto local_bounds = getLocalBounds();
  const int slider_width = local_bounds.getWidth() / sliders_.size();
  for (auto& slider : sliders_) {
    slider->setBounds(local_bounds.removeFromLeft(slider_width));
  }
}

TemperatureComponent::TemperatureComponent()
    : button_average_("Average temps"),
      button_average_accessor_(button_average_),
      cpu_display_accesor_(cpu_display_),
      gpu_display_accesor_(gpu_display_) {
  addAndMakeVisible(cpu_);
  addAndMakeVisible(gpu_);
  addAndMakeVisible(button_average_);

  cpu_.setText("CPU: N/A", juce::NotificationType::dontSendNotification);
  gpu_.setText("GPU: N/A", juce::NotificationType::dontSendNotification);
}

void TemperatureComponent::resized() {
  auto local_bounds = getLocalBounds();
  cpu_.setBounds(local_bounds.removeFromLeft(100));
  gpu_.setBounds(local_bounds.removeFromLeft(100));
  button_average_.setBounds(local_bounds.removeFromLeft(150));
}

void TemperatureComponent::SetTemps(const std::optional<float>& cpu,
                                    const std::optional<float>& gpu) {
  if (cpu_temp_.load(std::memory_order_relaxed) != cpu) {
    cpu_temp_.store(cpu);
    repaint_.store(true, std::memory_order_relaxed);
    triggerAsyncUpdate();
  }

  if (gpu_temp_.load(std::memory_order_relaxed) != gpu) {
    gpu_temp_.store(gpu);
    repaint_.store(true, std::memory_order_relaxed);
    triggerAsyncUpdate();
  }
}
