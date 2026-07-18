#include "PresetManager.h"
#include "App/AppPaths.h"
#include <cmath>
#include <cstdint>

namespace vox {
PresetManager::PresetManager() { dir = AppPaths::presets(); }

juce::String PresetManager::generatedName(int index) {
  const juce::StringArray families{"Aurora", "Neon", "Titan", "Crystal",
      "Phantom", "Nova", "Cyber", "Echo", "Pulse", "Shadow", "Solar",
      "Lunar", "Vortex", "Pixel", "Storm", "Velvet", "Quantum", "Retro",
      "Cosmic", "Prism"};
  return "Voz " + families[index % families.size()] + " " +
         juce::String(index + 1).paddedLeft('0', 4);
}

juce::String PresetManager::generatedCategory(int index) {
  const juce::StringArray categories{
      "Naturais", "Graves", "Agudas", juce::String::fromUTF8("Robôs"),
      "Criativas", "Terror", juce::String::fromUTF8("Rádio"), "Personagens",
      "Espaciais", "Jogos"};
  return categories[index % categories.size()];
}

juce::StringArray PresetManager::names() const {
  juce::StringArray result{
      juce::String::fromUTF8("Conversação Natural"), "Podcast",
      juce::String::fromUTF8("Narração"), "Voz Quente", "Voz Robusta",
      "Baixo consumo", "Voz normal limpa", "Masculina grave",
      "Feminina suave", juce::String::fromUTF8("Robô"),
      juce::String::fromUTF8("Rádio policial"), "Telefone",
      juce::String::fromUTF8("Demônio"), juce::String::fromUTF8("Alienígena"),
      "Monstro", "Personagem infantil", "Narrador", "Megafone"};
  for (int i = 0; i < generatedVoiceCount; ++i)
    result.add(generatedName(i));
  for (const auto &file : dir.findChildFiles(juce::File::findFiles, false, "*.json"))
    result.addIfNotAlreadyThere(file.getFileNameWithoutExtension());
  return result;
}

juce::String PresetManager::categoryFor(const juce::String &name) const {
  for (int i = 0; i < generatedVoiceCount; ++i)
    if (name == generatedName(i))
      return generatedCategory(i);
  if (dir.getChildFile(juce::File::createLegalFileName(name) + ".json")
          .existsAsFile())
    return "Personalizadas";
  if (name == "Masculina grave") return "Graves";
  if (name == "Personagem infantil") return "Agudas";
  if (name == juce::String::fromUTF8("Robô"))
    return juce::String::fromUTF8("Robôs");
  if (name == juce::String::fromUTF8("Rádio policial"))
    return juce::String::fromUTF8("Rádio");
  if (name == "Telefone") return "Dispositivos";
  if (name == juce::String::fromUTF8("Demônio") || name == "Monstro")
    return "Terror";
  if (name == juce::String::fromUTF8("Conversação Natural") ||
      name == "Podcast" || name == juce::String::fromUTF8("Narração") ||
      name == "Voz Quente" || name == "Voz Robusta" ||
      name == "Baixo consumo" || name == "Voz normal limpa" ||
      name == "Feminina suave" || name == "Narrador")
    return "Naturais";
  return "Criativas";
}

bool PresetManager::save(const juce::String &name, const Parameters &p) const {
  const auto safe = juce::File::createLegalFileName(name.trim());
  return safe.isNotEmpty() && dir.getChildFile(safe + ".json").replaceWithText(
      juce::JSON::toString(SettingsManager::parametersToJson(p), true));
}

bool PresetManager::load(const juce::String &name, Parameters &p) const {
  const auto file = dir.getChildFile(juce::File::createLegalFileName(name) + ".json");
  if (file.existsAsFile())
    return SettingsManager::jsonToParameters(juce::JSON::parse(file), p);
  applyFactory(name, p);
  return names().contains(name);
}

bool PresetManager::remove(const juce::String &name) const {
  const auto file = dir.getChildFile(juce::File::createLegalFileName(name) + ".json");
  return file.existsAsFile() && file.deleteFile();
}

void PresetManager::applyFactory(const juce::String &name, Parameters &p) const {
  p.inputGainDb = 0; p.outputGainDb = 0; p.pitchSemitones = 0;
  p.fineCents = 0; p.formant = 0; p.mix = 1; p.distortion = 0;
  p.chorus = 0; p.flanger = 0; p.delay = 0; p.reverb = 0; p.ringMod = 0;
  p.bitCrush = 0; p.hpFreq = 70; p.lpFreq = 18000;
  p.bassDb = 0; p.midDb = 0; p.trebleDb = 0;
  p.noiseReduction = .25f; p.deEsserAmount = .35f;
  p.gateEnabled = true; p.gateThreshold = -58; p.gateAttack = 5;
  p.gateRelease = 160; p.gateHold = 70;
  p.cleanupEnabled = true; p.compressorEnabled = true;
  p.compressorThreshold = -18; p.compressorRatio = 3;
  p.compressorAttack = 8; p.compressorRelease = 120;
  p.agcAmount = 0; p.multibandAmount = 0;
  p.limiterEnabled = true; p.formantPreserve = true; p.economyMode = false;

  if (name == juce::String::fromUTF8("Conversação Natural")) {
    p.noiseReduction = .25f; p.deEsserAmount = .30f;
    p.gateThreshold = -62; p.gateRelease = 190; p.gateHold = 90;
    p.compressorThreshold = -20; p.compressorRatio = 2.2f;
    p.compressorAttack = 12; p.compressorRelease = 170;
    p.trebleDb = .5f;
    return;
  }
  if (name == "Podcast") {
    p.noiseReduction = .22f; p.deEsserAmount = .42f;
    p.gateThreshold = -60; p.gateRelease = 180; p.gateHold = 85;
    p.bassDb = 2.0f; p.midDb = .3f; p.trebleDb = 1.2f;
    p.compressorThreshold = -20; p.compressorRatio = 3.0f;
    p.compressorAttack = 8; p.compressorRelease = 140;
    p.agcAmount = .18f; p.multibandAmount = .30f;
    return;
  }
  if (name == juce::String::fromUTF8("Narração")) {
    p.noiseReduction = .20f; p.deEsserAmount = .38f;
    p.gateThreshold = -61; p.bassDb = 2.5f; p.midDb = .7f;
    p.trebleDb = 1.0f; p.compressorThreshold = -21;
    p.compressorRatio = 3.2f; p.compressorAttack = 10;
    p.compressorRelease = 165; p.agcAmount = .15f;
    p.multibandAmount = .25f; p.reverb = .04f;
    return;
  }
  if (name == "Voz Quente") {
    p.noiseReduction = .18f; p.deEsserAmount = .26f;
    p.gateThreshold = -62; p.bassDb = 3.0f; p.midDb = .5f;
    p.trebleDb = -.5f; p.compressorThreshold = -19;
    p.compressorRatio = 2.4f; p.distortion = .035f; p.reverb = .035f;
    return;
  }
  if (name == "Voz Robusta") {
    p.noiseReduction = .28f; p.deEsserAmount = .40f;
    p.gateThreshold = -57; p.gateHold = 100; p.bassDb = 2.0f;
    p.midDb = 1.2f; p.trebleDb = .5f; p.compressorThreshold = -22;
    p.compressorRatio = 3.8f; p.compressorAttack = 5;
    p.compressorRelease = 180; p.agcAmount = .20f;
    p.multibandAmount = .35f;
    return;
  }
  if (name == "Baixo consumo") {
    p.noiseReduction = .15f; p.deEsserAmount = .25f;
    p.gateThreshold = -60; p.compressorRatio = 2.4f;
    p.agcAmount = 0; p.multibandAmount = 0;
    p.formantPreserve = false; p.economyMode = true;
    return;
  }

  if (name.startsWith("Voz ")) {
    int index = -1;
    for (int i = 0; i < generatedVoiceCount; ++i)
      if (name == generatedName(i)) { index = i; break; }
    if (index >= 0) {
      const std::uint32_t seed =
          static_cast<std::uint32_t>(index + 1) * 2654435761u;
      const auto unit = [seed](int shift) {
        return (float)((seed >> shift) & 255u) / 255.0f;
      };
      const int family = index % 10;
      p.pitchSemitones = -.8f + 1.6f * unit(0);
      p.formant = -.12f + .24f * unit(8);
      p.hpFreq = 65.0f + 55.0f * unit(16);
      p.lpFreq = 14500.0f + 3500.0f * unit(4);
      p.chorus = .04f * unit(12); p.reverb = .08f * unit(2);
      p.compressorThreshold = -24.0f + 8.0f * unit(5);
      p.compressorRatio = 2.2f + 1.8f * unit(13);
      if (family == 1) { p.pitchSemitones=-3.0f-3.5f*unit(0); p.formant=-.28f-.30f*unit(8); p.hpFreq=55; }
      else if (family == 2) { p.pitchSemitones=2.2f+3.8f*unit(0); p.formant=.22f+.30f*unit(8); p.chorus=.06f+.08f*unit(12); }
      else if (family == 3) { p.pitchSemitones=-2.0f+4.0f*unit(0); p.ringMod=.22f+.28f*unit(6); p.bitCrush=.08f+.16f*unit(14); p.chorus=.12f; }
      else if (family == 4) { p.pitchSemitones=-5.0f-3.0f*unit(0); p.formant=-.38f; p.distortion=.10f+.16f*unit(18); p.reverb=.15f+.16f*unit(2); }
      else if (family == 5) { p.pitchSemitones=-3.0f+6.0f*unit(0); p.formant=-.35f+.70f*unit(8); p.chorus=.10f+.16f*unit(12); p.delay=.04f+.10f*unit(10); }
      else if (family == 6) { p.hpFreq=320.0f+240.0f*unit(16); p.lpFreq=2800.0f+1800.0f*unit(4); p.distortion=.08f+.12f*unit(18); p.bitCrush=.04f+.08f*unit(14); }
      else if (family == 7) { p.pitchSemitones=-4.0f+9.0f*unit(0); p.formant=-.45f+.90f*unit(8); p.chorus=.05f+.10f*unit(12); }
      else if (family == 8) { p.pitchSemitones=-2.0f+4.0f*unit(0); p.formant=-.2f+.4f*unit(8); p.reverb=.20f+.15f*unit(2); p.delay=.08f+.12f*unit(10); p.chorus=.12f; }
      else if (family == 9) { p.pitchSemitones=-1.5f+3.0f*unit(0); p.formant=-.18f+.36f*unit(8); p.compressorRatio=3.0f+2.0f*unit(13); p.chorus=.04f; }
      if (std::abs(p.pitchSemitones.load()) > 6.0f || family >= 3)
        p.formantPreserve = false;
      return;
    }
  }
  if (name == "Masculina grave") { p.pitchSemitones=-4; p.formant=-.3f; }
  else if (name == "Feminina suave") { p.pitchSemitones=3; p.formant=.25f; p.chorus=.12f; p.reverb=.12f; }
  else if (name == juce::String::fromUTF8("Robô")) { p.ringMod=.8f; p.bitCrush=.2f; }
  else if (name == juce::String::fromUTF8("Rádio policial")) { p.hpFreq=400; p.lpFreq=3500; p.distortion=.18f; }
  else if (name == "Telefone") { p.hpFreq=500; p.lpFreq=3000; p.bitCrush=.12f; }
  else if (name == juce::String::fromUTF8("Demônio")) { p.pitchSemitones=-8; p.formantPreserve=false; p.distortion=.35f; p.reverb=.25f; }
  else if (name == juce::String::fromUTF8("Alienígena")) { p.pitchSemitones=5; p.formantPreserve=false; p.ringMod=.45f; p.delay=.15f; }
  else if (name == "Monstro") { p.pitchSemitones=-10; p.formantPreserve=false; p.distortion=.25f; p.chorus=.2f; }
  else if (name == "Personagem infantil") { p.pitchSemitones=7; p.formantPreserve=false; p.formant=.5f; }
  else if (name == "Narrador") { p.pitchSemitones=-2; p.compressorRatio=4; p.reverb=.08f; }
  else if (name == "Megafone") { p.hpFreq=350; p.lpFreq=4500; p.distortion=.3f; p.delay=.08f; }
}
} // namespace vox
