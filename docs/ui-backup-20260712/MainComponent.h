#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "Diagnostics/DiagnosticsManager.h"
namespace vox {
class MainComponent final : public juce::Component, private juce::Timer {
public: MainComponent(AudioEngine&,SettingsManager&);~MainComponent()override;void resized()override;void paint(juce::Graphics&)override;
private:
    class ControlsPage; class DiagnosticsPage;
    void timerCallback()override;
    AudioEngine& engine;SettingsManager& settings;PresetManager presets;juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};
    juce::AudioDeviceSelectorComponent devices;std::unique_ptr<ControlsPage> voice,cleanup,dynamics,effects,presetPage;std::unique_ptr<DiagnosticsPage> diagnostics;
    juce::TextButton start{"Iniciar"},bypass{"Bypass geral"},mute{"Silenciar"};juce::Label status,meter;
};
}
