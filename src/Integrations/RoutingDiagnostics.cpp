#include "RoutingDiagnostics.h"
namespace vox {
RoutingDiagnosticResult
RoutingDiagnostics::run(AudioEngine &e,
                        const juce::Array<DetectedAudioDevice> &devices) {
  RoutingDiagnosticResult r;
  auto *d = e.deviceManager().getCurrentAudioDevice();
  if (!d) {
    r.headline = juce::String::fromUTF8("Dispositivo de áudio não aberto");
    r.details = e.lastError();
    return r;
  }
  auto setup = e.deviceManager().getAudioDeviceSetup();
  if (!VirtualDeviceDetector::isVirtual(setup.outputDeviceName)) {
    r.headline = juce::String::fromUTF8("Dispositivo virtual não encontrado");
    r.details =
        juce::String::fromUTF8("Selecione CABLE Input, VoiceMeeter Input ou outra saída virtual.");
    return r;
  }
  if (VirtualDeviceDetector::isVirtual(setup.inputDeviceName)) {
    r.headline = juce::String::fromUTF8("Possível loop de áudio detectado");
    r.details = juce::String::fromUTF8("A entrada do modificador parece ser virtual. Selecione o microfone físico como entrada.");
    return r;
  }
  if (d->getCurrentSampleRate() != 44100.0 &&
      d->getCurrentSampleRate() != 48000.0) {
    r.headline = juce::String::fromUTF8("Taxa de amostragem incompatível");
    r.details = "Prefira 48.000 Hz ou 44.100 Hz.";
    return r;
  }
  if (e.processor().inputPeak() < 0.0001f) {
    r.headline = "Microfone sem sinal";
    r.details = juce::String::fromUTF8("Fale no microfone e confirme a entrada física selecionada.");
    return r;
  }
  if (!e.isRunning()) {
    r.headline = "Processamento desligado";
    r.details = juce::String::fromUTF8("Ligue o modificador para enviar áudio processado.");
    return r;
  }
  if (e.processor().outputPeak() >= 1.0f) {
    r.headline = "Clipping detectado";
    r.details = juce::String::fromUTF8("Reduza o ganho de saída.");
    return r;
  }
  r.success = true;
  r.headline = "Tudo funcionando";
  r.details = juce::String::fromUTF8("Áudio processado chegando a ") + setup.outputDeviceName + " em " +
              juce::String(d->getCurrentSampleRate(), 0) + " Hz.";
  juce::ignoreUnused(devices);
  return r;
}
} // namespace vox
