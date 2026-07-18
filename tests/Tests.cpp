#include <juce_core/juce_core.h>

#include "Admin/AdminAccessController.h"
#include "DSP/VoiceProcessor.h"
#include "Integrations/ApplicationDetector.h"
#include "Integrations/IntegrationProfile.h"
#include "Integrations/VirtualDeviceDetector.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/Navigation/PageRouter.h"

#include <cmath>
#include <iostream>

namespace {
constexpr double testSampleRate = 48000.0;

void configureTransparent(vox::Parameters &parameters) {
  parameters.cleanupEnabled = false;
  parameters.gateEnabled = false;
  parameters.compressorEnabled = false;
  parameters.limiterEnabled = false;
  parameters.deEsserAmount = 0.0f;
  parameters.agcAmount = 0.0f;
  parameters.multibandAmount = 0.0f;
  parameters.pitchSemitones = 0.0f;
  parameters.formant = 0.0f;
  parameters.mix = 1.0f;
}

float rms(const juce::AudioBuffer<float> &buffer, int channel) {
  double power = 0.0;
  for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
    const double value = buffer.getSample(channel, sample);
    power += value * value;
  }
  return static_cast<float>(
      std::sqrt(power / juce::jmax(1, buffer.getNumSamples())));
}
} // namespace

class DSPTests final : public juce::UnitTest {
public:
  DSPTests() : UnitTest("DSP") {}

  void runTest() override {
    beginTest("Buffer vazio e bypass");
    vox::Parameters parameters;
    vox::VoiceProcessor processor(parameters);
    processor.prepare(testSampleRate, 256, 2);
    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    processor.process(buffer);
    expectEquals(buffer.getMagnitude(0, 256), 0.0f);
    buffer.setSample(0, 0, 0.25f);
    parameters.bypass = true;
    processor.process(buffer);
    expectWithinAbsoluteError(buffer.getSample(0, 0), 0.25f, 0.0001f);

    beginTest("Processamento respeita a regiao ativa pre-alocada");
    parameters.bypass = false;
    configureTransparent(parameters);
    processor.reset();
    buffer.clear();
    for (int channel = 0; channel < 2; ++channel) {
      for (int sample = 0; sample < 64; ++sample)
        buffer.setSample(channel, sample,
                         0.1f * std::sin(0.03f * static_cast<float>(sample)));
      for (int sample = 64; sample < buffer.getNumSamples(); ++sample)
        buffer.setSample(channel, sample, 0.321f);
    }
    processor.process(buffer, 64);
    expectWithinAbsoluteError(buffer.getSample(0, 180), 0.321f, 0.000001f);
    expectWithinAbsoluteError(buffer.getSample(1, 255), 0.321f, 0.000001f);

    beginTest("Limitador evita clipping");
    parameters.inputGainDb = 24.0f;
    parameters.limiterEnabled = true;
    for (int channel = 0; channel < 2; ++channel)
      for (int sample = 0; sample < 256; ++sample)
        buffer.setSample(channel, sample, 1.0f);
    processor.process(buffer);
    expect(buffer.getMagnitude(0, 256) <= 1.0f);

    beginTest("De-esser atenua sibilancia sem remover medios");
    const auto measureTone = [](float frequency) {
      vox::Parameters p;
      configureTransparent(p);
      p.deEsserAmount = 1.0f;
      vox::VoiceProcessor voice(p);
      voice.prepare(testSampleRate, 256, 1);
      juce::AudioBuffer<float> tone(1, 256);
      double phase = 0.0;
      float measured = 0.0f;
      for (int block = 0; block < 120; ++block) {
        for (int sample = 0; sample < tone.getNumSamples(); ++sample) {
          tone.setSample(0, sample,
                         0.22f * static_cast<float>(std::sin(phase)));
          phase += juce::MathConstants<double>::twoPi * frequency /
                   testSampleRate;
          if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
        }
        voice.process(tone);
        if (block == 119)
          measured = rms(tone, 0);
      }
      return measured;
    };
    const float middleRms = measureTone(1000.0f);
    const float sibilantRms = measureTone(7000.0f);
    expect(middleRms > sibilantRms * 1.2f,
           "A banda sibilante deve receber reducao dinamica maior");

    beginTest("Pitch preserva canais estereo independentes");
    vox::Parameters pitchParameters;
    configureTransparent(pitchParameters);
    pitchParameters.pitchSemitones = 3.0f;
    pitchParameters.formantPreserve = true;
    vox::VoiceProcessor pitchProcessor(pitchParameters);
    pitchProcessor.prepare(testSampleRate, 256, 2);
    juce::AudioBuffer<float> stereo(2, 256);
    double leftPhase = 0.0;
    double rightPhase = 0.0;
    for (int block = 0; block < 80; ++block) {
      for (int sample = 0; sample < stereo.getNumSamples(); ++sample) {
        stereo.setSample(0, sample,
                         0.16f * static_cast<float>(std::sin(leftPhase)));
        stereo.setSample(1, sample,
                         0.16f * static_cast<float>(std::sin(rightPhase)));
        leftPhase += juce::MathConstants<double>::twoPi * 190.0 /
                     testSampleRate;
        rightPhase += juce::MathConstants<double>::twoPi * 310.0 /
                      testSampleRate;
      }
      pitchProcessor.process(stereo);
    }
    double channelDifference = 0.0;
    for (int sample = 0; sample < stereo.getNumSamples(); ++sample) {
      const double difference =
          stereo.getSample(0, sample) - stereo.getSample(1, sample);
      channelDifference += difference * difference;
      expect(std::isfinite(stereo.getSample(0, sample)));
      expect(std::isfinite(stereo.getSample(1, sample)));
    }
    channelDifference = std::sqrt(channelDifference / stereo.getNumSamples());
    expect(channelDifference > 0.01,
           "O pitch nao pode converter a entrada estereo em mono");

    beginTest("DSP permanece finito em buffers pequenos");
    vox::Parameters smallBufferParameters;
    smallBufferParameters.pitchSemitones = 2.0f;
    smallBufferParameters.deEsserAmount = 0.5f;
    smallBufferParameters.multibandAmount = 0.4f;
    smallBufferParameters.agcAmount = 0.2f;
    vox::VoiceProcessor smallBufferProcessor(smallBufferParameters);
    smallBufferProcessor.prepare(testSampleRate, 256, 2);
    juce::AudioBuffer<float> smallBuffer(2, 256);
    for (const int activeSamples : {1, 16, 64, 256}) {
      for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < activeSamples; ++sample)
          smallBuffer.setSample(
              channel, sample,
              0.1f * std::sin(0.02f * static_cast<float>(sample + channel)));
      smallBufferProcessor.process(smallBuffer, activeSamples);
      for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < activeSamples; ++sample)
          expect(std::isfinite(smallBuffer.getSample(channel, sample)));
    }

    beginTest("Parametros invalidos sao limitados");
    auto *object = new juce::DynamicObject();
    object->setProperty("pitchSemitones", 99);
    object->setProperty("mix", -4);
    object->setProperty("bassDb", 50);
    object->setProperty("midDb", -50);
    object->setProperty("deEsserAmount", 8);
    object->setProperty("gateHold", 900);
    expect(vox::SettingsManager::jsonToParameters(juce::var(object),
                                                  parameters));
    expectEquals(parameters.pitchSemitones.load(), 12.0f);
    expectEquals(parameters.mix.load(), 0.0f);
    expectEquals(parameters.bassDb.load(), 12.0f);
    expectEquals(parameters.midDb.load(), -12.0f);
    expectEquals(parameters.deEsserAmount.load(), 1.0f);
    expectEquals(parameters.gateHold.load(), 500.0f);

    beginTest("JSON invalido");
    expect(!vox::SettingsManager::jsonToParameters(
        juce::JSON::parse("{bad"), parameters));

    beginTest("Preset salva e carrega novos parametros");
    vox::PresetManager presetManager;
    parameters.pitchSemitones = 6.0f;
    parameters.deEsserAmount = 0.62f;
    parameters.formantPreserve = true;
    expect(presetManager.save("Teste automatizado", parameters));
    parameters.pitchSemitones = 0.0f;
    parameters.deEsserAmount = 0.0f;
    expect(presetManager.load("Teste automatizado", parameters));
    expectEquals(parameters.pitchSemitones.load(), 6.0f);
    expectEquals(parameters.deEsserAmount.load(), 0.62f);
    expect(parameters.formantPreserve.load());
    expect(presetManager.remove("Teste automatizado"));

    beginTest("Presets profissionais sao conservadores e reproduziveis");
    vox::Parameters natural;
    presetManager.applyFactory(juce::String::fromUTF8("Conversação Natural"),
                               natural);
    expectEquals(natural.pitchSemitones.load(), 0.0f);
    expectEquals(natural.reverb.load(), 0.0f);
    expect(natural.deEsserAmount.load() > 0.0f);
    expect(natural.formantPreserve.load());
    vox::Parameters podcast;
    presetManager.applyFactory("Podcast", podcast);
    expect(podcast.multibandAmount.load() > 0.0f);
    expect(podcast.agcAmount.load() > 0.0f);
    vox::Parameters economy;
    presetManager.applyFactory("Baixo consumo", economy);
    expect(economy.economyMode.load());
    expectEquals(economy.multibandAmount.load(), 0.0f);

    beginTest("Mil vozes geradas sao distintas e limitadas");
    expectEquals(vox::PresetManager::generatedVoiceCount, 1000);
    vox::Parameters generatedA, generatedB;
    presetManager.applyFactory(vox::PresetManager::generatedName(0),
                               generatedA);
    presetManager.applyFactory(vox::PresetManager::generatedName(999),
                               generatedB);
    expect(generatedA.pitchSemitones.load() !=
               generatedB.pitchSemitones.load() ||
           generatedA.formant.load() != generatedB.formant.load());
    expectWithinAbsoluteError(
        juce::jlimit(-12.0f, 12.0f, generatedB.pitchSemitones.load()),
        generatedB.pitchSemitones.load(), 0.0001f);

    beginTest("Categorias acompanham a fonte de presets");
    expectEquals(presetManager.categoryFor("Masculina grave"),
                 juce::String("Graves"));
    expectEquals(presetManager.categoryFor(vox::PresetManager::generatedName(7)),
                 vox::PresetManager::generatedCategory(7));
    expect(presetManager.names().contains(
        juce::String::fromUTF8("Conversação Natural")));

    beginTest("Roteador preserva pagina anterior");
    vox::PageRouter router(vox::PageId::Home);
    int changes = 0;
    router.setListener([&](vox::PageId oldPage, vox::PageId newPage) {
      expect(oldPage != newPage);
      ++changes;
    });
    router.navigateTo(vox::PageId::Settings);
    expect(router.currentPage() == vox::PageId::Settings);
    expect(router.previousPage() == vox::PageId::Home);
    router.navigateTo(vox::PageId::Settings);
    expectEquals(changes, 1);
    router.back();
    expect(router.currentPage() == vox::PageId::Home);
    expectEquals(changes, 2);

    beginTest("Perfil de integracao preserva roteamento");
    vox::IntegrationProfile profile;
    profile.name = "Discord";
    profile.inputDevice = "Microfone USB";
    profile.virtualOutput = "CABLE Input";
    profile.sampleRate = 48000;
    profile.bufferSize = 256;
    const auto restored = vox::IntegrationProfile::fromJson(profile.toJson());
    expectEquals(restored.name, profile.name);
    expectEquals(restored.inputDevice, profile.inputDevice);
    expectEquals(restored.virtualOutput, profile.virtualOutput);
    expectEquals(restored.bufferSize, 256);

    beginTest("Detector reconhece cabos virtuais sem falso positivo comum");
    expect(vox::VirtualDeviceDetector::isVirtual(
        "CABLE Input (VB-Audio Virtual Cable)"));
    expect(vox::VirtualDeviceDetector::isVirtual("VoiceMeeter AUX Input"));
    expect(vox::VirtualDeviceDetector::isVirtual(
        "Dubbing Speaker (Dubbing Virtual Device)"));
    expect(!vox::VirtualDeviceDetector::isVirtual("Microfone USB Realtek"));

    beginTest("Detector de aplicativos mapeia alvos conhecidos");
    expect(vox::ApplicationDetector::executablesFor("Discord")
               .contains("Discord.exe"));
    expect(vox::ApplicationDetector::executablesFor("OBS")
               .contains("obs64.exe"));
    expect(vox::ApplicationDetector::executablesFor("Manual").isEmpty());

    beginTest("Controle administrativo bloqueia usuario comum");
    vox::AdminAccessController access;
    vox::UserAccount common;
    common.id = "common";
    common.status = vox::UserStatus::Active;
    common.role = vox::Role::User;
    expect(!access.begin(common));
    expect(!access.can(vox::Permission::ViewLogs));
    vox::UserAccount administrator;
    administrator.id = "admin";
    administrator.status = vox::UserStatus::Active;
    administrator.role = vox::Role::Administrator;
    expect(access.begin(administrator));
    expect(access.can(vox::Permission::ViewLogs));

    beginTest("Interface administrativa preserva UTF-8");
    const auto adminText = juce::String::fromUTF8(
        "Administração · usuários · permissões · relatório");
    expect(adminText.contains(juce::String::fromUTF8("Administração")));
    expect(!adminText.contains("Ãƒ"));
  }
};

int main() {
  juce::ScopedJuceInitialiser_GUI initialiseJuce;
  DSPTests tests;
  juce::UnitTestRunner runner;
  runner.runAllTests();
  for (int index = 0; index < runner.getNumResults(); ++index) {
    const auto *result = runner.getResult(index);
    if (result->failures > 0) {
      std::cerr << result->subcategoryName << ": " << result->failures
                << " failure(s)\n";
      for (const auto &message : result->messages)
        std::cerr << "  " << message << '\n';
      return 1;
    }
  }
  return 0;
}
