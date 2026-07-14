#include "ModulePage.h"
#include "UI/Theme.h"
#include <cmath>

namespace vox {
namespace {
void moduleLabel(juce::Label &label, float size, juce::Colour colour,
                 bool bold = false) {
  label.setColour(juce::Label::textColourId, colour);
  label.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}
} // namespace

ModulePage::ModulePage(Kind pageKind, AudioEngine &audio,
                       PresetManager &presetManager,
                       SettingsManager &settingsManager)
    : kind(pageKind), engine(audio), presets(presetManager),
      settings(settingsManager) {
  moduleLabel(title, 24, theme::text, true);
  moduleLabel(description, 13, theme::muted);
  moduleLabel(status, 14, theme::text, true);
  moduleLabel(emptyState, 14, theme::muted);
  emptyState.setJustificationType(juce::Justification::topLeft);
  for (auto *label : {&title, &description, &status, &emptyState})
    addAndMakeVisible(label);
  for (auto *button : {&primary, &secondary, &tertiary})
    addAndMakeVisible(button);
  addAndMakeVisible(presetList);
  addAndMakeVisible(soundList);
  soundProgress.setSliderStyle(juce::Slider::LinearHorizontal);
  soundProgress.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  soundProgress.setEnabled(false);
  soundProgress.setRange(0.0, 1.0);
  addAndMakeVisible(soundProgress);
  for (auto *slider : {&hp, &lp}) {
    slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 75, 24);
    addAndMakeVisible(slider);
  }
  cardViewport.setViewedComponent(&cardCanvas, false);
  cardViewport.setScrollBarsShown(true, false);
  addAndMakeVisible(cardViewport);
  setup();
  startTimerHz(10);
}

ModulePage::~ModulePage() { stopTimer(); }

void ModulePage::setup() {
  primary.setColour(juce::TextButton::buttonColourId, theme::blue);
  presetList.addItemList(presets.names(), 1);
  switch (kind) {
  case Kind::Home:
    title.setText("Início", juce::dontSendNotification);
    description.setText("Visão geral do áudio e acesso rápido às funções principais.",
                        juce::dontSendNotification);
    primary.setButtonText("Abrir vozes");
    secondary.setButtonText("Configurar dispositivos");
    tertiary.setButtonText("Alternar bypass");
    primary.onClick = [this] { if (navigate) navigate(PageId::Voices); };
    secondary.onClick = [this] { if (navigate) navigate(PageId::Devices); };
    tertiary.onClick = [this] {
      engine.parameters().bypass = !engine.parameters().bypass.load();
    };
    break;
  case Kind::Effects:
    title.setText("Biblioteca de Efeitos", juce::dontSendNotification);
    description.setText("Ative efeitos conectados diretamente ao processador de voz.",
                        juce::dontSendNotification);
    primary.setButtonText("Limpar efeitos");
    secondary.setVisible(false);
    tertiary.setVisible(false);
    primary.onClick = [this] {
      auto &p = engine.parameters();
      p.reverb = p.delay = p.chorus = p.flanger = p.distortion = p.bitCrush =
          p.ringMod = 0.0f;
      for (auto *card : cards) card->setSelected(false);
      if (notify) notify("Efeitos desativados");
    };
    buildEffects();
    break;
  case Kind::Soundboard:
    title.setText("Painel de Som", juce::dontSendNotification);
    description.setText("Importe, selecione e reproduza WAV, MP3, OGG, FLAC, AIFF ou AIF.",
                        juce::dontSendNotification);
    primary.setButtonText("+ Importar som");
    secondary.setButtonText("Remover selecionado");
    tertiary.setButtonText("Reproduzir");
    secondary.setEnabled(false);
    tertiary.setEnabled(false);
    soundList.setTextWhenNothingSelected("Selecione um arquivo importado");
    soundList.onChange = [this] { selectSound(soundList.getSelectedId() - 1); };
    primary.onClick = [this] {
      chooser = std::make_unique<juce::FileChooser>(
          "Selecionar arquivo de áudio",
          juce::File::getSpecialLocation(juce::File::userMusicDirectory),
          "*.wav;*.mp3;*.ogg;*.flac;*.aiff;*.aif", true);
      chooser->launchAsync(
          juce::FileBrowserComponent::openMode |
              juce::FileBrowserComponent::canSelectFiles,
          [this](const juce::FileChooser &fileChooser) {
            auto file = fileChooser.getResult();
            if (!file.existsAsFile()) {
              status.setText("Nenhum arquivo foi selecionado.",
                             juce::dontSendNotification);
              return;
            }
            for (int i = 0; i < importedSounds.size(); ++i)
              if (importedSounds[i] == file) {
                soundList.setSelectedId(i + 1);
                return;
              }
            importedSounds.add(file);
            soundList.addItem(file.getFileName(), importedSounds.size());
            soundList.setSelectedId(importedSounds.size());
          });
    };
    secondary.onClick = [this] {
      const int index = soundList.getSelectedId() - 1;
      if (!juce::isPositiveAndBelow(index, importedSounds.size())) return;
      engine.stopSoundboard();
      importedSounds.remove(index);
      soundList.clear();
      for (int i = 0; i < importedSounds.size(); ++i)
        soundList.addItem(importedSounds[i].getFileName(), i + 1);
      selectedSound = {};
      secondary.setEnabled(false);
      tertiary.setEnabled(false);
      emptyState.setText(importedSounds.isEmpty()
                             ? "Nenhum som importado. Clique em + Importar som."
                             : "Selecione outro arquivo na lista.",
                         juce::dontSendNotification);
    };
    tertiary.onClick = [this] {
      if (!selectedSound.existsAsFile()) return;
      if (engine.isSoundboardPlaying()) engine.stopSoundboard();
      else engine.playSoundboard();
    };
    emptyState.setText("Nenhum som importado. Clique em + Importar som para começar.",
                       juce::dontSendNotification);
    break;
  case Kind::Favorites:
    title.setText("Favoritos", juce::dontSendNotification);
    description.setText("Presets marcados como favoritos na biblioteca de vozes.",
                        juce::dontSendNotification);
    primary.setVisible(false);
    secondary.setVisible(false);
    tertiary.setVisible(false);
    rebuildFavorites();
    break;
  case Kind::Equalizer:
    title.setText("Equalizador", juce::dontSendNotification);
    description.setText("Resposta real dos filtros globais do processamento.",
                        juce::dontSendNotification);
    primary.setButtonText("Redefinir filtros");
    secondary.setVisible(false);
    tertiary.setVisible(false);
    hp.setRange(20, 2000, 1);
    lp.setRange(2000, 20000, 10);
    hp.setValue(engine.parameters().hpFreq.load(), juce::dontSendNotification);
    lp.setValue(engine.parameters().lpFreq.load(), juce::dontSendNotification);
    hp.onValueChange = [this] {
      engine.parameters().hpFreq = static_cast<float>(hp.getValue());
      repaint();
    };
    lp.onValueChange = [this] {
      engine.parameters().lpFreq = static_cast<float>(lp.getValue());
      repaint();
    };
    primary.onClick = [this] { hp.setValue(70); lp.setValue(18000); };
    emptyState.setText("Passa-altas                                      Passa-baixas",
                       juce::dontSendNotification);
    break;
  case Kind::Presets:
    title.setText("Presets", juce::dontSendNotification);
    description.setText("Carregue ou salve configurações de voz em JSON.",
                        juce::dontSendNotification);
    primary.setButtonText("Aplicar preset");
    secondary.setButtonText("Salvar cópia");
    tertiary.setVisible(false);
    primary.onClick = [this] {
      if (presetList.getSelectedId() <= 0) return;
      presets.load(presetList.getText(), engine.parameters());
      status.setText("Preset aplicado: " + presetList.getText(),
                     juce::dontSendNotification);
    };
    secondary.onClick = [this] {
      auto name = presetList.getText().isNotEmpty()
                      ? presetList.getText() + " - cópia"
                      : "Meu preset";
      presets.save(name, engine.parameters());
      presetList.clear();
      presetList.addItemList(presets.names(), 1);
      status.setText("Preset salvo: " + name, juce::dontSendNotification);
    };
    break;
  }
}

void ModulePage::buildEffects() {
  struct Effect { const char *name; std::atomic<float> Parameters::*value; };
  const Effect effects[]{{"Reverb", &Parameters::reverb},
                         {"Delay", &Parameters::delay},
                         {"Chorus", &Parameters::chorus},
                         {"Flanger", &Parameters::flanger},
                         {"Distorção", &Parameters::distortion},
                         {"Bit Crusher", &Parameters::bitCrush},
                         {"Ring Modulation", &Parameters::ringMod}};
  for (int i = 0; i < static_cast<int>(std::size(effects)); ++i) {
    auto *card = cards.add(new VoiceCardComponent(effects[i].name, "Efeito", i));
    card->onClick = [this, card, member = effects[i].value] {
      auto &value = engine.parameters().*member;
      const bool enabled = value.load() > 0.0f;
      value = enabled ? 0.0f : 0.5f;
      card->setSelected(!enabled);
      if (notify) notify(card->presetName() + (enabled ? " desativado" : " ativado"));
    };
    card->onFavourite = [card](bool) { card->setFavourite(false); };
    cardCanvas.addAndMakeVisible(card);
  }
}

void ModulePage::rebuildFavorites() {
  const auto favourites = settings.favourites();
  const auto signature = favourites.joinIntoString("\n");
  if (favoritesInitialized && signature == favouriteSignature) return;
  favoritesInitialized = true;
  favouriteSignature = signature;
  cards.clear();
  for (const auto &name : favourites) {
    auto *card = cards.add(
        new VoiceCardComponent(name, presets.categoryFor(name), cards.size()));
    card->setFavourite(true);
    card->onClick = [this, name] {
      presets.load(name, engine.parameters());
      if (notify) notify("Favorito aplicado: " + name);
    };
    card->onFavourite = [this, name](bool enabled) {
      settings.setFavourite(name, enabled);
      favouriteSignature.clear();
    };
    cardCanvas.addAndMakeVisible(card);
  }
  emptyState.setText(favourites.isEmpty()
                         ? "Nenhum favorito salvo. Marque presets na biblioteca."
                         : juce::String(),
                     juce::dontSendNotification);
  layoutCards();
  repaint();
}

void ModulePage::selectSound(int index) {
  if (!juce::isPositiveAndBelow(index, importedSounds.size())) return;
  selectedSound = importedSounds[index];
  const auto error = engine.loadSoundboardFile(selectedSound);
  secondary.setEnabled(true);
  tertiary.setEnabled(error.isEmpty());
  if (error.isNotEmpty()) {
    status.setText(error, juce::dontSendNotification);
    return;
  }
  emptyState.setText("Arquivo selecionado: " + selectedSound.getFileName(),
                     juce::dontSendNotification);
  status.setText("Pronto para reproduzir.", juce::dontSendNotification);
}

void ModulePage::timerCallback() {
  if (kind == Kind::Favorites) {
    rebuildFavorites();
    return;
  }
  if (kind == Kind::Soundboard) {
    const auto duration = engine.soundboardDuration();
    const auto position = engine.soundboardPosition();
    soundProgress.setValue(duration > 0.0 ? position / duration : 0.0,
                           juce::dontSendNotification);
    tertiary.setButtonText(engine.isSoundboardPlaying() ? "Parar" : "Reproduzir");
    if (selectedSound.existsAsFile())
      status.setText((engine.isSoundboardPlaying() ? "Reproduzindo: " : "Selecionado: ") +
                         selectedSound.getFileName() + "   " +
                         juce::String(position, 1) + " / " +
                         juce::String(duration, 1) + " s",
                     juce::dontSendNotification);
    return;
  }
  if (kind == Kind::Equalizer) {
    if (std::abs(hp.getValue() - engine.parameters().hpFreq.load()) > 0.5)
      hp.setValue(engine.parameters().hpFreq.load(), juce::dontSendNotification);
    if (std::abs(lp.getValue() - engine.parameters().lpFreq.load()) > 0.5)
      lp.setValue(engine.parameters().lpFreq.load(), juce::dontSendNotification);
    repaint();
    return;
  }
  if (kind != Kind::Home) return;
  auto *device = engine.deviceManager().getCurrentAudioDevice();
  juce::String text = "Processamento: " +
      juce::String(engine.isRunning() ? "ATIVO" : "DESLIGADO") + "\n";
  text += "Dispositivo: " + (device ? device->getName() : "Não disponível") + "\n";
  if (device)
    text += "Sample rate: " + juce::String(device->getCurrentSampleRate(), 0) +
            " Hz   Buffer: " + juce::String(device->getCurrentBufferSizeSamples()) + "\n";
  text += "CPU DSP: " + juce::String(engine.cpuUsage() * 100, 1) +
          "%   Underruns: " + juce::String((juce::int64)engine.underruns());
  status.setText(text, juce::dontSendNotification);
}

void ModulePage::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  for (auto panel : panels)
    theme::roundedPanel(g, panel.toFloat(), 12, theme::panel);
  if (kind != Kind::Equalizer || panels.isEmpty()) return;
  auto graph = panels[0].toFloat().reduced(30, 145);
  if (graph.getHeight() < 60) return;
  g.setColour(theme::border);
  for (int i = 0; i <= 8; ++i)
    g.drawVerticalLine((int)(graph.getX() + i * graph.getWidth() / 8.0f),
                       graph.getY(), graph.getBottom());
  for (int i = 0; i <= 4; ++i)
    g.drawHorizontalLine((int)(graph.getY() + i * graph.getHeight() / 4.0f),
                         graph.getX(), graph.getRight());
  const double hpFreq = engine.parameters().hpFreq.load();
  const double lpFreq = engine.parameters().lpFreq.load();
  juce::Path response;
  for (int x = 0; x <= (int)graph.getWidth(); ++x) {
    const double proportion = x / (double)graph.getWidth();
    const double frequency = 20.0 * std::pow(1000.0, proportion);
    const double hpRatio = frequency / juce::jmax(20.0, hpFreq);
    const double lpRatio = frequency / juce::jmax(20.0, lpFreq);
    const double hpMagnitude = hpRatio / std::sqrt(1.0 + hpRatio * hpRatio);
    const double lpMagnitude = 1.0 / std::sqrt(1.0 + lpRatio * lpRatio);
    const double db = juce::jlimit(-36.0, 3.0,
        juce::Decibels::gainToDecibels(hpMagnitude * lpMagnitude, -60.0));
    const float y = juce::jmap((float)db, -36.0f, 3.0f,
                               graph.getBottom(), graph.getY());
    if (x == 0) response.startNewSubPath(graph.getX(), y);
    else response.lineTo(graph.getX() + (float)x, y);
  }
  g.setColour(theme::cyan);
  g.strokePath(response, juce::PathStrokeType(2.5f));
}

void ModulePage::layoutCards() {
  const int width = juce::jmax(420, cardViewport.getWidth() - 14);
  const int columns = juce::jlimit(2, 6, juce::jmax(2, width / 190));
  const int gap = 12;
  const int cardWidth = (width - 16 - (columns - 1) * gap) / columns;
  int x = 8, y = 8, column = 0;
  for (auto *card : cards) {
    card->setBounds(x, y, cardWidth, 205);
    if (++column == columns) { column = 0; x = 8; y += 217; }
    else x += cardWidth + gap;
  }
  cardCanvas.setSize(width, juce::jmax(cardViewport.getHeight(), y + 217));
}

void ModulePage::resized() {
  auto area = getLocalBounds().reduced(18);
  title.setBounds(area.removeFromTop(34));
  description.setBounds(area.removeFromTop(28));
  auto buttons = area.removeFromTop(48);
  primary.setBounds(buttons.removeFromLeft(180).reduced(3));
  secondary.setBounds(buttons.removeFromLeft(190).reduced(3));
  tertiary.setBounds(buttons.removeFromLeft(150).reduced(3));
  area.removeFromTop(10);
  panels.clear();
  panels.add(area);
  auto content = area.reduced(24);
  soundList.setBounds({}); soundProgress.setBounds({}); presetList.setBounds({});
  hp.setBounds({}); lp.setBounds({}); cardViewport.setBounds({});
  status.setBounds({}); emptyState.setBounds({});
  if (kind == Kind::Home) {
    status.setBounds(content.removeFromTop(150));
    emptyState.setText("Ações rápidas usam dados reais do AudioEngine.",
                       juce::dontSendNotification);
    emptyState.setBounds(content.removeFromTop(50));
  } else if (kind == Kind::Effects || kind == Kind::Favorites) {
    emptyState.setBounds(content.removeFromTop(kind == Kind::Favorites ? 45 : 0));
    cardViewport.setBounds(content);
    layoutCards();
  } else if (kind == Kind::Soundboard) {
    soundList.setBounds(content.removeFromTop(42));
    content.removeFromTop(10);
    emptyState.setBounds(content.removeFromTop(65));
    status.setBounds(content.removeFromTop(36));
    soundProgress.setBounds(content.removeFromTop(28));
  } else if (kind == Kind::Equalizer) {
    emptyState.setBounds(content.removeFromTop(35));
    hp.setBounds(content.removeFromTop(48));
    lp.setBounds(content.removeFromTop(48));
  } else if (kind == Kind::Presets) {
    presetList.setBounds(content.removeFromTop(42));
    status.setBounds(content.removeFromTop(60));
  }
}

bool ModulePage::runSmokeTest() {
  setBounds(0, 0, 1100, 700);
  resized();
  if (panels.isEmpty() || panels[0].isEmpty()) return false;
  if ((kind == Kind::Effects || kind == Kind::Favorites) &&
      cardViewport.getBounds().isEmpty()) return false;
  if (kind == Kind::Equalizer && (hp.getBounds().isEmpty() || lp.getBounds().isEmpty()))
    return false;
  return true;
}
} // namespace vox
