#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include "DSP/VoiceProcessor.h"
namespace vox {
class SettingsManager {
public:
    SettingsManager(); bool load(Parameters&); bool save(const Parameters&) const; juce::File file() const { return settingsFile; }
    static juce::var parametersToJson(const Parameters&); static bool jsonToParameters(const juce::var&,Parameters&);
private: juce::File settingsFile;
};
}

