#pragma once
#include "Audio/AudioEngine.h"
#include "VirtualDeviceDetector.h"
namespace vox {
struct RoutingDiagnosticResult {
  bool success = false;
  juce::String headline, details;
};
class RoutingDiagnostics {
public:
  static RoutingDiagnosticResult run(AudioEngine &,
                                     const juce::Array<DetectedAudioDevice> &);
};
} // namespace vox
