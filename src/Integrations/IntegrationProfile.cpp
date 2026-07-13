#include "IntegrationProfile.h"
namespace vox {
juce::var IntegrationProfile::toJson() const {
  auto *o = new juce::DynamicObject();
  o->setProperty("name", name);
  o->setProperty("target", target);
  o->setProperty("audioType", audioType);
  o->setProperty("inputDevice", inputDevice);
  o->setProperty("virtualOutput", virtualOutput);
  o->setProperty("preset", preset);
  o->setProperty("sampleRate", sampleRate);
  o->setProperty("bufferSize", bufferSize);
  o->setProperty("monitoring", monitoring);
  o->setProperty("gainDb", gainDb);
  o->setProperty("gateThreshold", gateThreshold);
  o->setProperty("limiter", limiter);
  o->setProperty("lastUsed", lastUsed.toISO8601(true));
  return juce::var(o);
}
IntegrationProfile IntegrationProfile::fromJson(const juce::var &v) {
  IntegrationProfile p;
  if (auto *o = v.getDynamicObject()) {
    p.name = o->getProperty("name").toString();
    p.target = o->getProperty("target").toString();
    p.audioType = o->getProperty("audioType").toString();
    p.inputDevice = o->getProperty("inputDevice").toString();
    p.virtualOutput = o->getProperty("virtualOutput").toString();
    p.preset = o->getProperty("preset").toString();
    p.sampleRate = (double)o->getProperty("sampleRate");
    p.bufferSize = (int)o->getProperty("bufferSize");
    p.monitoring = (bool)o->getProperty("monitoring");
    p.gainDb = (float)o->getProperty("gainDb");
    p.gateThreshold = (float)o->getProperty("gateThreshold");
    p.limiter = (bool)o->getProperty("limiter");
    p.lastUsed = juce::Time::fromISO8601(o->getProperty("lastUsed").toString());
  }
  return p;
}
} // namespace vox
