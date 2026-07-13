#pragma once
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/Navigation/PageRouter.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox {
class AdminPage final : public juce::Component, private juce::Timer {
public:
  AdminPage(AudioEngine &, SettingsManager &, PresetManager &);
  void paint(juce::Graphics &) override;
  void resized() override;
  std::function<void(PageId)> navigate;
  std::function<void(const juce::String &)> notify;

private:
  void timerCallback() override;
  void refresh();
  void createBackup();
  void exportLogs();
  void clearOldLogs();
  juce::String storageText() const;
  AudioEngine &engine;
  SettingsManager &settings;
  PresetManager &presets;
  juce::Label title, subtitle, presetsValue, favouritesValue, logsValue,
      storageValue, statusValue;
  juce::TextButton overview{juce::String::fromUTF8("Visão Geral")}, presetsTab{"Presets"},
      securityTab{juce::String::fromUTF8("Segurança")}, logsTab{"Logs do Sistema"},
      settingsTab{juce::String::fromUTF8("Configurações")};
  juce::TextButton managePresets{juce::String::fromUTF8("Gerenciar Presets  →")},
      managePermissions{juce::String::fromUTF8("Preferências e Permissões  →")},
      security{juce::String::fromUTF8("Configurações de Segurança  →")},
      openLogs{juce::String::fromUTF8("Abrir Logs do Sistema  →")}, backup{"Fazer Backup Agora"},
      exportButton{"Exportar Logs"}, clearButton{"Limpar Logs Antigos"};
  juce::TextEditor activities;
  juce::Rectangle<int> summaryArea, activityArea, quickArea, infoArea,
      actionsArea;
};
} // namespace vox
