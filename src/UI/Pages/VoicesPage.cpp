#include "VoicesPage.h"
#include "UI/Theme.h"

namespace vox {
static void setupLabel(juce::Label &label, float size, juce::Colour colour,
                       bool bold = false) {
  label.setColour(juce::Label::textColourId, colour);
  label.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}

VoicesPage::VoicesPage(AudioEngine &e, SettingsManager &s, PresetManager &p)
    : engine(e), settings(s), presets(p) {
  title.setText("Biblioteca de Vozes", juce::dontSendNotification);
  subtitle.setText(
      juce::String::fromUTF8(
          "Escolha uma voz para transformar sua comunicação em tempo real."),
      juce::dontSendNotification);
  setupLabel(title, 26, theme::text, true);
  setupLabel(subtitle, 13, theme::muted);
  addAndMakeVisible(title);
  addAndMakeVisible(subtitle);
  createVoice.setColour(juce::TextButton::buttonColourId, theme::panel);
  createVoice.onClick = [this] {
    auto name = "Minha Voz " + juce::Time::getCurrentTime().formatted("%H%M%S");
    int suffix = 2;
    while (presets.names().contains(name))
      name = "Minha Voz " + juce::Time::getCurrentTime().formatted("%H%M%S") +
             " (" + juce::String(suffix++) + ")";
    if (presets.save(name, engine.parameters())) {
      addCustomVoiceCard(name, true);
      if (notify)
        notify("Voz personalizada criada: " + name);
    } else if (notify) {
      notify("Não foi possível salvar a voz personalizada");
    }
  };
  addAndMakeVisible(createVoice);
  const juce::StringArray filters{"Todas",
                                  "Favoritas",
                                  "Naturais",
                                  "Criativas",
                                  juce::String::fromUTF8("Robôs"),
                                  "Terror",
                                  juce::String::fromUTF8("Rádio"),
                                  "Dispositivos",
                                  "Mais"};
  for (auto name : filters) {
    auto *b = chips.add(new juce::TextButton(name));
    b->setColour(juce::TextButton::buttonColourId,
                 name == "Todas" ? theme::blue : theme::panel);
    b->onClick = [this, b] {
      activeFilter = b->getButtonText();
      for (auto *c : chips)
        c->setColour(juce::TextButton::buttonColourId,
                     c == b ? theme::blue : theme::panel);
      updateFilter();
    };
    addAndMakeVisible(b);
  }
  advancedFilter.setTooltip(juce::String::fromUTF8("Filtros avançados"));
  advancedFilter.onClick = [this] {
    activeFilter = "Todas";
    searchText.clear();
    for (int i = 0; i < chips.size(); ++i)
      chips[i]->setColour(juce::TextButton::buttonColourId,
                          i == 0 ? theme::blue : theme::panel);
    updateFilter();
    if (notify)
      notify("Filtros redefinidos");
  };
  addAndMakeVisible(advancedFilter);
  viewport.setViewedComponent(&grid, false);
  viewport.setScrollBarsShown(true, false);
  addAndMakeVisible(viewport);
  const juce::StringArray names{"Voz Limpa",
                                "Grave Profunda",
                                "Feminina Suave",
                                juce::String::fromUTF8("Robô"),
                                juce::String::fromUTF8("Rádio"),
                                "Telefone",
                                juce::String::fromUTF8("Demônio"),
                                juce::String::fromUTF8("Alienígena"),
                                "Megafone",
                                juce::String::fromUTF8("Voz Metálica"),
                                "Personagem Infantil",
                                "Narrador",
                                "Monstro",
                                "Eco Espacial",
                                "Som Digital",
                                "Grave Suave",
                                "Aguda",
                                juce::String::fromUTF8("Robô Pesado")};
  const juce::StringArray categories{"Naturais",
                                     "Graves",
                                     "Naturais",
                                     juce::String::fromUTF8("Robôs"),
                                     juce::String::fromUTF8("Rádio"),
                                     "Dispositivos",
                                     "Terror",
                                     "Criativas",
                                     "Criativas",
                                     juce::String::fromUTF8("Robôs"),
                                     "Criativas",
                                     "Naturais",
                                     "Terror",
                                     "Criativas",
                                     "Criativas",
                                     "Graves",
                                     "Agudas",
                                     juce::String::fromUTF8("Robôs")};
  auto favourites = settings.favourites();
  for (int i = 0; i < names.size(); ++i) {
    auto *c = cards.add(new VoiceCardComponent(names[i], categories[i], i));
    c->setSelected(i == 0);
    c->setFavourite(favourites.contains(names[i]));
    c->onClick = [this, i] { applyVoice(i); };
    c->onFavourite = [this, c](bool value) {
      settings.setFavourite(c->presetName(), value);
      if (activeFilter == "Favoritas")
        updateFilter();
      if (notify)
        notify(value ? "Adicionado aos favoritos" : "Removido dos favoritos");
    };
    grid.addAndMakeVisible(c);
  }
  for (int i = 0; i < PresetManager::generatedVoiceCount; ++i) {
    const auto name = PresetManager::generatedName(i);
    auto *card = cards.add(new VoiceCardComponent(
        name, PresetManager::generatedCategory(i), i + 20));
    card->setFavourite(favourites.contains(name));
    card->onClick = [this, card] { applyVoice(cards.indexOf(card)); };
    card->onFavourite = [this, card](bool value) {
      settings.setFavourite(card->presetName(), value);
      if (activeFilter == "Favoritas")
        updateFilter();
    };
    grid.addAndMakeVisible(card);
  }
  for (const auto &file : presets.directory().findChildFiles(
           juce::File::findFiles, false, "*.json"))
    addCustomVoiceCard(file.getFileNameWithoutExtension(), false);
  selectedCaption.setText("VOZ SELECIONADA", juce::dontSendNotification);
  selectedName.setText(names[0], juce::dontSendNotification);
  selectedType.setText(categories[0], juce::dontSendNotification);
  setupLabel(selectedCaption, 10, theme::muted, true);
  setupLabel(selectedName, 17, theme::text, true);
  setupLabel(selectedType, 12, theme::muted);
  for (auto *l : {&selectedCaption, &selectedName, &selectedType})
    addAndMakeVisible(l);
  favouriteButton.onClick = [this] {
    auto *c = cards[selectedIndex];
    c->setFavourite(!c->isFavourite());
    settings.setFavourite(c->presetName(), c->isFavourite());
    favouriteButton.setButtonText(c->isFavourite()
                                      ? juce::String::fromUTF8("♥")
                                      : juce::String::fromUTF8("♡"));
  };
  addAndMakeVisible(favouriteButton);
  menuButton.onClick = [this] {
    if (notify)
      notify(juce::String::fromUTF8(
          "Ações da voz: salvar, redefinir ou favoritar"));
  };
  addAndMakeVisible(menuButton);
  auto &pms = engine.parameters();
  std::atomic<float> *values[]{&pms.noiseReduction, &pms.outputGainDb, &pms.mix,
                               &pms.pitchSemitones, &pms.formant};
  const juce::StringArray labels{"Intensidade", "Volume da Voz",
                                 "Mistura Dry/Wet", "Pitch", "Formante"};
  const double ranges[][3] = {
      {0, 1, .01}, {-24, 24, .1}, {0, 1, .01}, {-12, 12, .1}, {-1, 1, .01}};
  for (int i = 0; i < 5; ++i) {
    auto *l = controlLabels.add(new juce::Label({}, labels[i]));
    auto *v = valueLabels.add(new juce::Label());
    setupLabel(*l, 12, theme::text);
    setupLabel(*v, 11, theme::muted, true);
    v->setJustificationType(juce::Justification::centredRight);
    auto *slider = sliders.add(new juce::Slider(juce::Slider::LinearHorizontal,
                                                juce::Slider::NoTextBox));
    slider->setRange(ranges[i][0], ranges[i][1], ranges[i][2]);
    slider->setValue(values[i]->load(), juce::dontSendNotification);
    slider->setColour(juce::Slider::trackColourId, theme::purple);
    slider->setColour(juce::Slider::thumbColourId, theme::text);
    slider->onValueChange = [this, slider, value = values[i], v, i] {
      value->store((float)slider->getValue());
      v->setText(i == 0 || i == 2
                     ? juce::String(slider->getValue() * 100, 0) + "%"
                     : juce::String(slider->getValue(), 1),
                 juce::dontSendNotification);
      markDirty();
    };
    slider->onValueChange();
    addAndMakeVisible(l);
    addAndMakeVisible(v);
    addAndMakeVisible(slider);
  }
  const juce::StringArray sections{
      "Voz",
      "Equalizador",
      juce::String::fromUTF8("Dinâmica"),
      "Ambiente",
      "Efeitos",
      juce::String::fromUTF8("Configurações Avançadas")};
  for (int i = 0; i < sections.size(); ++i) {
    auto name = sections[i];
    auto *b = accordions.add(new juce::TextButton(
        name +
        juce::String::fromUTF8("                                      ›")));
    b->setColour(juce::TextButton::buttonColourId, theme::panel);
    b->setTooltip(juce::String::fromUTF8("Expandir seção ") + name);
    b->onClick = [this, i] {
      const auto requested = static_cast<DetailsSection>(i + 1);
      selectDetailsSection(selectedSection == requested ? DetailsSection::None
                                                        : requested);
    };
    addAndMakeVisible(b);
  }
  sectionViewport.setViewedComponent(&sectionContent, false);
  sectionViewport.setScrollBarsShown(true, false);
  sectionViewport.setColour(juce::ScrollBar::thumbColourId, theme::purple);
  addAndMakeVisible(sectionViewport);
  setupLabel(sectionTitle, 13, theme::text, true);
  setupLabel(sectionBody, 11, theme::muted);
  sectionBody.setJustificationType(juce::Justification::topLeft);
  sectionContent.addAndMakeVisible(sectionTitle);
  sectionContent.addAndMakeVisible(sectionBody);
  selectDetailsSection(DetailsSection::Voice);
  dirtyLabel.setText(juce::String::fromUTF8("Alterações não salvas"),
                     juce::dontSendNotification);
  setupLabel(dirtyLabel, 11, theme::yellow, true);
  dirtyLabel.setVisible(false);
  addAndMakeVisible(dirtyLabel);
  saveButton.setColour(juce::TextButton::buttonColourId, theme::blue);
  saveButton.onClick = [this] {
    if (presets.save(selectedVoice(), engine.parameters())) {
      dirty = false;
      dirtyLabel.setVisible(false);
      saveButton.setEnabled(false);
      if (notify)
        notify(juce::String::fromUTF8("Alterações salvas"));
    }
  };
  resetButton.onClick = [this] {
    applyVoice(selectedIndex);
    dirty = false;
    dirtyLabel.setVisible(false);
    saveButton.setEnabled(false);
    if (notify)
      notify("Controles redefinidos");
  };
  saveButton.setEnabled(false);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(resetButton);
  dirty = false;
  dirtyLabel.setVisible(false);
  saveButton.setEnabled(false);
}

void VoicesPage::selectDetailsSection(DetailsSection section) {
  selectedSection = section;
  const juce::StringArray names{
      "Voz",
      "Equalizador",
      juce::String::fromUTF8("Dinâmica"),
      "Ambiente",
      "Efeitos",
      juce::String::fromUTF8("Configurações Avançadas")};
  const int selected = section == DetailsSection::None ? -1 : (int)section - 1;
  for (int i = 0; i < accordions.size(); ++i) {
    accordions[i]->setColour(juce::TextButton::buttonColourId,
                             i == selected ? theme::blue : theme::panel);
    accordions[i]->setButtonText(names[i] + (i == selected ? "  v" : "  >"));
  }
  auto &p = engine.parameters();
  juce::String body;
  switch (section) {
  case DetailsSection::Voice:
    body = "Pitch: " + juce::String(p.pitchSemitones.load(), 1) +
           "\nFormante: " + juce::String(p.formant.load(), 2) +
           "\nMistura: " + juce::String(p.mix.load() * 100, 0) + "%";
    break;
  case DetailsSection::Equalizer:
    body = "Passa-altas: " + juce::String(p.hpFreq.load(), 0) +
           " Hz\nPassa-baixas: " + juce::String(p.lpFreq.load(), 0) + " Hz";
    break;
  case DetailsSection::Dynamics:
    body = "Compressor: " +
           juce::String(p.compressorEnabled.load() ? "Ativo" : "Desativado") +
           "\nThreshold: " + juce::String(p.compressorThreshold.load(), 1) +
           " dB\nRatio: " + juce::String(p.compressorRatio.load(), 1);
    break;
  case DetailsSection::Environment:
    body = "Reverb: " + juce::String(p.reverb.load() * 100, 0) +
           "%\nDelay: " + juce::String(p.delay.load() * 100, 0) + "%";
    break;
  case DetailsSection::Effects:
    body = juce::String::fromUTF8("Distorção: ") + juce::String(p.distortion.load() * 100, 0) +
           "%\nChorus: " + juce::String(p.chorus.load() * 100, 0) +
           "%\nBit Crusher: " + juce::String(p.bitCrush.load() * 100, 0) + "%";
    break;
  case DetailsSection::Advanced:
    body = "Ganho de entrada: " + juce::String(p.inputGainDb.load(), 1) +
           juce::String::fromUTF8(" dB\nGanho de saída: ") + juce::String(p.outputGainDb.load(), 1) +
           " dB\nBypass: " +
           juce::String(p.bypass.load() ? "Ativo" : "Desativado");
    break;
  default:
    break;
  }
  sectionTitle.setText(selected >= 0 ? names[selected] : juce::String(),
                       juce::dontSendNotification);
  sectionBody.setText(body, juce::dontSendNotification);
  sectionContent.setSize(juce::jmax(180, sectionViewport.getWidth() - 10), 120);
  sectionTitle.setBounds(10, 8, sectionContent.getWidth() - 20, 22);
  sectionBody.setBounds(10, 34, sectionContent.getWidth() - 20, 80);
  sectionViewport.setViewPosition(0, 0);
  repaint();
}

juce::String VoicesPage::selectedVoice() const {
  return cards[selectedIndex]->presetName();
}
juce::String VoicesPage::selectedCategory() const {
  return cards[selectedIndex]->presetCategory();
}
void VoicesPage::addCustomVoiceCard(const juce::String &name,
                                    bool selectAfterAdding) {
  for (auto *existing : cards)
    if (existing->presetName() == name) {
      if (selectAfterAdding)
        applyVoice(cards.indexOf(existing));
      return;
    }
  auto *card = cards.add(
      new VoiceCardComponent(name, "Personalizadas", cards.size() + 3));
  card->setFavourite(settings.isFavourite(name));
  card->onClick = [this, card] { applyVoice(cards.indexOf(card)); };
  card->onFavourite = [this, card](bool value) {
    settings.setFavourite(card->presetName(), value);
    if (activeFilter == "Favoritas")
      updateFilter();
    if (notify)
      notify(value ? "Adicionado aos favoritos" : "Removido dos favoritos");
  };
  grid.addAndMakeVisible(card);
  activeFilter = "Todas";
  searchText.clear();
  for (int i = 0; i < chips.size(); ++i)
    chips[i]->setColour(juce::TextButton::buttonColourId,
                        i == 0 ? theme::blue : theme::panel);
  updateFilter();
  if (selectAfterAdding) {
    applyVoice(cards.indexOf(card));
    viewport.setViewPositionProportionately(0.0, 1.0);
  }
}
void VoicesPage::setSearchText(const juce::String &text) {
  searchText = text.trim();
  updateFilter();
}
bool VoicesPage::runSmokeTest() {
  const auto original = settings.isFavourite("Voz Limpa");
  if (!settings.setFavourite("Voz Limpa", !original) ||
      settings.isFavourite("Voz Limpa") == original)
    return false;
  if (!settings.setFavourite("Voz Limpa", original))
    return false;
  setSearchText(juce::String::fromUTF8("Robô"));
  setSearchText({});
  applyVoice(1);
  applyVoice(0);
  const juce::Point<int> sizes[]{{900, 570}, {1080, 590}, {1240, 770},
                                 {1720, 950}};
  for (const auto size : sizes) {
    setBounds(0, 0, size.x, size.y);
    resized();
    juce::Array<juce::Rectangle<int>> fixedHeaders;
    for (auto *button : accordions)
      fixedHeaders.add(button->getBounds());
    const auto fixedPanel = detailsArea;
    const auto fixedGrid = gridArea;
    const auto fixedSave = saveButton.getBounds();
    const auto fixedReset = resetButton.getBounds();
    for (int section = 0; section <= 6; ++section) {
      selectDetailsSection(static_cast<DetailsSection>(section));
      resized();
      if (detailsArea != fixedPanel || gridArea != fixedGrid ||
          saveButton.getBounds() != fixedSave ||
          resetButton.getBounds() != fixedReset)
        return false;
      for (int i = 0; i < accordions.size(); ++i)
        if (accordions[i]->getBounds() != fixedHeaders[i] ||
            (i > 0 && accordions[i]->getY() <= accordions[i - 1]->getBottom()))
          return false;
      if (sectionViewport.getBounds().isEmpty())
        return false;
    }
  }
  selectDetailsSection(DetailsSection::Voice);
  return selectedVoice().isNotEmpty() && grid.getWidth() > 0;
}
void VoicesPage::markDirty() {
  dirty = true;
  dirtyLabel.setVisible(true);
  saveButton.setEnabled(true);
}
void VoicesPage::syncControls() {
  auto &p = engine.parameters();
  const float values[]{p.noiseReduction.load(), p.outputGainDb.load(),
                       p.mix.load(), p.pitchSemitones.load(), p.formant.load()};
  for (int i = 0; i < sliders.size(); ++i)
    sliders[i]->setValue(values[i], juce::dontSendNotification);
  selectDetailsSection(selectedSection);
}
void VoicesPage::applyVoice(int index) {
  selectedIndex = index;
  for (int i = 0; i < cards.size(); ++i)
    cards[i]->setSelected(i == index);
  auto name = selectedVoice();
  juce::String factory = name;
  if (name == "Voz Limpa")
    factory = "Voz normal limpa";
  else if (name.contains("Grave"))
    factory = "Masculina grave";
  else if (name == "Aguda" || name == "Personagem Infantil")
    factory = "Personagem infantil";
  else if (name.contains(juce::String::fromUTF8("Robô")) ||
           name == juce::String::fromUTF8("Voz Metálica") ||
           name == "Som Digital")
    factory = juce::String::fromUTF8("Robô");
  else if (name == juce::String::fromUTF8("Rádio"))
    factory = juce::String::fromUTF8("Rádio policial");
  else if (name == juce::String::fromUTF8("Alienígena") ||
           name == "Eco Espacial")
    factory = juce::String::fromUTF8("Alienígena");
  presets.load(factory, engine.parameters());
  selectedName.setText(name, juce::dontSendNotification);
  selectedType.setText(selectedCategory(), juce::dontSendNotification);
  favouriteButton.setButtonText(cards[index]->isFavourite()
                                    ? juce::String::fromUTF8("♥")
                                    : juce::String::fromUTF8("♡"));
  syncControls();
  dirty = false;
  dirtyLabel.setVisible(false);
  saveButton.setEnabled(false);
  if (notify)
    notify("Voz aplicada: " + name);
  repaint();
}
void VoicesPage::updateFilter() {
  for (auto *c : cards) {
    const bool text = searchText.isEmpty() ||
                      c->presetName().containsIgnoreCase(searchText) ||
                      c->presetCategory().containsIgnoreCase(searchText);
    bool category = activeFilter == "Todas" || activeFilter == "Mais";
    if (activeFilter == "Favoritas")
      category = c->isFavourite();
    else if (!category)
      category = c->presetCategory().containsIgnoreCase(activeFilter);
    c->setVisible(text && category);
  }
  resized();
}
void VoicesPage::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  g.setColour(theme::border);
  g.drawLine((float)headerArea.getX(), (float)headerArea.getBottom(),
             (float)headerArea.getRight(), (float)headerArea.getBottom());
  theme::roundedPanel(g, detailsArea.toFloat(), 12, juce::Colour(0xff101827));
  g.setColour(theme::cyan.withAlpha(.15f));
  g.fillEllipse((float)detailsArea.getX() + 17, (float)detailsArea.getY() + 18,
                54, 54);
  g.setColour(theme::cyan);
  g.drawEllipse((float)detailsArea.getX() + 17, (float)detailsArea.getY() + 18,
                54, 54, 2);
  g.drawRoundedRectangle((float)detailsArea.getX() + 39,
                         (float)detailsArea.getY() + 31, 10, 23, 5, 2);
  g.setColour(theme::border);
  g.drawLine((float)detailsArea.getX() + 12, (float)detailsArea.getY() + 88,
             (float)detailsArea.getRight() - 12,
             (float)detailsArea.getY() + 88);
  g.setColour(theme::text);
  g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
  g.drawText(juce::String::fromUTF8("CONTROLES RÁPIDOS"),
             quickControlsHeaderArea, juce::Justification::centredLeft);
}
void VoicesPage::resized() {
  auto area = getLocalBounds().reduced(12);
  detailsArea = area.removeFromRight(getWidth() < 1180 ? 270 : 300);
  area.removeFromRight(12);
  headerArea = area.removeFromTop(74);
  title.setBounds(headerArea.removeFromTop(34));
  subtitle.setBounds(headerArea);
  createVoice.setBounds(area.getRight() - 210, getLocalBounds().getY() + 19,
                        205, 38);
  filterArea = area.removeFromTop(54);
  auto chipsArea = filterArea;
  for (auto *b : chips) {
    int w = juce::jlimit(66, 110, b->getButtonText().length() * 8 + 24);
    b->setBounds(chipsArea.removeFromLeft(w).reduced(3, 9));
  }
  advancedFilter.setBounds(chipsArea.removeFromLeft(42).reduced(3, 9));
  gridArea = area;
  viewport.setBounds(gridArea);
  int columns = juce::jlimit(3, 6, juce::jmax(3, gridArea.getWidth() / 175));
  int gap = 10,
      cardW = (gridArea.getWidth() - 18 - (columns - 1) * gap) / columns, x = 8,
      y = 8, col = 0;
  for (auto *c : cards) {
    if (!c->isVisible())
      continue;
    c->setBounds(x, y, cardW, 210);
    if (++col >= columns) {
      col = 0;
      x = 8;
      y += 220;
    } else
      x += cardW + gap;
  }
  grid.setSize(juce::jmax(420, gridArea.getWidth() - 14),
               juce::jmax(gridArea.getHeight(), y + 220));
  auto d = detailsArea.reduced(14);
  const bool compactDetails = detailsArea.getHeight() < 650;
  auto actions =
      detailsArea.reduced(12).removeFromBottom(compactDetails ? 58 : 72);
  d.setBottom(actions.getY() - 8);
  d.removeFromTop(6);
  auto selected = d.removeFromTop(compactDetails ? 54 : 68);
  selected.removeFromLeft(64);
  selectedCaption.setBounds(selected.removeFromTop(18));
  selectedName.setBounds(selected.removeFromTop(27));
  selectedType.setBounds(selected);
  favouriteButton.setBounds(detailsArea.getRight() - 74,
                            detailsArea.getY() + 25, 30, 32);
  menuButton.setBounds(detailsArea.getRight() - 42, detailsArea.getY() + 25, 30,
                       32);
  d.removeFromTop(compactDetails ? 8 : 12);
  quickControlsHeaderArea = d.removeFromTop(22);
  d.removeFromTop(4);
  for (int i = 0; i < sliders.size(); ++i) {
    auto row = d.removeFromTop(compactDetails ? 32 : 40);
    controlLabels[i]->setBounds(
        row.removeFromTop(16).removeFromLeft(row.getWidth() - 55));
    valueLabels[i]->setBounds(detailsArea.getRight() - 62, row.getY() - 19, 44,
                              19);
    sliders[i]->setBounds(row);
  }
  d.removeFromTop(4);
  for (auto *b : accordions)
    b->setBounds(d.removeFromTop(compactDetails ? 27 : 30).reduced(0, 1));
  d.removeFromTop(5);
  sectionViewport.setBounds(d);
  sectionContent.setSize(juce::jmax(180, d.getWidth() - 10), 120);
  sectionTitle.setBounds(10, 8, sectionContent.getWidth() - 20, 22);
  sectionBody.setBounds(10, 34, sectionContent.getWidth() - 20, 80);
  dirtyLabel.setBounds(actions.removeFromTop(compactDetails ? 14 : 20));
  saveButton.setBounds(
      actions.removeFromLeft(actions.getWidth() / 2).reduced(2));
  resetButton.setBounds(actions.reduced(2));
}
} // namespace vox
