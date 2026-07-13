#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "Diagnostics/DiagnosticsManager.h"
#include "AppLookAndFeel.h"
#include "ModernComponents.h"
#include "Navigation/PageRouter.h"
#include "Pages/SettingsPage.h"
#include "Pages/ModulePage.h"
namespace vox {
class MainComponent final:public juce::Component,private juce::Timer{
public:MainComponent(AudioEngine&,SettingsManager&);~MainComponent()override;void paint(juce::Graphics&)override;void resized()override;bool runNavigationSmokeTest();
private:
 void timerCallback()override;void showPage(int);void applyPreset(int);void filterCards();void addParameter(const juce::String&,double,double,double,std::atomic<float>&);
 AudioEngine&engine;SettingsManager&settings;PresetManager presets;AppLookAndFeel look;juce::TooltipWindow tooltips{this,500};PageRouter router{PageId::Voices};
 SidebarComponent sidebar;juce::Label title,version,pageTitle,bannerTitle,bannerText,selectedName,selectedCategory,status,cpuLatency,deviceName,bottomVoiceName,bottomVoiceCategory;
 juce::TextEditor search,diagnosticText;juce::TextButton clearSearch{"Limpar"},deviceStatus{"Audio"},configure{"Configurar dispositivos"},copyDiagnostic{"Copiar relatorio"};
 juce::OwnedArray<juce::TextButton>tabs,chips;juce::Viewport viewport;juce::Component cardCanvas;juce::OwnedArray<VoiceCardComponent>cards,effectCards;
 juce::Component details;juce::OwnedArray<juce::Label>parameterLabels;juce::OwnedArray<juce::Slider>parameterSliders;juce::TextButton savePreset{"Salvar alteracoes"},duplicatePreset{"Duplicar"},resetPreset{"Restaurar"};
 juce::AudioDeviceSelectorComponent devices;AudioLevelMeter meter;juce::TextButton power{"LIGAR"},monitor{"Ouvir\nminha voz"},mute{"Silenciar\nmicrofone"},bypass{"Bypass"};
 juce::TextButton shortcutDevices{"Dispositivos"},shortcutEffects{"Efeitos"},shortcutEqualizer{"Equalizador"},shortcutSettings{"Config."};
 std::unique_ptr<SettingsPage> settingsPage;
 std::unique_ptr<ModulePage> homePage,soundboardPage,favoritesPage,equalizerPage,presetsModulePage;
 NotificationComponent notification;int currentPage=1,selectedCard=0;juce::String categoryFilter{"Todos"};
 juce::Rectangle<int> topBounds,contentBounds,rightBounds,bottomBounds,bannerBounds,chipBounds;
};}
