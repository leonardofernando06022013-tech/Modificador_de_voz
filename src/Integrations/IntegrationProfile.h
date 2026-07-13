#pragma once
#include <juce_data_structures/juce_data_structures.h>
namespace vox {
struct IntegrationProfile {
  juce::String name{juce::String::fromUTF8("Padrão")}, target{juce::String::fromUTF8("Padrão")}, audioType, inputDevice,
      virtualOutput, preset;
  double sampleRate = 48000;
  int bufferSize = 256;
  bool monitoring = false;
  float gainDb = 0, gateThreshold = -55;
  bool limiter = true;
  juce::Time lastUsed;
  juce::var toJson() const;
  static IntegrationProfile fromJson(const juce::var &);
};
} // namespace vox
