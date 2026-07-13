#pragma once
#include "Settings/SettingsManager.h"
namespace vox {
class PresetManager {
public:
    PresetManager(); juce::StringArray names() const; bool save(const juce::String&,const Parameters&)const;bool load(const juce::String&,Parameters&)const;bool remove(const juce::String&)const;
    static juce::String generatedName(int index);
    static juce::String generatedCategory(int index);
    static constexpr int generatedVoiceCount = 1000;
    void applyFactory(const juce::String&,Parameters&)const; juce::File directory()const{return dir;}
private:juce::File dir;
};
}
