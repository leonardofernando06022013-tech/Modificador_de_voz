#pragma once
#include <juce_core/juce_core.h>
namespace vox {
struct WindowsAudioSettingsLauncher {
  static void sound() {
    juce::URL("ms-settings:sound").launchInDefaultBrowser();
  }
  static void microphonePrivacy() {
    juce::URL("ms-settings:privacy-microphone").launchInDefaultBrowser();
  }
  static void classicPanel() {
    juce::ChildProcess p;
    p.start("control.exe mmsys.cpl");
  }
};
} // namespace vox
