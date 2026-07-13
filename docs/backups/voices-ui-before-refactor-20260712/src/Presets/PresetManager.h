#pragma once
#include "Settings/SettingsManager.h"
namespace vox {
class PresetManager {
public:
    PresetManager(); juce::StringArray names() const; bool save(const juce::String&,const Parameters&)const;bool load(const juce::String&,Parameters&)const;bool remove(const juce::String&)const;
    void applyFactory(const juce::String&,Parameters&)const; juce::File directory()const{return dir;}
private:juce::File dir;
};
}

