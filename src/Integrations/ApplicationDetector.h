#pragma once

#include <juce_core/juce_core.h>

namespace vox {
class ApplicationDetector {
public:
  static juce::StringArray executablesFor(const juce::String &target);
  static bool isRunning(const juce::String &executable);
  static bool isTargetRunning(const juce::String &target);
  static juce::String statusFor(const juce::String &target);
};
} // namespace vox
