#pragma once
#include "Audio/AudioEngine.h"
#include "Settings/SettingsManager.h"
#include "Localization/LocalizationManager.h"
#include <juce_audio_utils/juce_audio_utils.h>
namespace vox {
class SettingsPage final : public juce::Component, private juce::Timer {
public:
  SettingsPage(AudioEngine &, SettingsManager &);
  ~SettingsPage() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool runLanguageSmokeTest();
  std::function<void()> onOpenDevices;
  std::function<void()> onPreferencesChanged;

private:
  void timerCallback() override;
  void markDirty();
  void applyDevice(int, double);
  void restoreDefaults();
  void loadPreferences();
  void persistPreferences();
  void updateTexts();
  void setupCombo(juce::ComboBox &, const juce::StringArray &, int);
  void setupToggle(juce::ToggleButton &, const juce::String &, bool);
  void row(juce::Rectangle<int> &, juce::Component &);
  void layoutCard(int, juce::Rectangle<int>);
  AudioEngine &engine;
  SettingsManager &settings;
  LocalizationManager &localization;
  int languageListener = 0;
  bool dirty = false;
  juce::Label title, subtitle, saveStatus, languageTitle, languageDescription;
  juce::TextButton restore, openDevices, exportButton, importButton, openLogs,
      clearLogs, clearCache, checkUpdates, backupButton;
  juce::ComboBox themeBox, language, fontSize, scale, quality, buffer,
      sampleRate, audioMode, channels, processPriority, meterRate, logLevel,
      updateChannel;
  juce::ToggleButton compact, reduceAnimations, showTips, autoContext,
      monitorInput, safetyLimiter, reconnect, syncDevices, startWindows,
      startMinimized, startTray, restorePreset, autoProcess, cpuOptimization,
      adaptiveQuality, economy, highContrast, focusVisible, largeClick,
      keyboardNav, saveLogs, telemetry, autoUpdates;
  juce::Array<juce::Rectangle<int>> cards;
  juce::Viewport viewport;
  juce::Component canvas;
};
} // namespace vox
