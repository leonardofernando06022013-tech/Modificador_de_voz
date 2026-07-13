#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
namespace vox {
struct DetectedAudioDevice {
  juce::String name, type;
  bool input = false, virtualDevice = false, current = false;
};
class VirtualDeviceDetector {
public:
  static juce::Array<DetectedAudioDevice> scan(juce::AudioDeviceManager &);
  static bool isVirtual(const juce::String &);
  static juce::StringArray inputs(const juce::Array<DetectedAudioDevice> &);
  static juce::StringArray
  virtualOutputs(const juce::Array<DetectedAudioDevice> &);
};
} // namespace vox
