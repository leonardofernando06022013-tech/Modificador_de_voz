#include "AudioRoutingManager.h"
namespace vox {
juce::String AudioRoutingManager::apply(const IntegrationProfile &p) {
  auto &m = engine.deviceManager();
  if (p.audioType.isNotEmpty() && m.getCurrentAudioDeviceType() != p.audioType)
    m.setCurrentAudioDeviceType(p.audioType, true);
  auto setup = m.getAudioDeviceSetup();
  if (p.inputDevice.isNotEmpty())
    setup.inputDeviceName = p.inputDevice;
  if (p.virtualOutput.isNotEmpty())
    setup.outputDeviceName = p.virtualOutput;
  if (p.sampleRate > 0)
    setup.sampleRate = p.sampleRate;
  if (p.bufferSize > 0)
    setup.bufferSize = p.bufferSize;
  const auto error = m.setAudioDeviceSetup(setup, true);
  if (error.isEmpty()) {
    engine.parameters().inputGainDb = p.gainDb;
    engine.parameters().gateThreshold = p.gateThreshold;
    engine.parameters().limiterEnabled = p.limiter;
  }
  return error;
}
juce::String AudioRoutingManager::currentInput() const {
  return engine.deviceManager().getAudioDeviceSetup().inputDeviceName;
}
juce::String AudioRoutingManager::currentOutput() const {
  return engine.deviceManager().getAudioDeviceSetup().outputDeviceName;
}
double AudioRoutingManager::latencyMs() const {
  auto *d = engine.deviceManager().getCurrentAudioDevice();
  return d && d->getCurrentSampleRate() > 0
             ? 1000.0 *
                   (d->getInputLatencyInSamples() +
                    d->getOutputLatencyInSamples() +
                    d->getCurrentBufferSizeSamples()) /
                   d->getCurrentSampleRate()
             : 0;
}
} // namespace vox
