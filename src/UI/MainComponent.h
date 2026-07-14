#pragma once
#include "AppLookAndFeel.h"
#include "Audio/AudioEngine.h"
#include "Diagnostics/DiagnosticsManager.h"
#include "ModernComponents.h"
#include "Navigation/PageRouter.h"
#include "Pages/AdminPage.h"
#include "Pages/IntegrationsPage.h"
#include "Pages/ModulePage.h"
#include "Pages/SettingsPage.h"
#include "Pages/VoicesPage.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "Localization/LocalizationManager.h"
#include <juce_audio_utils/juce_audio_utils.h>
namespace vox {
class MainComponent final : public juce::Component, private juce::Timer {
public:
  MainComponent(AudioEngine &, SettingsManager &);
  ~MainComponent() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool runNavigationSmokeTest();

private:
  void timerCallback() override;
  void showPage(int);
  void applyPreset(int);
  void filterCards();
  void updateTexts();
  void addParameter(const juce::String &, double, double, double,
                    std::atomic<float> &);
  AudioEngine &engine;
  SettingsManager &settings;
  PresetManager presets;
  AppLookAndFeel look;
  juce::TooltipWindow tooltips{this, 500};
  PageRouter router{PageId::Voices};
  SidebarComponent sidebar;
  juce::Label title, version, pageTitle, bannerTitle, bannerText, selectedName,
      selectedCategory, status, cpuLatency, deviceName, bottomVoiceName,
      bottomVoiceCategory, searchShortcut;
  juce::TextEditor search, diagnosticText;
  juce::TextButton clearSearch{juce::String::fromUTF8("\xc3\x97")},
      deviceStatus{"Audio"}, configure{juce::String::fromUTF8("Configurar dispositivos")},
      copyDiagnostic{juce::String::fromUTF8("Copiar relat\xc3\xb3rio")};
  juce::TextButton notifications{juce::String::fromUTF8("●")}, helpButton{"?"},
      topSettings{juce::String::fromUTF8("⚙")},
      profile{juce::String::fromUTF8("●")};
  juce::OwnedArray<juce::TextButton> tabs, chips;
  juce::Viewport viewport;
  juce::Component cardCanvas;
  juce::OwnedArray<VoiceCardComponent> cards, effectCards;
  juce::Component details;
  juce::OwnedArray<juce::Label> parameterLabels;
  juce::OwnedArray<juce::Slider> parameterSliders;
  juce::TextButton savePreset{juce::String::fromUTF8("Salvar altera\xc3\xa7\xc3\xb5\x65s")}, duplicatePreset{"Duplicar"},
      resetPreset{"Restaurar"};
  juce::AudioDeviceSelectorComponent devices;
  AudioLevelMeter meter;
  juce::TextButton power{"LIGAR"}, monitor{"Ouvir\nminha voz"},
      mute{"Silenciar\nmicrofone"}, bypass{"Bypass"};
  juce::TextButton shortcutDevices{"Dispositivos"}, shortcutEffects{"Efeitos"},
      shortcutEqualizer{"Equalizador"}, shortcutSettings{"Config."};
  std::unique_ptr<SettingsPage> settingsPage;
  std::unique_ptr<ModulePage> homePage, soundboardPage, favoritesPage,
      equalizerPage, presetsModulePage;
  std::unique_ptr<VoicesPage> voicesPage;
  std::unique_ptr<AdminPage> adminPage;
  std::unique_ptr<IntegrationsPage> integrationsPage;
  NotificationComponent notification;
  int currentPage = 1, selectedCard = 0;
  int languageListener = 0;
  juce::String categoryFilter{"Todos"};
  juce::Rectangle<int> topBounds, contentBounds, rightBounds, bottomBounds,
      bannerBounds, chipBounds;
};
} // namespace vox
