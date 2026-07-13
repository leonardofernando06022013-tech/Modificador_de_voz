#include "SettingsManager.h"
#include "App/AppPaths.h"
namespace vox {
SettingsManager::SettingsManager() {
  settingsFile = AppPaths::data().getChildFile("settings.json");
}
juce::var SettingsManager::parametersToJson(const Parameters &p) {
  auto *o = new juce::DynamicObject();
#define P(n) o->setProperty(#n, p.n.load())
  P(inputGainDb);
  P(outputGainDb);
  P(mix);
  P(pitchSemitones);
  P(fineCents);
  P(formant);
  P(gateThreshold);
  P(gateAttack);
  P(gateRelease);
  P(compressorThreshold);
  P(compressorRatio);
  P(compressorAttack);
  P(compressorRelease);
  P(hpFreq);
  P(lpFreq);
  P(noiseReduction);
  P(distortion);
  P(chorus);
  P(flanger);
  P(delay);
  P(reverb);
  P(ringMod);
  P(bitCrush);
  P(bypass);
  P(muted);
  P(gateEnabled);
  P(cleanupEnabled);
  P(compressorEnabled);
  P(limiterEnabled);
  P(phaseInvert);
#undef P
  return juce::var(o);
}
bool SettingsManager::jsonToParameters(const juce::var &v, Parameters &p) {
  auto *o = v.getDynamicObject();
  if (!o)
    return false;
#define F(n, lo, hi)                                                           \
  if (o->hasProperty(#n))                                                      \
  p.n.store(juce::jlimit((float)lo, (float)hi, (float)o->getProperty(#n)))
  F(inputGainDb, -24, 24);
  F(outputGainDb, -24, 24);
  F(mix, 0, 1);
  F(pitchSemitones, -12, 12);
  F(fineCents, -100, 100);
  F(formant, -1, 1);
  F(gateThreshold, -80, 0);
  F(gateAttack, 0.1, 200);
  F(gateRelease, 1, 2000);
  F(compressorThreshold, -60, 0);
  F(compressorRatio, 1, 20);
  F(compressorAttack, 0.1, 200);
  F(compressorRelease, 1, 2000);
  F(hpFreq, 20, 2000);
  F(lpFreq, 2000, 20000);
  F(noiseReduction, 0, 1);
  F(distortion, 0, 1);
  F(chorus, 0, 1);
  F(flanger, 0, 1);
  F(delay, 0, 1);
  F(reverb, 0, 1);
  F(ringMod, 0, 1);
  F(bitCrush, 0, 1);
#undef F
#define B(n)                                                                   \
  if (o->hasProperty(#n))                                                      \
  p.n.store((bool)o->getProperty(#n))
  B(bypass);
  B(muted);
  B(gateEnabled);
  B(cleanupEnabled);
  B(compressorEnabled);
  B(limiterEnabled);
  B(phaseInvert);
#undef B
  return true;
}
bool SettingsManager::load(Parameters &p) {
  if (!settingsFile.existsAsFile())
    return true;
  auto parsed = juce::JSON::parse(settingsFile);
  if (jsonToParameters(parsed, p))
    return true;
  settingsFile.moveFileTo(settingsFile.getSiblingFile(
      "settings-corrompido-" +
      juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + ".json"));
  return false;
}
bool SettingsManager::save(const Parameters &p) const {
  juce::TemporaryFile temporary(settingsFile);
  if (!temporary.getFile().replaceWithText(
          juce::JSON::toString(parametersToJson(p), true)))
    return false;
  return temporary.overwriteTargetFileWithTemporary();
}
juce::StringArray SettingsManager::favourites() const {
  juce::StringArray result;
  auto file = settingsFile.getSiblingFile("favorites.json");
  if (!file.existsAsFile())
    return result;
  auto value = juce::JSON::parse(file);
  if (auto *array = value.getArray())
    for (const auto &item : *array)
      if (item.isString())
        result.addIfNotAlreadyThere(item.toString());
  return result;
}
bool SettingsManager::isFavourite(const juce::String &name) const {
  return favourites().contains(name);
}
bool SettingsManager::setFavourite(const juce::String &name,
                                   bool enabled) const {
  auto items = favourites();
  if (enabled)
    items.addIfNotAlreadyThere(name);
  else
    items.removeString(name);
  juce::Array<juce::var> values;
  for (const auto &item : items)
    values.add(item);
  juce::TemporaryFile temporary(settingsFile.getSiblingFile("favorites.json"));
  if (!temporary.getFile().replaceWithText(
          juce::JSON::toString(juce::var(values), true)))
    return false;
  return temporary.overwriteTargetFileWithTemporary();
}
juce::String SettingsManager::preference(const juce::String &key,
                                         const juce::String &fallback) const {
  auto file = settingsFile.getSiblingFile("preferences.json");
  if (!file.existsAsFile())
    return fallback;
  auto value = juce::JSON::parse(file);
  if (auto *object = value.getDynamicObject())
    if (object->hasProperty(key))
      return object->getProperty(key).toString();
  return fallback;
}
bool SettingsManager::setPreference(const juce::String &key,
                                    const juce::String &value) const {
  auto file = settingsFile.getSiblingFile("preferences.json");
  auto parsed = file.existsAsFile() ? juce::JSON::parse(file) : juce::var{};
  auto *object = parsed.getDynamicObject();
  if (!object) {
    object = new juce::DynamicObject();
    parsed = juce::var(object);
  }
  object->setProperty(key, value);
  juce::TemporaryFile temporary(file);
  if (!temporary.getFile().replaceWithText(juce::JSON::toString(parsed, true)))
    return false;
  return temporary.overwriteTargetFileWithTemporary();
}
} // namespace vox
