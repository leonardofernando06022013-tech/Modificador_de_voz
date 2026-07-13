#pragma once
#include "AudioEngine.h"
#include "Integrations/IntegrationProfile.h"
namespace vox {
class AudioRoutingManager {
public:
  explicit AudioRoutingManager(AudioEngine &e) : engine(e) {}
  juce::String apply(const IntegrationProfile &);
  juce::String currentInput() const;
  juce::String currentOutput() const;
  double latencyMs() const;

private:
  AudioEngine &engine;
};
} // namespace vox
