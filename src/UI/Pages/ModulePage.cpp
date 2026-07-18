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
  for (auto *label : {&homeInput, &homeOutput, &homeVoice, &homeIntegration}) {
    moduleLabel(*label, 13, theme::text, true);
    label->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);
  }
  for (auto *button : {&primary, &secondary, &tertiary})
    addAndMakeVisible(button);
  addAndMakeVisible(presetList);
  addAndMakeVisible(soundList);
  soundProgress.setSliderStyle(juce::Slider::LinearHorizontal);
  soundProgress.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  soundProgress.setEnabled(false);
  soundProgress.setRange(0.0, 1.0);
  addAndMakeVisible(soundProgress);
  for (auto *slider : {&hp, &lp, &bass, &mid, &treble}) {
    slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 75, 24);
    addAndMakeVisible(slider);
  }
  for (auto *slider : {&homeInputGain, &homeOutputGain, &homeMix, &homeNoise}) {
    slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
    addAndMakeVisible(slider);
  }
  addAndMakeVisible(homeInputMeter);
  addAndMakeVisible(homeOutputMeter);
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
    homeInputGain.setRange(-24, 24, .1);
    homeOutputGain.setRange(-24, 24, .1);
    homeMix.setRange(0, 100, 1);
    homeNoise.setRange(0, 100, 1);
    homeInputGain.setTextValueSuffix(" dB");
    homeOutputGain.setTextValueSuffix(" dB");
    homeMix.setTextValueSuffix(" %");
    homeNoise.setTextValueSuffix(" %");
    homeInputGain.setValue(engine.parameters().inputGainDb.load(), juce::dontSendNotification);
    homeOutputGain.setValue(engine.parameters().outputGainDb.load(), juce::dontSendNotification);
    homeMix.setValue(engine.parameters().mix.load() * 100.0, juce::dontSendNotification);
    homeNoise.setValue(engine.parameters().noiseReduction.load() * 100.0, juce::dontSendNotification);
    homeInputGain.onValueChange = [this] { engine.parameters().inputGainDb = (float)homeInputGain.getValue(); };
    homeOutputGain.onValueChange = [this] { engine.parameters().outputGainDb = (float)homeOutputGain.getValue(); };
    homeMix.onValueChange = [this] { engine.parameters().mix = (float)homeMix.getValue() / 100.0f; };
    homeNoise.onValueChange = [this] { engine.parameters().noiseReduction = (float)homeNoise.getValue() / 100.0f; };
    refreshHomeFavorites();
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
            rebuildSoundPads();
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
      rebuildSoundPads();
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
    rebuildSoundPads();
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
    bass.setRange(-12, 12, .1);
    mid.setRange(-12, 12, .1);
    treble.setRange(-12, 12, .1);
    hp.setTextValueSuffix(" Hz");
    lp.setTextValueSuffix(" Hz");
    bass.setTextValueSuffix(" dB");
    mid.setTextValueSuffix(" dB");
    treble.setTextValueSuffix(" dB");
    hp.setValue(engine.parameters().hpFreq.load(), juce::dontSendNotification);
    lp.setValue(engine.parameters().lpFreq.load(), juce::dontSendNotification);
    bass.setValue(engine.parameters().bassDb.load(), juce::dontSendNotification);
    mid.setValue(engine.parameters().midDb.load(), juce::dontSendNotification);
    treble.setValue(engine.parameters().trebleDb.load(), juce::dontSendNotification);
    hp.onValueChange = [this] {
      engine.parameters().hpFreq = static_cast<float>(hp.getValue());
      repaint();
    };
    lp.onValueChange = [this] {
      engine.parameters().lpFreq = static_cast<float>(lp.getValue());
      repaint();
    };
    bass.onValueChange = [this] { engine.parameters().bassDb = (float)bass.getValue(); repaint(); };
    mid.onValueChange = [this] { engine.parameters().midDb = (float)mid.getValue(); repaint(); };
    treble.onValueChange = [this] { engine.parameters().trebleDb = (float)treble.getValue(); repaint(); };
    primary.onClick = [this] { hp.setValue(70); lp.setValue(18000); bass.setValue(0); mid.setValue(0); treble.setValue(0); };
    emptyState.setText("Filtros de corte e equalização tonal em dB",
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
      const auto name = presetList.getText();
      const bool ok = presets.load(name, engine.parameters());
      if (ok) settings.setPreference("activePreset", name);
      status.setText(ok ? "Preset aplicado: " + name
                        : "Falha ao carregar o preset: " + name,
                     juce::dontSendNotification);
      status.setColour(juce::Label::textColourId,
                       ok ? theme::green : theme::red);
    };
    secondary.onClick = [this] {
      auto name = presetList.getText().isNotEmpty()
                      ? presetList.getText() + " - cópia"
                      : "Meu preset";
      if (!presets.save(name, engine.parameters())) {
        status.setText("Não foi possível salvar o preset.",
                       juce::dontSendNotification);
        status.setColour(juce::Label::textColourId, theme::red);
        return;
      }
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
      if (presets.load(name, engine.parameters())) {
        settings.setPreference("activePreset", name);
        if (notify) notify("Favorito aplicado: " + name);
      } else if (notify) notify("Não foi possível carregar o favorito");
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

void ModulePage::refreshHomeFavorites() {
  if (kind != Kind::Home) return;
  const auto favourites = settings.favourites();
  const auto signature = favourites.joinIntoString("\n");
  if (signature == homeFavouriteSignature && homeFavorites.size() > 0) return;
  homeFavouriteSignature = signature;
  homeFavorites.clear();
  const int count = juce::jmin(4, favourites.size());
  for (int i = 0; i < count; ++i) {
    const auto name = favourites[i];
    auto *button = homeFavorites.add(new juce::TextButton("★  " + name));
    button->setColour(juce::TextButton::buttonColourId, theme::elevated);
    button->onClick = [this, name] {
      if (presets.load(name, engine.parameters())) {
        settings.setPreference("activePreset", name);
        if (notify) notify("Favorito aplicado: " + name);
      } else if (notify) notify("Não foi possível carregar o favorito");
    };
    addAndMakeVisible(button);
  }
  if (homeFavorites.isEmpty()) {
    auto *button = homeFavorites.add(new juce::TextButton("Adicionar favoritos na biblioteca"));
    button->onClick = [this] { if (navigate) navigate(PageId::Voices); };
    addAndMakeVisible(button);
  }
  resized();
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

void ModulePage::rebuildSoundPads() {
  if (kind != Kind::Soundboard) return;
  soundPads.clear();
  for (int i = 0; i < importedSounds.size(); ++i) {
    auto *button = soundPads.add(
        new juce::TextButton("▶  " + importedSounds[i].getFileNameWithoutExtension()));
    button->setColour(juce::TextButton::buttonColourId, theme::elevated);
    button->setTooltip("Carregar e reproduzir " + importedSounds[i].getFileName());
    button->onClick = [this, i] {
      selectSound(i);
      if (!selectedSound.existsAsFile() || !tertiary.isEnabled()) return;
      soundList.setSelectedId(i + 1, juce::dontSendNotification);
      engine.playSoundboard();
    };
    addAndMakeVisible(button);
  }
  resized();
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
    if (std::abs(bass.getValue() - engine.parameters().bassDb.load()) > .05)
      bass.setValue(engine.parameters().bassDb.load(), juce::dontSendNotification);
    if (std::abs(mid.getValue() - engine.parameters().midDb.load()) > .05)
      mid.setValue(engine.parameters().midDb.load(), juce::dontSendNotification);
    if (std::abs(treble.getValue() - engine.parameters().trebleDb.load()) > .05)
      treble.setValue(engine.parameters().trebleDb.load(), juce::dontSendNotification);
    repaint();
    return;
  }
  if (kind != Kind::Home) return;
  const auto setup = engine.deviceManager().getAudioDeviceSetup();
  homeInput.setText("ENTRADA\n" + (setup.inputDeviceName.isNotEmpty()
      ? setup.inputDeviceName : "Microfone não encontrado"), juce::dontSendNotification);
  homeOutput.setText("SAÍDA\n" + (setup.outputDeviceName.isNotEmpty()
      ? setup.outputDeviceName : "Saída não encontrada"), juce::dontSendNotification);
  homeInputMeter.setLevels(engine.processor().inputPeak(), engine.processor().inputPeak());
  homeOutputMeter.setLevels(engine.processor().outputPeak(), engine.processor().outputPeak());
  status.setText(engine.isRunning()
      ? "● PROCESSAMENTO ATIVO  ·  CPU " + juce::String(engine.cpuUsage() * 100, 1) +
            "%  ·  " + juce::String((juce::int64)engine.underruns()) + " underruns"
      : "○ PROCESSAMENTO DESLIGADO  ·  clique em Ativar no rodapé",
      juce::dontSendNotification);
  status.setColour(juce::Label::textColourId,
                   engine.isRunning() ? theme::green : theme::muted);
  if (++homeRefreshCounter >= 20) {
    homeRefreshCounter = 0;
    homeVoice.setText(
        "VOZ ATIVA\n" +
            settings.preference("activePreset", "Voz normal limpa"),
        juce::dontSendNotification);
    const bool virtualRoute =
        VirtualDeviceDetector::isVirtual(setup.outputDeviceName);
    const bool voiceAppOpen = ApplicationDetector::isTargetRunning("Discord") ||
                              ApplicationDetector::isTargetRunning("FiveM") ||
                              ApplicationDetector::isTargetRunning("OBS");
    homeIntegration.setText(
        virtualRoute ? "ROTEAMENTO\nSaída virtual configurada"
        : voiceAppOpen ? "INTEGRAÇÕES\nAplicativo aberto · falta saída virtual"
                       : "INTEGRAÇÕES\nNenhum aplicativo detectado",
        juce::dontSendNotification);
    homeIntegration.setColour(
        juce::Label::textColourId,
        virtualRoute ? theme::green
                     : voiceAppOpen ? theme::yellow : theme::muted);
    refreshHomeFavorites();
  }
}

void ModulePage::paint(juce::Graphics &g) {
  theme::paintBackground(g, getLocalBounds());
  for (auto panel : panels)
    theme::roundedPanel(g, panel.toFloat(), 12, theme::panel);
  if (kind == Kind::Home) {
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.drawText("Controle rápido", homeControlsArea.reduced(18).removeFromTop(24),
               juce::Justification::centredLeft);
    g.drawText("Vozes favoritas", homeFavoritesArea.reduced(18).removeFromTop(24),
               juce::Justification::centredLeft);
    const juce::Slider *controls[]{&homeInputGain, &homeOutputGain, &homeMix, &homeNoise};
    const char *names[]{"Ganho de entrada", "Volume de saída", "Intensidade", "Redução de ruído"};
    g.setColour(theme::muted);
    g.setFont(12.0f);
    for (int i = 0; i < 4; ++i)
      g.drawText(juce::String::fromUTF8(names[i]), controls[i]->getBounds().withWidth(124),
                 juce::Justification::centredLeft);
    return;
  }
  if (kind != Kind::Equalizer || equalizerGraphArea.isEmpty()) return;
  const juce::Slider *toneControls[]{&hp, &lp, &bass, &mid, &treble};
  const char *toneNames[]{"Passa-altas", "Passa-baixas", "Graves", "Médios", "Agudos"};
  g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
  g.setColour(theme::muted);
  for (int i = 0; i < 5; ++i)
    g.drawText(juce::String::fromUTF8(toneNames[i]),
               toneControls[i]->getBounds().withWidth(104),
               juce::Justification::centredLeft);
  auto graph = equalizerGraphArea.toFloat();
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
    const double lowWeight = 1.0 / (1.0 + std::pow(frequency / 250.0, 4.0));
    const double highWeight = 1.0 / (1.0 + std::pow(4000.0 / frequency, 4.0));
    const double midWeight = juce::jmax(0.0, 1.0 - lowWeight - highWeight);
    const double toneDb = lowWeight * engine.parameters().bassDb.load() +
                          midWeight * engine.parameters().midDb.load() +
                          highWeight * engine.parameters().trebleDb.load();
    const double db = juce::jlimit(-36.0, 12.0,
        juce::Decibels::gainToDecibels(hpMagnitude * lpMagnitude, -60.0) + toneDb);
    const float y = juce::jmap((float)db, -36.0f, 12.0f,
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
  hp.setBounds({}); lp.setBounds({}); bass.setBounds({}); mid.setBounds({});
  treble.setBounds({}); cardViewport.setBounds({});
  status.setBounds({}); emptyState.setBounds({});
  homeInput.setBounds({}); homeOutput.setBounds({}); homeVoice.setBounds({});
  homeIntegration.setBounds({}); homeInputGain.setBounds({});
  homeOutputGain.setBounds({}); homeMix.setBounds({}); homeNoise.setBounds({});
  homeInputMeter.setBounds({}); homeOutputMeter.setBounds({});
  for (auto *button : homeFavorites) button->setBounds({});
  for (auto *button : soundPads) button->setBounds({});
  homeHeroArea = {}; homeStatsArea = {}; homeControlsArea = {}; homeFavoritesArea = {};
  equalizerGraphArea = {};
  if (kind == Kind::Home) {
    panels.clear();
    homeHeroArea = content.removeFromTop(82);
    panels.add(homeHeroArea);
    auto hero = homeHeroArea.reduced(18, 12);
    status.setBounds(hero.removeFromTop(26));
    homeInputMeter.setBounds(hero.removeFromTop(22));
    hero.removeFromTop(5);
    homeOutputMeter.setBounds(hero.removeFromTop(22));
    content.removeFromTop(12);
    homeStatsArea = content.removeFromTop(98);
    panels.add(homeStatsArea);
    auto stats = homeStatsArea.reduced(12);
    juce::Label *statLabels[]{&homeInput, &homeOutput, &homeVoice, &homeIntegration};
    for (int i = 0; i < 4; ++i)
      statLabels[i]->setBounds(
          stats.removeFromLeft(stats.getWidth() / (4 - i)).reduced(6));
    content.removeFromTop(12);
    homeControlsArea = content.removeFromLeft(content.getWidth() / 2 - 6);
    content.removeFromLeft(12);
    homeFavoritesArea = content;
    panels.add(homeControlsArea); panels.add(homeFavoritesArea);
    auto controls = homeControlsArea.reduced(18);
    controls.removeFromTop(34);
    auto place = [&controls](juce::Slider &slider) {
      auto row = controls.removeFromTop(40);
      slider.setBounds(row.withTrimmedLeft(128));
      controls.removeFromTop(4);
    };
    place(homeInputGain); place(homeOutputGain); place(homeMix); place(homeNoise);
    auto favourites = homeFavoritesArea.reduced(18);
    favourites.removeFromTop(34);
    for (auto *button : homeFavorites)
      button->setBounds(favourites.removeFromTop(40).reduced(0, 3));
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
    content.removeFromTop(8);
    const int columns = juce::jlimit(1, 4, juce::jmax(1, content.getWidth() / 210));
    const int gap = 8;
    const int padWidth = (content.getWidth() - gap * (columns - 1)) / columns;
    int column = 0;
    auto row = content.removeFromTop(44);
    for (auto *button : soundPads) {
      if (column == columns) {
        column = 0;
        content.removeFromTop(gap);
        row = content.removeFromTop(44);
      }
      button->setBounds(row.removeFromLeft(padWidth));
      if (++column < columns) row.removeFromLeft(gap);
    }
  } else if (kind == Kind::Equalizer) {
    emptyState.setBounds(content.removeFromTop(35));
    auto controls = content.removeFromLeft(juce::jmin(390, content.getWidth() / 2));
    content.removeFromLeft(22);
    equalizerGraphArea = content.reduced(8, 10);
    auto placeTone = [&controls](juce::Slider &slider, int height) {
      auto row = controls.removeFromTop(height);
      slider.setBounds(row.withTrimmedLeft(108));
      controls.removeFromTop(5);
    };
    placeTone(hp, 42); placeTone(lp, 42); placeTone(bass, 42);
    placeTone(mid, 42); placeTone(treble, 42);
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
