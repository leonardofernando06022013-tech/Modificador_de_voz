#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "UI/Navigation/PageRouter.h"
namespace vox {
class ModulePage final:public juce::Component,private juce::Timer{
public:enum class Kind{Home,Soundboard,Favorites,Equalizer,Presets};ModulePage(Kind,AudioEngine&,PresetManager&);~ModulePage()override;void paint(juce::Graphics&)override;void resized()override;std::function<void(PageId)>navigate;
private:void timerCallback()override;void setup();Kind kind;AudioEngine&engine;PresetManager&presets;juce::Label title,description,status,emptyState;juce::TextButton primary,secondary,tertiary;juce::ComboBox presetList;juce::Slider hp,lp;std::unique_ptr<juce::FileChooser>chooser;juce::File selectedSound;juce::Array<juce::Rectangle<int>>panels;
};}
