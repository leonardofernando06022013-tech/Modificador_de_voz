#pragma once
#include "Audio/AudioRoutingManager.h"
#include "Integrations/IntegrationProfileManager.h"
#include "Integrations/ApplicationDetector.h"
#include "Integrations/RoutingDiagnostics.h"
#include "Integrations/VirtualDeviceDetector.h"
#include "UI/ModernComponents.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox {
class IntegrationsPage final : public juce::Component, private juce::Timer {
public:
  IntegrationsPage(AudioEngine &);
  void paint(juce::Graphics &) override;
  void resized() override;
  std::function<void(const juce::String &)> notify;
  bool runSmokeTest();
  juce::String activeTarget() const { return target; }
  juce::String routingStatusText() const {
    return target + " · " + routing.currentInput() + " → " +
           routing.currentOutput();
  }

private:
  void timerCallback() override;
  void refreshDevices();
  void selectTarget(const juce::String &);
  void applyRouting();
  void testRouting();
  void saveProfile();
  void refreshProfiles();
  AudioEngine &engine;
  AudioRoutingManager routing;
  IntegrationProfileManager profileManager;
  juce::Array<DetectedAudioDevice> detected;
  juce::Label title, subtitle, applicationStatus, virtualWarning, diagnosticTitle,
      diagnosticDetails, inputMeterLabel, outputMeterLabel;
  juce::OwnedArray<juce::TextButton> targets;
  juce::ComboBox inputBox, outputBox, profileBox, sampleRateBox, bufferBox;
  juce::ToggleButton monitoring{"Monitoramento local (use fones)"};
  juce::TextButton reload{"Recarregar dispositivos"},
      apply{"Aplicar roteamento"}, test{juce::String::fromUTF8("Testar integração")},
      copyDevice{"Copiar nome do dispositivo"},
      windowsSound{"Abrir Som do Windows"}, guide{juce::String::fromUTF8("Ver guia de configuração")},
      saveProfileButton{"Salvar perfil"}, deleteProfile{"Excluir perfil"},
      duplicateProfile{"Duplicar"}, exportProfiles{"Exportar"},
      importProfiles{"Importar"};
  juce::TextEditor instructions;
  AudioLevelMeter inputMeter, outputMeter;
  juce::String target{"Discord"};
  juce::Rectangle<int> flowArea, setupArea, guideArea, statusArea;
};
} // namespace vox
