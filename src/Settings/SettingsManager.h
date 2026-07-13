#pragma once
#include "DSP/VoiceProcessor.h"
#include <juce_data_structures/juce_data_structures.h>
namespace vox {
class SettingsManager {
public:
  SettingsManager();
  bool load(Parameters &);
  bool save(const Parameters &) const;
  juce::File file() const { return settingsFile; }
  juce::StringArray favourites() const;
  bool isFavourite(const juce::String &) const;
  bool setFavourite(const juce::String &, bool) const;
  juce::String preference(const juce::String &,
                          const juce::String &fallback = {}) const;
  bool setPreference(const juce::String &, const juce::String &) const;
  static juce::var parametersToJson(const Parameters &);
  static bool jsonToParameters(const juce::var &, Parameters &);

private:
  juce::File settingsFile;
};
} // namespace vox
