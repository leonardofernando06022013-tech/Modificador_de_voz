#pragma once

#include "AppLookAndFeel.h"
#include "Audio/AudioEngine.h"
#include "Diagnostics/DiagnosticsManager.h"
#include "Localization/LocalizationManager.h"
#include "ModernComponents.h"
#include "Navigation/PageRouter.h"
#include "Pages/AdminPage.h"
#include "Pages/IntegrationsPage.h"
#include "Pages/ModulePage.h"
#include "Pages/SettingsPage.h"
#include "Pages/VoicesPage.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
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
  void showPage(PageId);
  void updateTexts();
  void applyPreferences();

  AudioEngine &engine;
  SettingsManager &settings;
  PresetManager presets;
  AppLookAndFeel look;
  juce::TooltipWindow tooltips{this, 500};
  PageRouter router{PageId::Voices};
  SidebarComponent sidebar;
  juce::Label title, version, pageTitle, status, cpuLatency, deviceName,
      bottomVoiceName, bottomVoiceCategory, searchShortcut;
  juce::TextEditor search, diagnosticText;
  juce::TextButton clearSearch{juce::String::fromUTF8("×")},
      deviceStatus{"Áudio"}, copyDiagnostic{"Copiar relatório"};
  juce::TextButton notifications{juce::String::fromUTF8("●")}, helpButton{"?"},
      topSettings{juce::String::fromUTF8("⚙")}, profile{juce::String::fromUTF8("●")};
  juce::AudioDeviceSelectorComponent devices;
  AudioLevelMeter meter;
  juce::TextButton power{"LIGAR"}, monitor{"Ouvir\nminha voz"},
      mute{"Silenciar\nmicrofone"}, bypass{"Bypass"};
  juce::TextButton shortcutDevices{"Dispositivos"}, shortcutEffects{"Efeitos"},
      shortcutEqualizer{"Equalizador"}, shortcutSettings{"Config."};
  std::unique_ptr<SettingsPage> settingsPage;
  std::unique_ptr<ModulePage> homePage, effectsPage, soundboardPage,
      favoritesPage, equalizerPage, presetsModulePage;
  std::unique_ptr<VoicesPage> voicesPage;
  std::unique_ptr<AdminPage> adminPage;
  std::unique_ptr<IntegrationsPage> integrationsPage;
  NotificationComponent notification;
  PageId currentPage = PageId::Voices;
  int languageListener = 0;
  bool compactUi = false;
  juce::Rectangle<int> topBounds, contentBounds, bottomBounds;
};
} // namespace vox
