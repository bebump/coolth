/*
Copyright (c) 2021 Attila Szarvas <attila.szarvas@gmail.com>

All rights reserved. Use of this source code is governed by the 3-Clause BSD
License that can be found in the LICENSE file.
*/

#include "main_component.h"

#include "BinaryData.h"

class GuiAppApplication : public juce::JUCEApplication {
 public:
  GuiAppApplication() = default;

  const juce::String getApplicationName() override {
    return JUCE_APPLICATION_NAME_STRING;
  }

  const juce::String getApplicationVersion() override {
    return JUCE_APPLICATION_VERSION_STRING;
  }

  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String& command_line) override {
    juce::ignoreUnused(command_line);
    main_window = std::make_unique<MainWindow>(getApplicationName() + " " +
                                               getApplicationVersion());
  }

  void shutdown() override { main_window = nullptr; }

  void systemRequestedQuit() override {
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app
    // to close.
    quit();
  }

  void anotherInstanceStarted(const juce::String& command_line) override {
    // When another instance of the app is launched while this one is running,
    // this method is invoked, and the command_line parameter tells you what
    // the other instance's command-line arguments were.
    juce::ignoreUnused(command_line);
  }

  struct MySystrayIconComponent : public juce::SystemTrayIconComponent {
   public:
    void mouseUp(const juce::MouseEvent& event) override {
      if (onMouseUp) {
        onMouseUp(event);
      }
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override {
      if (onMouseDoubleClick) {
        onMouseDoubleClick(event);
      }
    }

    std::function<void(const juce::MouseEvent&)> onMouseUp;
    std::function<void(const juce::MouseEvent&)> onMouseDoubleClick;
  };

  //===========================================================================
  /*
      This class implements the desktop window that contains an instance of
      our MainComponent class.
  */
  class MainWindow : public juce::DocumentWindow {
   public:
    explicit MainWindow(juce::String name)
        : DocumentWindow(
              name,
              juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                  ResizableWindow::backgroundColourId),
              DocumentWindow::closeButton | DocumentWindow::minimiseButton) {
      setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
      setFullScreen(true);
#else
      setResizable(false, true);
      centreWithSize(getWidth(), getHeight());
#endif

      auto systray_image = juce::ImageFileFormat::loadFrom(
          BinaryData::systray_png, BinaryData::systray_pngSize);
      systray_component_.setIconImage(systray_image, systray_image);
      systray_component_.onMouseUp = [this](const juce::MouseEvent& event) {
        const auto cursor_position_float = juce::Desktop::getInstance()
                                               .getMainMouseSource()
                                               .getScreenPosition();
        addToDesktop();
        toFront(true);
      };
      removeFromDesktop();
      setVisible(true);
    }

    void closeButtonPressed() override {
      // This is called when the user tries to close this window. Here, we'll
      // just ask the app to quit when this happens, but you can change this to
      // do whatever you need.
      JUCEApplication::getInstance()->systemRequestedQuit();
    }

    virtual void minimiseButtonPressed() override { removeFromDesktop(); }

    /* Note: Be careful if you override any DocumentWindow methods - the base
       class uses a lot of them, so by overriding you might break its
       functionality. It's best to do all your work in your content component
       instead, but if you really have to override any DocumentWindow methods,
       make sure your subclass also calls the superclass's method.
    */

   private:
    MySystrayIconComponent systray_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

 private:
  std::unique_ptr<MainWindow> main_window;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(GuiAppApplication)
