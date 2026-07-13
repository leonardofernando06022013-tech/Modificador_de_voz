#include "MainComponent.h"
#include "Theme.h"
namespace vox {
static void styleLabel(juce::Label &l, float size, juce::Colour colour,
                       bool bold = false) {
  l.setColour(juce::Label::textColourId, colour);
  l.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}
MainComponent::MainComponent(AudioEngine &e, SettingsManager &s)
    : engine(e), settings(s),
      devices(e.deviceManager(), 0, 1, 0, 2, true, true, true, false) {
  setLookAndFeel(&look);
  languageListener = LocalizationManager::instance().addListener([this] { updateTexts(); });
  setSize(1450, 900);
  addAndMakeVisible(sidebar);
  router.setListener(
      [this](PageId, PageId page) { showPage(static_cast<int>(page)); });
  sidebar.onSelect = [this](int p) {
    if (p <= 11)
      router.navigateTo(static_cast<PageId>(p));
    else
      notification.showMessage(
          p == 12 ? "Ajuda: passe o mouse sobre os controles para ver dicas."
                  : "Modificador de Voz v1.0.0");
  };
  sidebar.setSelected(1);
  title.setText("Modificador de Voz", juce::dontSendNotification);
  styleLabel(title, 21, theme::text, true);
  version.setText("v1.0.0", juce::dontSendNotification);
  styleLabel(version, 12, theme::muted);
  pageTitle.setText("Biblioteca de vozes", juce::dontSendNotification);
  styleLabel(pageTitle, 23, theme::text, true);
  addAndMakeVisible(title);
  addAndMakeVisible(version);
  addAndMakeVisible(pageTitle);
  search.setTextToShowWhenEmpty("Pesquisar vozes e efeitos...", theme::muted);
  search.setTooltip("Pesquisar em tempo real (Ctrl+K)");
  search.onTextChange = [this] {
    filterCards();
    if (voicesPage)
      voicesPage->setSearchText(search.getText());
  };
  addAndMakeVisible(search);
  clearSearch.setButtonText(juce::String::fromUTF8("×"));
  clearSearch.setTooltip("Limpar pesquisa");
  clearSearch.onClick = [this] { search.clear(); };
  addAndMakeVisible(clearSearch);
  searchShortcut.setText("Ctrl K", juce::dontSendNotification);
  searchShortcut.setJustificationType(juce::Justification::centred);
  styleLabel(searchShortcut, 10, theme::muted, true);
  addAndMakeVisible(searchShortcut);
  deviceStatus.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::transparentBlack);
  deviceStatus.onClick = [this] { router.navigateTo(PageId::Devices); };
  addAndMakeVisible(deviceStatus);
  notifications.setTooltip(juce::String::fromUTF8("Notificações"));
  helpButton.setTooltip("Ajuda");
  topSettings.setTooltip(juce::String::fromUTF8("Configurações"));
  profile.setTooltip("Perfil local");
  notifications.setColour(juce::TextButton::textColourOffId, theme::red);
  profile.setColour(juce::TextButton::textColourOffId, theme::cyan);
  helpButton.onClick = [this] {
    notification.showMessage("Use os tooltips para conhecer cada controle.");
  };
  topSettings.onClick = [this] { router.navigateTo(PageId::Settings); };
  profile.onClick = [this] { notification.showMessage("Perfil local"); };
  for (auto *b : {&notifications, &helpButton, &topSettings, &profile}) {
    b->setColour(juce::TextButton::buttonColourId,
                 juce::Colours::transparentBlack);
    addAndMakeVisible(b);
  }
  const juce::StringArray tabNames{"Vozes", "Efeitos", "Painel de som",
                                   "Favoritos"};
  for (int i = 0; i < tabNames.size(); ++i) {
    auto *b = tabs.add(new juce::TextButton(tabNames[i]));
    b->setColour(juce::TextButton::buttonColourId,
                 i == 0 ? theme::blue : juce::Colours::transparentBlack);
    b->onClick = [this, i] {
      const PageId pages[]{PageId::Voices, PageId::Effects, PageId::Soundboard,
                           PageId::Favorites};
      router.navigateTo(pages[i]);
    };
    addAndMakeVisible(b);
  }
  bannerTitle.setText("Transforme sua voz em tempo real",
                      juce::dontSendNotification);
  styleLabel(bannerTitle, 24, theme::text, true);
  bannerText.setText(
      juce::String("Escolha uma voz, ajuste os efeitos e use em\nDiscord, ") +
          juce::String::fromUTF8("FiveM, jogos, chamadas e transmissões."),
      juce::dontSendNotification);
  styleLabel(bannerText, 13, theme::text);
  configure.setButtonText("Configurar dispositivos");
  configure.setColour(juce::TextButton::buttonColourId, theme::blue);
  configure.onClick = [this] { router.navigateTo(PageId::Devices); };
  addAndMakeVisible(bannerTitle);
  addAndMakeVisible(bannerText);
  addAndMakeVisible(configure);
  bannerTitle.setVisible(false);
  bannerText.setVisible(false);
  configure.setVisible(false);
  const juce::StringArray chipNames{
      "Todos",  "Naturais",  "Graves", "Agudas",   "Robos",
      "Terror", "Criativas", "Radio",  "Telefone", "Jogos"};
  for (int i = 0; i < chipNames.size(); ++i) {
    auto *b = chips.add(new juce::TextButton(chipNames[i]));
    b->setColour(juce::TextButton::buttonColourId,
                 i == 0 ? theme::purple : theme::panel);
    b->onClick = [this, b] {
      categoryFilter = b->getButtonText();
      for (auto *c : chips)
        c->setColour(juce::TextButton::buttonColourId,
                     c == b ? theme::purple : theme::panel);
      filterCards();
    };
    addAndMakeVisible(b);
  }
  viewport.setViewedComponent(&cardCanvas, false);
  viewport.setScrollBarsShown(true, false);
  addAndMakeVisible(viewport);
  const juce::StringArray names{
      "Voz Limpa",   "Grave Profunda", "Grave Suave",
      "Aguda",       "Feminina Suave", "Robo",
      "Robo Pesado", "Radio",          "Telefone",
      "Megafone",    "Demonio",        "Monstro",
      "Alienigena",  "Voz Metalica",   "Personagem Infantil",
      "Narrador",    "Eco Espacial",   "Som Digital"};
  const juce::StringArray cats{
      "Naturais",  "Graves", "Graves",   "Agudas",    "Naturais",  "Robos",
      "Robos",     "Radio",  "Telefone", "Criativas", "Terror",    "Terror",
      "Criativas", "Robos",  "Agudas",   "Naturais",  "Criativas", "Jogos"};
  for (int i = 0; i < names.size(); ++i) {
    auto *c = cards.add(new VoiceCardComponent(names[i], cats[i], i));
    c->onClick = [this, i] { applyPreset(i); };
    cardCanvas.addAndMakeVisible(c);
  }
  cards[0]->setSelected(true);
  const juce::StringArray effectNames{
      "Reverb",    "Delay",       "Chorus",          "Flanger",
      "Distorcao", "Bit Crusher", "Ring Modulation", "Radio",
      "Telefone",  "Megafone",    "Eco Espacial",    "Som Metalico"};
  const juce::StringArray effectCats{"Espacial",  "Espacial",  "Modulacao",
                                     "Modulacao", "Distorcao", "Criativo",
                                     "Modulacao", "Retro",     "Retro",
                                     "Criativo",  "Espacial",  "Criativo"};
  for (int i = 0; i < effectNames.size(); ++i) {
    auto *c = effectCards.add(
        new VoiceCardComponent(effectNames[i], effectCats[i], i + 2));
    c->setVisible(false);
    c->onClick = [this, i, c] {
      for (auto *e : effectCards)
        e->setSelected(e == c);
      auto &p = engine.parameters();
      if (i == 0)
        p.reverb = juce::jmax(.5f, p.reverb.load());
      else if (i == 1)
        p.delay = juce::jmax(.5f, p.delay.load());
      else if (i == 2)
        p.chorus = juce::jmax(.5f, p.chorus.load());
      else if (i == 3)
        p.flanger = juce::jmax(.5f, p.flanger.load());
      else if (i == 4)
        p.distortion = juce::jmax(.5f, p.distortion.load());
      else if (i == 5)
        p.bitCrush = juce::jmax(.5f, p.bitCrush.load());
      else if (i == 6)
        p.ringMod = juce::jmax(.5f, p.ringMod.load());
      else
        presets.applyFactory(c->presetName(), p);
      selectedName.setText(c->presetName(), juce::dontSendNotification);
      selectedCategory.setText("Efeito ativo - " + c->presetCategory(),
                               juce::dontSendNotification);
      notification.showMessage("Efeito ativado: " + c->presetName());
    };
    cardCanvas.addAndMakeVisible(c);
  }
  addAndMakeVisible(details);
  selectedName.setText(names[0], juce::dontSendNotification);
  selectedCategory.setText("Natural - Preset ativo",
                           juce::dontSendNotification);
  styleLabel(selectedName, 20, theme::text, true);
  styleLabel(selectedCategory, 12, theme::muted);
  details.addAndMakeVisible(selectedName);
  details.addAndMakeVisible(selectedCategory);
  auto &p = e.parameters();
  addParameter("Intensidade / mistura", 0, 1, .01, p.mix);
  addParameter("Pitch", -12, 12, .1, p.pitchSemitones);
  addParameter("Formante", -1, 1, .01, p.formant);
  addParameter("Ganho de saida", -24, 24, .1, p.outputGainDb);
  addParameter("Reverb", 0, 1, .01, p.reverb);
  addParameter("Distorcao", 0, 1, .01, p.distortion);
  savePreset.setColour(juce::TextButton::buttonColourId, theme::blue);
  savePreset.onClick = [this] {
    presets.save(selectedName.getText(), engine.parameters());
    notification.showMessage("Configuracoes salvas.");
  };
  duplicatePreset.onClick = [this] {
    presets.save(selectedName.getText() + " - copia", engine.parameters());
    notification.showMessage("Preset duplicado.");
  };
  resetPreset.onClick = [this] {
    applyPreset(selectedCard);
    notification.showMessage("Preset restaurado.");
  };
  for (auto *b : {&savePreset, &duplicatePreset, &resetPreset})
    details.addAndMakeVisible(b);
  voicesPage = std::make_unique<VoicesPage>(engine, settings, presets);
  voicesPage->notify = [this](const juce::String &message) {
    notification.showMessage(message);
  };
  addAndMakeVisible(*voicesPage);
  adminPage = std::make_unique<AdminPage>(engine, settings, presets);
  adminPage->navigate = [this](PageId page) { router.navigateTo(page); };
  adminPage->notify = [this](const juce::String &message) {
    notification.showMessage(message);
  };
  addAndMakeVisible(*adminPage);
  adminPage->setVisible(false);
  sidebar.setAdminVisible(adminPage->hasAccess());
  integrationsPage=std::make_unique<IntegrationsPage>(engine);integrationsPage->notify=[this](const juce::String&message){notification.showMessage(message);};addAndMakeVisible(*integrationsPage);integrationsPage->setVisible(false);
  addAndMakeVisible(devices);
  devices.setVisible(false);
  settingsPage = std::make_unique<SettingsPage>(engine, settings);
  settingsPage->onOpenDevices = [this] { router.navigateTo(PageId::Devices); };
  addAndMakeVisible(*settingsPage);
  settingsPage->setVisible(false);
  homePage =
      std::make_unique<ModulePage>(ModulePage::Kind::Home, engine, presets);
  soundboardPage = std::make_unique<ModulePage>(ModulePage::Kind::Soundboard,
                                                engine, presets);
  favoritesPage = std::make_unique<ModulePage>(ModulePage::Kind::Favorites,
                                               engine, presets);
  equalizerPage = std::make_unique<ModulePage>(ModulePage::Kind::Equalizer,
                                               engine, presets);
  presetsModulePage =
      std::make_unique<ModulePage>(ModulePage::Kind::Presets, engine, presets);
  for (auto *module :
       {homePage.get(), soundboardPage.get(), favoritesPage.get(),
        equalizerPage.get(), presetsModulePage.get()}) {
    module->navigate = [this](PageId page) { router.navigateTo(page); };
    addAndMakeVisible(module);
    module->setVisible(false);
  }
  diagnosticText.setMultiLine(true);
  diagnosticText.setReadOnly(true);
  diagnosticText.setColour(juce::TextEditor::backgroundColourId, theme::panel);
  addAndMakeVisible(diagnosticText);
  diagnosticText.setVisible(false);
  copyDiagnostic.onClick = [this] {
    juce::SystemClipboard::copyTextToClipboard(diagnosticText.getText());
    notification.showMessage("Relatorio copiado.");
  };
  addAndMakeVisible(copyDiagnostic);
  copyDiagnostic.setVisible(false);
  power.setComponentID("powerButton");
  power.setColour(juce::TextButton::buttonColourId, theme::blue);
  power.onClick = [this] {
    engine.isRunning() ? engine.stop() : engine.start();
    notification.showMessage(engine.isRunning()
                                 ? "Modificador de voz ativado."
                                 : "Modificador de voz desativado.");
  };
  mute.onClick = [&p, this] {
    p.muted = !p.muted.load();
    notification.showMessage(p.muted.load() ? "Microfone silenciado."
                                            : "Microfone reativado.");
  };
  bypass.onClick = [&p] { p.bypass = !p.bypass.load(); };
  monitor.setTooltip("O monitor depende do dispositivo de saida selecionado");
  monitor.onClick = [this] {
    const bool enable=!engine.isMonitoring();
    const auto error=engine.setMonitoring(enable);
    if(error.isNotEmpty())notification.showMessage("Falha no monitoramento: "+error,true);
    else notification.showMessage(enable?"Monitoramento ativo em: "+engine.monitorDeviceName():"Monitoramento desativado.");
  };
  for (auto *b : {&power, &monitor, &mute, &bypass})
    addAndMakeVisible(b);
  const PageId shortcutPages[]{PageId::Devices, PageId::Effects,
                               PageId::Equalizer, PageId::Settings};
  juce::TextButton *shortcutButtons[]{&shortcutDevices, &shortcutEffects,
                                      &shortcutEqualizer, &shortcutSettings};
  for (int i = 0; i < 4; ++i) {
    auto *b = shortcutButtons[i];
    b->setColour(juce::TextButton::buttonColourId,
                 juce::Colours::transparentBlack);
    b->onClick = [this, page = shortcutPages[i]] { router.navigateTo(page); };
    addAndMakeVisible(b);
  }
  addAndMakeVisible(meter);
  styleLabel(status, 12, theme::green, true);
  styleLabel(cpuLatency, 11, theme::muted);
  styleLabel(deviceName, 10, theme::muted);
  styleLabel(bottomVoiceName, 13, theme::text, true);
  styleLabel(bottomVoiceCategory, 10, theme::muted);
  bottomVoiceName.setText("Voz Limpa", juce::dontSendNotification);
  bottomVoiceCategory.setText("Natural", juce::dontSendNotification);
  for (auto *l : {&status, &cpuLatency, &deviceName, &bottomVoiceName,
                  &bottomVoiceCategory})
    addAndMakeVisible(l);
  addChildComponent(notification);
  showPage(1);
  startTimerHz(30);
  updateTexts();
}
MainComponent::~MainComponent() {
  stopTimer();
  LocalizationManager::instance().removeListener(languageListener);
  settings.save(engine.parameters());
  setLookAndFeel(nullptr);
}
void MainComponent::updateTexts() {
  auto &l = LocalizationManager::instance();
  title.setText(l.text("app.name"), juce::dontSendNotification);
  search.setTextToShowWhenEmpty(l.text("app.search"), theme::muted);
  search.setTooltip(l.text("app.search.tooltip"));
  clearSearch.setTooltip(l.text("app.search.clear"));
  configure.setButtonText(l.text("action.configure_devices"));
  savePreset.setButtonText(l.text("action.save"));
  duplicatePreset.setButtonText(l.text("action.duplicate"));
  resetPreset.setButtonText(l.text("action.restore"));
  power.setButtonText(l.text(engine.isRunning() ? "action.turn_off" : "action.turn_on"));
  monitor.setButtonText(l.text("action.monitor"));
  mute.setButtonText(l.text(engine.parameters().muted.load() ? "action.unmute" : "action.mute"));
  bypass.setButtonText(l.text("action.bypass"));
  shortcutDevices.setButtonText(l.text("nav.devices"));
  shortcutEffects.setButtonText(l.text("nav.effects"));
  shortcutEqualizer.setButtonText(l.text("nav.equalizer"));
  shortcutSettings.setButtonText(l.text("nav.settings_short"));
  const juce::StringArray tabKeys{"nav.voices", "nav.effects", "nav.soundboard", "nav.favorites"};
  for (int i = 0; i < tabs.size(); ++i) tabs[i]->setButtonText(l.text(tabKeys[i]));
  const juce::StringArray pageKeys{"page.home", "page.voices", "page.effects", "page.soundboard", "page.favorites", "page.equalizer", "page.devices", "page.integrations", "page.presets", "page.diagnostics", "settings.title", "page.admin"};
  sidebar.updateTexts();
  if (currentPage >= 0 && currentPage < pageKeys.size())
    pageTitle.setText(l.text(pageKeys[currentPage]), juce::dontSendNotification);
  resized();
  repaint();
}
bool MainComponent::runNavigationSmokeTest() {
  if (!voicesPage || !voicesPage->runSmokeTest())
    return false;
  if (!settingsPage || !settingsPage->runLanguageSmokeTest())
    return false;
  if (!integrationsPage || !integrationsPage->runSmokeTest())
    return false;
  if (!adminPage || !adminPage->runSmokeTest())
    return false;
  const PageId pages[]{
      PageId::Home,       PageId::Voices,    PageId::Effects,
      PageId::Soundboard, PageId::Favorites, PageId::Equalizer,
      PageId::Devices,    PageId::Integrations, PageId::Presets,   PageId::Diagnostics,
      PageId::Settings,   PageId::Admin};
  for (auto page : pages) {
    router.navigateTo(page);
    if (router.currentPage() != page)
      return false;
  }
  router.navigateTo(PageId::Voices);
  return true;
}
void MainComponent::addParameter(const juce::String &name, double lo, double hi,
                                 double step, std::atomic<float> &value) {
  auto *l = parameterLabels.add(new juce::Label({}, name));
  styleLabel(*l, 13, theme::muted);
  auto *s = parameterSliders.add(new juce::Slider(
      juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight));
  s->setRange(lo, hi, step);
  s->setValue(value.load(), juce::dontSendNotification);
  s->setTooltip(name);
  s->onValueChange = [s, &value] { value.store((float)s->getValue()); };
  details.addAndMakeVisible(l);
  details.addAndMakeVisible(s);
}
void MainComponent::showPage(int p) {
  currentPage = p;
  sidebar.setSelected(p);
  const bool effectsLibrary = p == 2;
  voicesPage->setVisible(p == 1);
  devices.setVisible(p == 6);
  integrationsPage->setVisible(p == 7);
  settingsPage->setVisible(p == 10);
  adminPage->setVisible(p == 11);
  homePage->setVisible(p == 0);
  soundboardPage->setVisible(p == 3);
  favoritesPage->setVisible(p == 4);
  equalizerPage->setVisible(p == 5);
  presetsModulePage->setVisible(p == 8);
  diagnosticText.setVisible(p == 9);
  copyDiagnostic.setVisible(p == 9);
  viewport.setVisible(effectsLibrary);
  details.setVisible(effectsLibrary);
  for (auto *c : cards)
    c->setVisible(false);
  for (auto *c : effectCards)
    c->setVisible(effectsLibrary);
  bannerTitle.setVisible(false);
  bannerText.setVisible(false);
  configure.setVisible(false);
  for (auto *b : tabs)
    b->setVisible(effectsLibrary);
  int activeTab = p == 2 ? 1 : 0;
  for (int i = 0; i < tabs.size(); ++i)
    tabs[i]->setColour(juce::TextButton::buttonColourId,
                       i == activeTab ? theme::blue
                                      : juce::Colours::transparentBlack);
  for (auto *b : chips)
    b->setVisible(effectsLibrary);
  pageTitle.setText(PageRouter::title(static_cast<PageId>(p)),
                    juce::dontSendNotification);
  pageTitle.setVisible(p == 6 || p == 9);
  categoryFilter = "Todos";
  filterCards();
  resized();
  repaint();
}
void MainComponent::applyPreset(int i) {
  selectedCard = i;
  for (int n = 0; n < cards.size(); ++n)
    cards[n]->setSelected(n == i);
  const auto name = cards[i]->presetName();
  juce::String factory = name;
  if (name == "Voz Limpa")
    factory = "Voz normal limpa";
  else if (name == "Grave Profunda" || name == "Grave Suave")
    factory = "Masculina grave";
  else if (name == "Aguda")
    factory = "Personagem infantil";
  else if (name == "Robo Pesado" || name == "Voz Metalica" ||
           name == "Som Digital")
    factory = juce::String::fromUTF8("Robô");
  else if (name == "Radio")
    factory = juce::String::fromUTF8("Rádio policial");
  else if (name == "Eco Espacial")
    factory = juce::String::fromUTF8("Alienígena");
  presets.load(factory, engine.parameters());
  selectedName.setText(name, juce::dontSendNotification);
  selectedCategory.setText("Preset aplicado ao processamento",
                           juce::dontSendNotification);
  auto &p = engine.parameters();
  const float values[]{p.mix.load(),     p.pitchSemitones.load(),
                       p.formant.load(), p.outputGainDb.load(),
                       p.reverb.load(),  p.distortion.load()};
  for (int n = 0; n < parameterSliders.size(); ++n)
    parameterSliders[n]->setValue(values[n], juce::dontSendNotification);
  notification.showMessage("Preset carregado: " + name);
}
void MainComponent::filterCards() {
  const auto q = search.getText().trim();
  int visible = 0;
  auto filter = [&](VoiceCardComponent *c, bool page) {
    const bool textOk = q.isEmpty() || c->presetName().containsIgnoreCase(q);
    const bool catOk = categoryFilter == "Todos" ||
                       c->presetCategory().containsIgnoreCase(categoryFilter);
    c->setVisible(page && textOk && catOk);
    if (c->isVisible())
      ++visible;
  };
  for (auto *c : cards)
    filter(c, currentPage != 2);
  for (auto *c : effectCards)
    filter(c, currentPage == 2);
  cardCanvas.setSize(
      juce::jmax(400, viewport.getWidth() - 12),
      juce::jmax(viewport.getHeight(), ((visible + 4) / 5) * 170));
  resized();
}
void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  g.setColour(theme::secondary);
  g.fillRect(topBounds);
  g.setColour(theme::border);
  g.drawLine(0, (float)topBounds.getBottom(), (float)getWidth(),
             (float)topBounds.getBottom());
  juce::ColourGradient logoGrad(theme::cyan, {20.0f, 22.0f}, theme::purple,
                                {52.0f, 52.0f}, false);
  g.setGradientFill(logoGrad);
  for (int i = 0; i < 7; ++i) {
    const float h = 12.0f + std::abs(3 - i) * -1.8f + ((i % 2) * 12.0f);
    g.fillRoundedRectangle(22.0f + i * 5.0f, 36.0f - h * .5f, 3.0f, h, 1.5f);
  }
  if (details.isVisible()) {
    theme::roundedPanel(g, rightBounds.toFloat().reduced(4), 12, theme::panel);
    g.setColour(theme::border);
    g.drawLine((float)rightBounds.getX(), (float)rightBounds.getY() + 64,
               (float)rightBounds.getRight(), (float)rightBounds.getY() + 64);
  }
  if (!bannerBounds.isEmpty()) {
    auto r = bannerBounds.toFloat();
    juce::ColourGradient bg(juce::Colour(0xff171755), r.getTopLeft(),
                            juce::Colour(0xff121044), r.getBottomRight(),
                            false);
    bg.addColour(.5, juce::Colour(0xff17206a));
    g.setGradientFill(bg);
    g.fillRoundedRectangle(r, 12);
    g.setColour(theme::blue.withAlpha(.15f));
    for (int x = (int)r.getX(); x < r.getRight(); x += 28)
      g.drawVerticalLine(x, r.getY(), r.getBottom());
    juce::Path wave;
    for (int x = 0; x < (int)r.getWidth(); ++x) {
      const float px = r.getX() + x, py = r.getCentreY() +
                                          std::sin(x * .055f) * 14.0f +
                                          std::sin(x * .17f) * 4.0f;
      if (x == 0)
        wave.startNewSubPath(px, py);
      else
        wave.lineTo(px, py);
    }
    g.setColour(theme::blue.withAlpha(.35f));
    g.strokePath(wave, juce::PathStrokeType(6));
    g.setColour(theme::cyan.withAlpha(.75f));
    g.strokePath(wave, juce::PathStrokeType(1.5f));
    auto mic =
        juce::Rectangle<float>(r.getRight() - 120, r.getY() + 18, 58, 94);
    g.setColour(juce::Colour(0xff0a0d22));
    g.fillRoundedRectangle(mic.withTrimmedBottom(28), 24);
    g.setColour(theme::text);
    g.drawRoundedRectangle(mic.withTrimmedBottom(28), 24, 3);
    for (int y = 12; y < 55; y += 9)
      g.drawHorizontalLine((int)mic.getY() + y, mic.getX() + 10,
                           mic.getRight() - 10);
    g.setColour(theme::purple);
    juce::Path micArc;
    micArc.addArc(mic.getX() - 8, mic.getY() + 27, mic.getWidth() + 16, 54,
                  0.0f, juce::MathConstants<float>::pi, true);
    g.strokePath(micArc, juce::PathStrokeType(3));
    g.drawLine(mic.getCentreX(), mic.getBottom() - 27, mic.getCentreX(),
               mic.getBottom() - 8, 4);
    g.drawLine(mic.getCentreX() - 17, mic.getBottom() - 8,
               mic.getCentreX() + 17, mic.getBottom() - 8, 4);
    auto pills = r.withTrimmedLeft(32).withTrimmedTop(101).withWidth(300);
    const juce::StringArray values{juce::String::fromUTF8("Latência: 8 ms"),
                                   "CPU: 6%", "48.000 Hz", "Buffer: 256"};
    for (auto value : values) {
      auto p = pills.removeFromLeft(70).reduced(2);
      g.setColour(theme::elevated.withAlpha(.9f));
      g.fillRoundedRectangle(p, 7);
      g.setColour(theme::muted);
      g.setFont(9);
      g.drawText(value, p, juce::Justification::centred);
    }
  }
  g.setColour(theme::secondary);
  g.fillRect(bottomBounds);
  g.setColour(theme::border);
  g.drawLine(0, (float)bottomBounds.getY(), (float)getWidth(),
             (float)bottomBounds.getY());
  auto footer = bottomBounds;
  auto voiceCard = footer.removeFromLeft(285).toFloat().reduced(10, 8);
  g.setColour(theme::panel);
  g.fillRoundedRectangle(voiceCard, 10);
  g.setColour(theme::purple);
  g.drawRoundedRectangle(voiceCard, 10, 1.4f);
  g.setColour(theme::cyan.withAlpha(.16f));
  g.fillEllipse(voiceCard.getX() + 10, voiceCard.getCentreY() - 24, 48, 48);
  g.setColour(theme::cyan);
  g.drawEllipse(voiceCard.getX() + 10, voiceCard.getCentreY() - 24, 48, 48,
                1.5f);
  auto mx = voiceCard.getX() + 34, my = voiceCard.getCentreY();
  g.drawRoundedRectangle(mx - 5, my - 14, 10, 22, 5, 1.5f);
  g.drawLine(mx, my + 8, mx, my + 15, 1.5f);
  g.drawLine(mx - 7, my + 15, mx + 7, my + 15, 1.5f);
  g.setColour(theme::yellow);
  g.drawText(juce::CharPointer_UTF8("\xE2\x98\x85"),
             voiceCard.removeFromRight(28), juce::Justification::centred);
  auto powerArea = footer.removeFromLeft(88).toFloat();
  g.setColour(theme::purple.withAlpha(.18f));
  g.fillEllipse(powerArea.withSizeKeepingCentre(70, 70));
  g.setColour(theme::purple);
  g.drawEllipse(powerArea.withSizeKeepingCentre(64, 64), 2);
  g.setColour(theme::cyan);
  g.drawEllipse(powerArea.withSizeKeepingCentre(56, 56), 1);
  g.setColour(theme::text);
  auto pc = powerArea.getCentre();
  juce::Path powerArc;
  powerArc.addCentredArc(pc.x, pc.y + 2, 14, 14, 0.0f, .72f,
                         juce::MathConstants<float>::twoPi - .72f, true);
  g.strokePath(powerArc, juce::PathStrokeType(2.5f));
  g.drawLine(pc.x, pc.y - 17, pc.x, pc.y + 1, 2.5f);
}
void MainComponent::resized() {
  auto all = getLocalBounds();
  topBounds = all.removeFromTop(72);
  bottomBounds = all.removeFromBottom(88);
  const bool compactNavigation = getWidth() < 1200;
  sidebar.setCompact(compactNavigation);
  auto sideArea = all.removeFromLeft(compactNavigation ? 72 : 170);
  sidebar.setBounds(sideArea);
  title.setBounds(64, 12, 230, 28);
  version.setBounds(66, 39, 80, 18);
  const int searchWidth = juce::jlimit(240, 430, getWidth() - 850);
  search.setBounds(330, 16, searchWidth, 38);
  clearSearch.setBounds(search.getRight() - 34, 21, 28, 27);
  searchShortcut.setBounds(search.getRight() - 92, 23, 52, 23);
  auto topRight = getWidth() - 14;
  profile.setBounds(topRight - 40, 16, 38, 38);
  topRight -= 44;
  topSettings.setBounds(topRight - 40, 16, 38, 38);
  topRight -= 44;
  helpButton.setBounds(topRight - 40, 16, 38, 38);
  topRight -= 44;
  notifications.setBounds(topRight - 40, 16, 38, 38);
  topRight -= 48;
  deviceStatus.setBounds(topRight - 270, 12, 270, 46);
  pageTitle.setBounds(all.getX() + 18, 82, 330, 34);
  contentBounds = all.reduced(14);
  if (currentPage != 1)
    contentBounds.removeFromTop(44);
  rightBounds = contentBounds.removeFromRight(getWidth() < 1220 ? 0 : 305);
  if (rightBounds.isEmpty())
    details.setVisible(false);
  else if (currentPage == 2)
    details.setVisible(true);
  auto work = contentBounds;
  auto tabArea = work.removeFromTop(42);
  for (auto *b : tabs)
    b->setBounds(tabArea.removeFromLeft(122).reduced(3));
  bannerBounds = {};
  bannerTitle.setBounds({});
  bannerText.setBounds({});
  configure.setBounds({});
  if (false) {
    bannerBounds = work.removeFromTop(138).reduced(3, 4);
    bannerTitle.setBounds(bannerBounds.getX() + 32, bannerBounds.getY() + 18,
                          430, 32);
    bannerText.setBounds(bannerBounds.getX() + 32, bannerBounds.getY() + 48,
                         430, 42);
    configure.setBounds(bannerBounds.getX() + 32, bannerBounds.getY() + 92, 165,
                        30);
  }
  chipBounds = work.removeFromTop(52);
  for (auto *b : chips) {
    const int w = juce::jmax(64, b->getButtonText().length() * 8 + 22);
    b->setBounds(chipBounds.removeFromLeft(w).reduced(3, 8));
  }
  viewport.setBounds(work.reduced(2));
  int columns = juce::jlimit(3, 7, juce::jmax(3, viewport.getWidth() / 145));
  int cardW = juce::jmax(120, (viewport.getWidth() - 18 - (columns - 1) * 9) /
                                  columns),
      x = 8, y = 8, col = 0;
  auto place = [&](VoiceCardComponent *c) {
    if (!c->isVisible())
      return;
    c->setBounds(x, y, cardW, 158);
    if (++col >= columns) {
      col = 0;
      x = 8;
      y += 167;
    } else
      x += cardW + 9;
  };
  for (auto *c : cards)
    place(c);
  for (auto *c : effectCards)
    place(c);
  cardCanvas.setSize(juce::jmax(400, viewport.getWidth() - 15),
                     juce::jmax(viewport.getHeight(), y + 168));
  details.setBounds(rightBounds);
  auto d = details.getLocalBounds().reduced(16);
  selectedName.setBounds(d.removeFromTop(32));
  selectedCategory.setBounds(d.removeFromTop(22));
  d.removeFromTop(12);
  for (int i = 0; i < parameterSliders.size(); ++i) {
    parameterLabels[i]->setBounds(d.removeFromTop(20));
    parameterSliders[i]->setBounds(d.removeFromTop(38));
    d.removeFromTop(2);
  }
  savePreset.setBounds(d.removeFromTop(38));
  d.removeFromTop(6);
  auto actions = d.removeFromTop(38);
  duplicatePreset.setBounds(
      actions.removeFromLeft(actions.getWidth() / 2).reduced(2));
  resetPreset.setBounds(actions.reduced(2));
  auto fullContent = contentBounds.getUnion(rightBounds);
  if (voicesPage)
    voicesPage->setBounds(fullContent.reduced(2));
  if (adminPage)
    adminPage->setBounds(fullContent.reduced(2));
  if (integrationsPage)
    integrationsPage->setBounds(fullContent.reduced(2));
  devices.setBounds(fullContent.reduced(8));
  if (settingsPage)
    settingsPage->setBounds(fullContent.reduced(4));
  for (auto *p : {homePage.get(), soundboardPage.get(), favoritesPage.get(),
                  equalizerPage.get(), presetsModulePage.get()})
    if (p)
      p->setBounds(fullContent.reduced(4));
  diagnosticText.setBounds(fullContent.reduced(8).withTrimmedBottom(46));
  copyDiagnostic.setBounds(
      fullContent.removeFromBottom(40).removeFromRight(190).reduced(3));
  auto btm = bottomBounds.reduced(10, 8);
  const bool compactBottom = getWidth() < 1250;
  auto voice = btm.removeFromLeft(compactBottom ? 235 : 285);
  voice.removeFromLeft(62);
  bottomVoiceName.setBounds(voice.removeFromTop(22));
  bottomVoiceCategory.setBounds(voice.removeFromTop(17));
  deviceName.setBounds(voice);
  power.setBounds(btm.removeFromLeft(88).reduced(9));
  monitor.setBounds(btm.removeFromLeft(compactBottom ? 94 : 104).reduced(3, 5));
  mute.setBounds(btm.removeFromLeft(compactBottom ? 88 : 98).reduced(3, 5));
  bypass.setBounds(btm.removeFromLeft(compactBottom ? 70 : 78).reduced(3, 5));
  auto shortcuts = btm.removeFromRight(compactBottom ? 220 : 300);
  auto stats = btm.removeFromRight(96);
  cpuLatency.setBounds(stats.reduced(5, 10));
  auto meterArea = btm.reduced(10, 9);
  status.setBounds(meterArea.removeFromTop(18));
  meter.setBounds(meterArea.reduced(0, 5));
  for (auto *b : {&shortcutDevices, &shortcutEffects, &shortcutEqualizer,
                  &shortcutSettings})
    b->setBounds(shortcuts.removeFromLeft(compactBottom ? 55 : 75).reduced(2, 5));
  notification.setBounds(getWidth() - 385, 82, 350, 54);
}
void MainComponent::timerCallback() {
  const auto in = engine.processor().inputPeak(),
             out = engine.processor().outputPeak();
  meter.setLevels(in, out);
  auto *d = engine.deviceManager().getCurrentAudioDevice();
  deviceName.setText(d ? "Entrada: " + d->getName().substring(0, 25)
                       : "Entrada indisponivel",
                     juce::dontSendNotification);
  bottomVoiceName.setText(voicesPage ? voicesPage->selectedVoice()
                                     : selectedName.getText(),
                          juce::dontSendNotification);
  bottomVoiceCategory.setText(voicesPage ? voicesPage->selectedCategory()
                                         : "Natural",
                              juce::dontSendNotification);
  deviceStatus.setButtonText(
      d ? (VirtualDeviceDetector::isVirtual(engine.deviceManager().getAudioDeviceSetup().outputDeviceName)&&integrationsPage
               ? juce::String::fromUTF8("●  ")+integrationsPage->routingStatusText().substring(0,42)
               : juce::String::fromUTF8("●  Microfone Ativo  ·  ") + d->getName().substring(0, 24))
        : juce::String::fromUTF8("●  Sem dispositivo de áudio"));
  deviceStatus.setTooltip(d ? d->getName() : "Nenhum dispositivo aberto");
  deviceStatus.setColour(juce::TextButton::buttonColourId,
                         juce::Colours::transparentBlack);
  deviceStatus.setColour(juce::TextButton::textColourOffId,
                         d ? theme::green : theme::red);
  power.setButtonText({});
  power.setTooltip(engine.isRunning() ? "Desligar modificador"
                                      : "Ligar modificador");
  auto &localization = LocalizationManager::instance();
  monitor.setButtonText(engine.isMonitoring() ? "Parar escuta" : localization.text("action.monitor"));
  monitor.setColour(juce::TextButton::buttonColourId,
                    engine.isMonitoring() ? theme::blue : theme::elevated);
  mute.setButtonText(localization.text(engine.parameters().muted.load()
                                           ? "action.unmute"
                                           : "action.mute"));
  const bool virtualRoute=VirtualDeviceDetector::isVirtual(engine.deviceManager().getAudioDeviceSetup().outputDeviceName);
  status.setText(engine.isRunning() ? (engine.parameters().muted.load()
                                           ? "MICROFONE SILENCIADO"
                                           : (virtualRoute&&integrationsPage?integrationsPage->activeTarget()+" · ROTEAMENTO ATIVO":"ENTRADA / SAIDA EM TEMPO REAL"))
                                    : "MODIFICADOR DESLIGADO",
                 juce::dontSendNotification);
  status.setColour(juce::Label::textColourId,
                   engine.isRunning() ? theme::green : theme::muted);
  double latency = 0;
  if (d && d->getCurrentSampleRate() > 0)
    latency = 1000.0 *
              (d->getInputLatencyInSamples() + d->getOutputLatencyInSamples() +
               d->getCurrentBufferSizeSamples()) /
              d->getCurrentSampleRate();
  cpuLatency.setText(
      "Latencia\n" + (d ? juce::String(latency, 1) + " ms" : "--") + "\nCPU " +
          juce::String(engine.cpuUsage() * 100, 1) + "%",
      juce::dontSendNotification);
  if (currentPage == 9)
    diagnosticText.setText(DiagnosticsManager::report(engine), false);
  repaint(bottomBounds);
}
} // namespace vox
