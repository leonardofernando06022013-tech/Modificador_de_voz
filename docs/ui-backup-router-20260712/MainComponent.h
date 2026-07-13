#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "Diagnostics/DiagnosticsManager.h"
#include "AppLookAndFeel.h"
#include "ModernComponents.h"
namespace vox {
class MainComponent final:public juce::Component,private juce::Timer{
public:MainComponent(AudioEngine&,SettingsManager&);~MainComponent()override;void paint(juce::Graphics&)override;void resized()override;
private:
 class SettingsPage;
 void timerCallback()override;void showPage(int);void applyPreset(int);void filterCards();void addParameter(const juce::String&,double,double,double,std::atomic<float>&);
 AudioEngine&engine;SettingsManager&settings;PresetManager presets;AppLookAndFeel look;juce::TooltipWindow tooltips{this,500};
 SidebarComponent sidebar;juce::Label title,version,pageTitle,bannerTitle,bannerText,selectedName,selectedCategory,status,cpuLatency,deviceName;
 juce::TextEditor search,diagnosticText;juce::TextButton clearSearch{"Limpar"},deviceStatus{"Audio"},configure{"Configurar dispositivos"},copyDiagnostic{"Copiar relatorio"};
 juce::OwnedArray<juce::TextButton>tabs,chips;juce::Viewport viewport;juce::Component cardCanvas;juce::OwnedArray<VoiceCardComponent>cards,effectCards;
 juce::Component details;juce::OwnedArray<juce::Label>parameterLabels;juce::OwnedArray<juce::Slider>parameterSliders;juce::TextButton savePreset{"Salvar alteracoes"},duplicatePreset{"Duplicar"},resetPreset{"Restaurar"};
 juce::AudioDeviceSelectorComponent devices;AudioLevelMeter meter;juce::TextButton power{"LIGAR"},monitor{"Ouvir voz"},mute{"Silenciar"},bypass{"Bypass"};
 std::unique_ptr<SettingsPage> settingsPage;
 NotificationComponent notification;int currentPage=1,selectedCard=0;juce::String categoryFilter{"Todos"};
 juce::Rectangle<int> topBounds,contentBounds,rightBounds,bottomBounds,bannerBounds,chipBounds;
};}
