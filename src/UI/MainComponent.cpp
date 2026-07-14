#include "MainComponent.h"
#include "App/AppPaths.h"
#include "Integrations/VirtualDeviceDetector.h"
#include "Theme.h"

namespace vox {
namespace {
void styleLabel(juce::Label &label, float size, juce::Colour colour,
                bool bold = false) {
  label.setColour(juce::Label::textColourId, colour);
  label.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}
} // namespace

MainComponent::MainComponent(AudioEngine &audio, SettingsManager &settingsManager)
    : engine(audio), settings(settingsManager),
      devices(audio.deviceManager(), 0, 1, 0, 2, true, true, true, false) {
  setLookAndFeel(&look);
  setSize(1450, 900);
  languageListener = LocalizationManager::instance().addListener(
      [this] { updateTexts(); });

  router.setListener([this](PageId, PageId page) { showPage(page); });
  sidebar.onSelect = [this](int page) {
    if (page <= static_cast<int>(PageId::Admin))
      router.navigateTo(static_cast<PageId>(page));
    else
      notification.showMessage(
          page == 12 ? "Ajuda: passe o mouse sobre os controles para ver dicas."
                     : "Modificador de Voz v1.0.0");
  };
  addAndMakeVisible(sidebar);

  title.setText("Modificador de Voz", juce::dontSendNotification);
  version.setText("v1.0.0", juce::dontSendNotification);
  styleLabel(title, 21, theme::text, true);
  styleLabel(version, 12, theme::muted);
  styleLabel(pageTitle, 23, theme::text, true);
  addAndMakeVisible(title);
  addAndMakeVisible(version);
  addAndMakeVisible(pageTitle);

  search.setTextToShowWhenEmpty("Pesquisar vozes...", theme::muted);
  search.setTooltip("Pesquisar presets em tempo real (Ctrl+K)");
  search.onTextChange = [this] {
    if (voicesPage) voicesPage->setSearchText(search.getText());
  };
  addAndMakeVisible(search);
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
  notifications.setTooltip("Notificações");
  helpButton.setTooltip("Ajuda");
  topSettings.setTooltip("Configurações");
  profile.setTooltip("Perfil local");
  notifications.setColour(juce::TextButton::textColourOffId, theme::red);
  profile.setColour(juce::TextButton::textColourOffId, theme::cyan);
  helpButton.onClick = [this] {
    notification.showMessage("Use os tooltips para conhecer cada controle.");
  };
  topSettings.onClick = [this] { router.navigateTo(PageId::Settings); };
  profile.onClick = [this] { notification.showMessage("Perfil local"); };
  for (auto *button : {&notifications, &helpButton, &topSettings, &profile}) {
    button->setColour(juce::TextButton::buttonColourId,
                      juce::Colours::transparentBlack);
    addAndMakeVisible(button);
  }

  voicesPage = std::make_unique<VoicesPage>(engine, settings, presets);
  voicesPage->notify = [this](const juce::String &message) {
    notification.showMessage(message);
  };
  addAndMakeVisible(*voicesPage);

  auto makeModule = [this](std::unique_ptr<ModulePage> &target,
                           ModulePage::Kind kind) {
    target = std::make_unique<ModulePage>(kind, engine, presets, settings);
    target->navigate = [this](PageId page) { router.navigateTo(page); };
    target->notify = [this](const juce::String &message) {
      notification.showMessage(message);
    };
    addAndMakeVisible(*target);
    target->setVisible(false);
  };
  makeModule(homePage, ModulePage::Kind::Home);
  makeModule(effectsPage, ModulePage::Kind::Effects);
  makeModule(soundboardPage, ModulePage::Kind::Soundboard);
  makeModule(favoritesPage, ModulePage::Kind::Favorites);
  makeModule(equalizerPage, ModulePage::Kind::Equalizer);
  makeModule(presetsModulePage, ModulePage::Kind::Presets);

  adminPage = std::make_unique<AdminPage>(engine, settings, presets);
  adminPage->navigate = [this](PageId page) { router.navigateTo(page); };
  adminPage->notify = [this](const juce::String &message) {
    notification.showMessage(message);
  };
  addAndMakeVisible(*adminPage);
  adminPage->setVisible(false);
  sidebar.setAdminVisible(adminPage->hasAccess());

  integrationsPage = std::make_unique<IntegrationsPage>(engine);
  integrationsPage->notify = [this](const juce::String &message) {
    notification.showMessage(message);
  };
  addAndMakeVisible(*integrationsPage);
  integrationsPage->setVisible(false);

  addAndMakeVisible(devices);
  devices.setVisible(false);
  settingsPage = std::make_unique<SettingsPage>(engine, settings);
  settingsPage->onOpenDevices = [this] { router.navigateTo(PageId::Devices); };
  settingsPage->onPreferencesChanged = [this] { applyPreferences(); };
  addAndMakeVisible(*settingsPage);
  settingsPage->setVisible(false);

  diagnosticText.setMultiLine(true);
  diagnosticText.setReadOnly(true);
  diagnosticText.setColour(juce::TextEditor::backgroundColourId, theme::panel);
  addAndMakeVisible(diagnosticText);
  diagnosticText.setVisible(false);
  copyDiagnostic.onClick = [this] {
    juce::SystemClipboard::copyTextToClipboard(diagnosticText.getText());
    notification.showMessage("Relatório copiado.");
  };
  addAndMakeVisible(copyDiagnostic);
  copyDiagnostic.setVisible(false);

  auto &parameters = engine.parameters();
  power.setComponentID("powerButton");
  power.setColour(juce::TextButton::buttonColourId, theme::blue);
  power.onClick = [this] {
    engine.isRunning() ? engine.stop() : engine.start();
    notification.showMessage(engine.isRunning() ? "Modificador de voz ativado."
                                                 : "Modificador de voz desativado.");
  };
  mute.onClick = [this, &parameters] {
    parameters.muted = !parameters.muted.load();
  };
  bypass.onClick = [&parameters] { parameters.bypass = !parameters.bypass.load(); };
  monitor.onClick = [this] {
    const bool enable = !engine.isMonitoring();
    const auto error = engine.setMonitoring(enable);
    notification.showMessage(error.isNotEmpty()
                                 ? "Falha no monitoramento: " + error
                                 : enable ? "Monitoramento ativo em: " +
                                                engine.monitorDeviceName()
                                          : "Monitoramento desativado.",
                             error.isNotEmpty());
  };
  for (auto *button : {&power, &monitor, &mute, &bypass})
    addAndMakeVisible(button);

  const PageId shortcutPages[]{PageId::Devices, PageId::Effects,
                               PageId::Equalizer, PageId::Settings};
  juce::TextButton *shortcutButtons[]{&shortcutDevices, &shortcutEffects,
                                      &shortcutEqualizer, &shortcutSettings};
  for (int i = 0; i < 4; ++i) {
    shortcutButtons[i]->setColour(juce::TextButton::buttonColourId,
                                  juce::Colours::transparentBlack);
    shortcutButtons[i]->onClick = [this, page = shortcutPages[i]] {
      router.navigateTo(page);
    };
    addAndMakeVisible(shortcutButtons[i]);
  }

  styleLabel(status, 12, theme::green, true);
  styleLabel(cpuLatency, 11, theme::muted);
  styleLabel(deviceName, 10, theme::muted);
  styleLabel(bottomVoiceName, 13, theme::text, true);
  styleLabel(bottomVoiceCategory, 10, theme::muted);
  for (auto *label : {&status, &cpuLatency, &deviceName, &bottomVoiceName,
                      &bottomVoiceCategory})
    addAndMakeVisible(label);
  addAndMakeVisible(meter);
  addChildComponent(notification);

  applyPreferences();
  showPage(PageId::Voices);
  startTimerHz(settings.preference("reduceAnimations", "0") == "1" ? 10 : 30);
  updateTexts();
}

MainComponent::~MainComponent() {
  stopTimer();
  LocalizationManager::instance().removeListener(languageListener);
  settings.save(engine.parameters());
  setLookAndFeel(nullptr);
}

void MainComponent::applyPreferences() {
  compactUi = settings.preference("compact", "0") == "1";
  const bool reduceAnimations =
      settings.preference("reduceAnimations", "0") == "1";
  theme::reduceAnimations.store(reduceAnimations);
  startTimerHz(reduceAnimations ? 10 : 30);
  resized();
  repaint();
}

void MainComponent::showPage(PageId page) {
  currentPage = page;
  sidebar.setSelected(static_cast<int>(page));
  voicesPage->setVisible(page == PageId::Voices);
  homePage->setVisible(page == PageId::Home);
  effectsPage->setVisible(page == PageId::Effects);
  soundboardPage->setVisible(page == PageId::Soundboard);
  favoritesPage->setVisible(page == PageId::Favorites);
  equalizerPage->setVisible(page == PageId::Equalizer);
  devices.setVisible(page == PageId::Devices);
  integrationsPage->setVisible(page == PageId::Integrations);
  presetsModulePage->setVisible(page == PageId::Presets);
  diagnosticText.setVisible(page == PageId::Diagnostics);
  copyDiagnostic.setVisible(page == PageId::Diagnostics);
  settingsPage->setVisible(page == PageId::Settings);
  adminPage->setVisible(page == PageId::Admin);
  pageTitle.setText(PageRouter::title(page), juce::dontSendNotification);
  pageTitle.setVisible(page == PageId::Devices || page == PageId::Diagnostics);
  search.setVisible(page == PageId::Voices);
  clearSearch.setVisible(page == PageId::Voices);
  searchShortcut.setVisible(page == PageId::Voices);
  resized();
  repaint();
}

void MainComponent::updateTexts() {
  auto &localization = LocalizationManager::instance();
  title.setText(localization.text("app.name"), juce::dontSendNotification);
  search.setTextToShowWhenEmpty(localization.text("app.search"), theme::muted);
  search.setTooltip(localization.text("app.search.tooltip"));
  clearSearch.setTooltip(localization.text("app.search.clear"));
  shortcutDevices.setButtonText(localization.text("nav.devices"));
  shortcutEffects.setButtonText(localization.text("nav.effects"));
  shortcutEqualizer.setButtonText(localization.text("nav.equalizer"));
  shortcutSettings.setButtonText(localization.text("nav.settings_short"));
  sidebar.updateTexts();
  pageTitle.setText(PageRouter::title(currentPage), juce::dontSendNotification);
  resized();
  repaint();
}

bool MainComponent::runNavigationSmokeTest() {
  const auto log = AppPaths::logs().getChildFile("ui-smoke.log");
  log.deleteFile();
  const auto mark = [&log](const juce::String &step) {
    log.appendText(step + "\r\n");
  };
  mark("start");
  mark("voices");
  if (!voicesPage || !voicesPage->runSmokeTest() || !settingsPage ||
      !settingsPage->runLanguageSmokeTest())
    return false;
  mark("integrations");
  if (!integrationsPage || !integrationsPage->runSmokeTest()) return false;
  mark("admin");
  if (adminPage) adminPage->runSmokeTest();
  mark("modules");
  for (auto *module : {homePage.get(), effectsPage.get(), soundboardPage.get(),
                       favoritesPage.get(), equalizerPage.get(),
                       presetsModulePage.get()})
    if (!module || !module->runSmokeTest()) return false;
  const PageId pages[]{PageId::Home, PageId::Voices, PageId::Effects,
                       PageId::Soundboard, PageId::Favorites,
                       PageId::Equalizer, PageId::Devices,
                       PageId::Integrations, PageId::Presets,
                       PageId::Diagnostics, PageId::Settings, PageId::Admin};
  for (auto page : pages) {
    mark("navigate " + juce::String(static_cast<int>(page)));
    router.navigateTo(page);
    if (router.currentPage() != page) return false;
  }
  router.navigateTo(PageId::Voices);
  mark("complete");
  return true;
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  g.setColour(theme::secondary);
  g.fillRect(topBounds);
  g.fillRect(bottomBounds);
  g.setColour(theme::border);
  g.drawLine(0, (float)topBounds.getBottom(), (float)getWidth(),
             (float)topBounds.getBottom());
  g.drawLine(0, (float)bottomBounds.getY(), (float)getWidth(),
             (float)bottomBounds.getY());
  juce::ColourGradient logo(theme::cyan, {20.0f, 22.0f}, theme::purple,
                            {52.0f, 52.0f}, false);
  g.setGradientFill(logo);
  for (int i = 0; i < 7; ++i) {
    const float height = 12.0f - std::abs(3 - i) * 1.8f + (i % 2) * 12.0f;
    g.fillRoundedRectangle(22.0f + i * 5.0f, 36.0f - height * .5f, 3.0f,
                           height, 1.5f);
  }
  auto footerPaint = bottomBounds;
  auto voiceCard = footerPaint.removeFromLeft(285).toFloat().reduced(10, 8);
  theme::roundedPanel(g, voiceCard, 10, theme::panel);
  g.setColour(theme::cyan.withAlpha(.16f));
  g.fillEllipse(voiceCard.getX() + 10, voiceCard.getCentreY() - 24, 48, 48);
}

void MainComponent::resized() {
  if (!voicesPage)
    return;
  auto all = getLocalBounds();
  topBounds = all.removeFromTop(72);
  bottomBounds = all.removeFromBottom(88);
  const bool compactNavigation = compactUi || getWidth() < 1200;
  sidebar.setCompact(compactNavigation);
  sidebar.setBounds(all.removeFromLeft(compactNavigation ? 72 : 170));
  title.setBounds(64, 12, 230, 28);
  version.setBounds(66, 39, 80, 18);
  const int searchWidth = juce::jlimit(240, 430, getWidth() - 850);
  search.setBounds(330, 16, searchWidth, 38);
  clearSearch.setBounds(search.getRight() - 34, 21, 28, 27);
  searchShortcut.setBounds(search.getRight() - 92, 23, 52, 23);
  int topRight = getWidth() - 14;
  for (auto *button : {&profile, &topSettings, &helpButton, &notifications}) {
    button->setBounds(topRight - 40, 16, 38, 38);
    topRight -= 44;
  }
  deviceStatus.setBounds(topRight - 270, 12, 270, 46);

  contentBounds = all.reduced(14);
  if (pageTitle.isVisible()) {
    pageTitle.setBounds(contentBounds.removeFromTop(42));
    contentBounds.removeFromTop(2);
  }
  auto pageArea = contentBounds.reduced(2);
  voicesPage->setBounds(pageArea);
  for (auto *module : {homePage.get(), effectsPage.get(), soundboardPage.get(),
                       favoritesPage.get(), equalizerPage.get(),
                       presetsModulePage.get()})
    module->setBounds(pageArea);
  devices.setBounds(pageArea.reduced(6));
  integrationsPage->setBounds(pageArea);
  settingsPage->setBounds(pageArea);
  adminPage->setBounds(pageArea);
  diagnosticText.setBounds(pageArea.withTrimmedBottom(46));
  copyDiagnostic.setBounds(pageArea.removeFromBottom(40).removeFromRight(190)
                               .reduced(3));

  auto footer = bottomBounds.reduced(10, 8);
  auto voice = footer.removeFromLeft(compactUi ? 235 : 285);
  voice.removeFromLeft(62);
  bottomVoiceName.setBounds(voice.removeFromTop(22));
  bottomVoiceCategory.setBounds(voice.removeFromTop(17));
  deviceName.setBounds(voice);
  power.setBounds(footer.removeFromLeft(88).reduced(9));
  monitor.setBounds(footer.removeFromLeft(104).reduced(3, 5));
  mute.setBounds(footer.removeFromLeft(98).reduced(3, 5));
  bypass.setBounds(footer.removeFromLeft(78).reduced(3, 5));
  auto shortcuts = footer.removeFromRight(compactUi ? 220 : 300);
  auto stats = footer.removeFromRight(96);
  cpuLatency.setBounds(stats.reduced(5, 10));
  auto meterArea = footer.reduced(10, 9);
  status.setBounds(meterArea.removeFromTop(18));
  meter.setBounds(meterArea.reduced(0, 5));
  for (auto *button : {&shortcutDevices, &shortcutEffects, &shortcutEqualizer,
                       &shortcutSettings})
    button->setBounds(shortcuts.removeFromLeft(compactUi ? 55 : 75).reduced(2, 5));
  notification.setBounds(getWidth() - 385, 82, 350, 54);
}

void MainComponent::timerCallback() {
  meter.setLevels(engine.processor().inputPeak(), engine.processor().outputPeak());
  auto *device = engine.deviceManager().getCurrentAudioDevice();
  deviceName.setText(device ? "Entrada: " + device->getName().substring(0, 25)
                            : "Entrada indisponível",
                     juce::dontSendNotification);
  bottomVoiceName.setText(voicesPage->selectedVoice(), juce::dontSendNotification);
  bottomVoiceCategory.setText(voicesPage->selectedCategory(),
                              juce::dontSendNotification);
  deviceStatus.setButtonText(device ? "●  Microfone Ativo  ·  " +
                                          device->getName().substring(0, 24)
                                    : "●  Sem dispositivo de áudio");
  deviceStatus.setColour(juce::TextButton::textColourOffId,
                         device ? theme::green : theme::red);
  monitor.setButtonText(engine.isMonitoring() ? "Parar escuta" : "Ouvir voz");
  mute.setButtonText(engine.parameters().muted.load() ? "Reativar" : "Silenciar");
  status.setText(engine.isRunning() ? "ENTRADA / SAÍDA EM TEMPO REAL"
                                    : "MODIFICADOR DESLIGADO",
                 juce::dontSendNotification);
  double latency = 0.0;
  if (device && device->getCurrentSampleRate() > 0)
    latency = 1000.0 * (device->getInputLatencyInSamples() +
                        device->getOutputLatencyInSamples() +
                        device->getCurrentBufferSizeSamples()) /
              device->getCurrentSampleRate();
  cpuLatency.setText("Latência\n" +
                         (device ? juce::String(latency, 1) + " ms" : "--") +
                         "\nCPU " + juce::String(engine.cpuUsage() * 100, 1) + "%",
                     juce::dontSendNotification);
  if (currentPage == PageId::Diagnostics)
    diagnosticText.setText(DiagnosticsManager::report(engine), false);
  repaint(bottomBounds);
}
} // namespace vox
