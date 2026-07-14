#include "SettingsPage.h"
#include "App/AppPaths.h"
#include "UI/Theme.h"
namespace vox {
namespace settingsLayout {
constexpr int cardPaddingX=20,cardPaddingY=22,headerHeight=28,headerGap=18;
constexpr int controlHeight=35,controlGap=10,columnGap=12,rowGap=22,cardHeight=530;
}
static void settingsLabel(juce::Label &l, float s, juce::Colour c,
                          bool b = false) {
  l.setColour(juce::Label::textColourId, c);
  l.setFont(juce::Font(
      juce::FontOptions(s, b ? juce::Font::bold : juce::Font::plain)));
}
SettingsPage::SettingsPage(AudioEngine &e, SettingsManager &s)
    : engine(e), settings(s), localization(LocalizationManager::instance()) {
  title.setText("Configuracoes", juce::dontSendNotification);
  subtitle.setText(
      "Preferencias de interface, audio, desempenho, privacidade e backup.",
      juce::dontSendNotification);
  settingsLabel(title, 24, theme::text, true);
  settingsLabel(subtitle, 13, theme::muted);
  addAndMakeVisible(title);
  addAndMakeVisible(subtitle);
  settingsLabel(saveStatus, 12, theme::green);
  saveStatus.setText("Alteracoes salvas", juce::dontSendNotification);
  addAndMakeVisible(saveStatus);
  restore.setButtonText("Restaurar tudo");
  restore.onClick = [this] { restoreDefaults(); };
  addAndMakeVisible(restore);
  viewport.setViewedComponent(&canvas, false);
  viewport.setScrollBarsShown(true, false);
  addAndMakeVisible(viewport);
  setupCombo(themeBox, {"Escuro"}, 1);
  setupCombo(language, localization.languageNames(),
             localization.languageCodes().indexOf(localization.currentLanguage()) + 1);
  settingsLabel(languageTitle,13,theme::text,true);
  settingsLabel(languageDescription,11,theme::muted);
  languageDescription.setMinimumHorizontalScale(.72f);
  language.setTooltip("Interface language / Idioma da interface");
  canvas.addAndMakeVisible(languageTitle);
  canvas.addAndMakeVisible(languageDescription);
  setupCombo(fontSize, {"Pequeno", "Medio", "Grande"}, 2);
  setupCombo(scale, {"100%", "125%", "150%", "175%", "200%"}, 1);
  setupCombo(quality, {"Baixa (44 kHz)", "Alta (48 kHz)"}, 2);
  setupCombo(buffer,
             {"128 amostras", "256 amostras", "512 amostras", "1024 amostras"},
             2);
  setupCombo(sampleRate, {"44100 Hz", "48000 Hz"}, 2);
  setupCombo(audioMode, {"WASAPI compartilhado"}, 1);
  setupCombo(channels, {"Mono para estereo", "Estereo"}, 1);
  setupCombo(processPriority, {"Normal", "Alta"}, 2);
  setupCombo(meterRate, {"20 Hz", "30 Hz", "60 Hz"}, 2);
  setupCombo(logLevel, {"Erros", "Normal", "Detalhado"}, 2);
  setupCombo(updateChannel, {"Estavel"}, 1);
  setupToggle(compact, "Modo compacto", false);
  setupToggle(reduceAnimations, "Reduzir animacoes", false);
  setupToggle(showTips, "Mostrar dicas", true);
  setupToggle(autoContext, "Painel direito automatico", true);
  setupToggle(monitorInput, "Monitoramento", true);
  setupToggle(safetyLimiter, "Limitador de seguranca",
              engine.parameters().limiterEnabled.load());
  setupToggle(reconnect, "Reconexao automatica", true);
  setupToggle(syncDevices, "Sincronizar dispositivos", true);
  setupToggle(startWindows, "Iniciar com Windows", false);
  setupToggle(startMinimized, "Iniciar minimizado", false);
  setupToggle(startTray, "Iniciar na bandeja", false);
  setupToggle(restorePreset, "Restaurar ultimo preset", true);
  setupToggle(autoProcess, "Ligar processamento automaticamente", false);
  setupToggle(cpuOptimization, "Otimizacao de CPU", true);
  setupToggle(adaptiveQuality, "Qualidade adaptativa", false);
  setupToggle(economy, "Modo economia", false);
  setupToggle(highContrast, "Alto contraste", false);
  setupToggle(focusVisible, "Foco visivel", true);
  setupToggle(largeClick, "Maior area de clique", false);
  setupToggle(keyboardNav, "Navegacao por teclado", true);
  setupToggle(saveLogs, "Salvar logs", true);
  setupToggle(telemetry, "Telemetria (sempre desativada)", false);
  telemetry.setEnabled(false);
  setupToggle(autoUpdates, "Atualizacao automatica", false);
  loadPreferences();
  juce::ComboBox *preferenceCombos[]{&themeBox, &fontSize,
                                     &scale,    &processPriority, &meterRate,
                                     &logLevel};
  for (auto *c : preferenceCombos)
    c->onChange = [this] { markDirty(); };
  language.onChange=[this]{const int index=language.getSelectedItemIndex();if(index<0)return;saveStatus.setText(localization.text("settings.applying"),juce::dontSendNotification);const auto codes=localization.languageCodes();if(index<codes.size()&&localization.setLanguage(codes[index])){saveStatus.setText(localization.text("settings.applied"),juce::dontSendNotification);saveStatus.setColour(juce::Label::textColourId,theme::green);}else{saveStatus.setText(localization.text("settings.failed"),juce::dontSendNotification);saveStatus.setColour(juce::Label::textColourId,theme::red);language.setSelectedId(codes.indexOf(localization.currentLanguage())+1,juce::dontSendNotification);}};
  juce::ToggleButton *preferenceToggles[]{
      &compact,         &reduceAnimations, &showTips,    &autoContext,
      &cpuOptimization, &adaptiveQuality,  &economy,     &highContrast,
      &focusVisible,    &largeClick,       &keyboardNav, &saveLogs,
      &autoUpdates};
  for (auto *t : preferenceToggles)
    t->onClick = [this] { markDirty(); };
  cpuOptimization.onClick = [this] {
    if (cpuOptimization.getToggleState() && buffer.getSelectedId() < 3)
      buffer.setSelectedId(3, juce::sendNotificationSync);
    markDirty();
  };
  economy.onClick = [this] {
    if (economy.getToggleState())
      buffer.setSelectedId(4, juce::sendNotificationSync);
    markDirty();
  };
  buffer.onChange = [this] { applyDevice(buffer.getText().getIntValue(), 0); };
  sampleRate.onChange = [this] {
    applyDevice(0, sampleRate.getText().getDoubleValue());
  };
  quality.onChange = [this] {
    sampleRate.setSelectedId(quality.getSelectedId() == 1 ? 1 : 2,
                             juce::sendNotificationSync);
  };
  safetyLimiter.onClick = [this] {
    engine.parameters().limiterEnabled = safetyLimiter.getToggleState();
    markDirty();
  };
  startWindows.onClick = [this] {
    const auto key = "HKEY_CURRENT_"
                     "USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\"
                     "ModificadorDeVoz";
    if (startWindows.getToggleState())
      juce::WindowsRegistry::setValue(
          key,
          "\"" +
              juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                  .getFullPathName() +
              "\"");
    else
      juce::WindowsRegistry::deleteValue(key);
    markDirty();
  };
  openDevices.setButtonText("Abrir pagina Dispositivos");
  openDevices.onClick = [this] {
    if (onOpenDevices)
      onOpenDevices();
  };
  exportButton.setButtonText("Exportar configuracoes");
  importButton.setButtonText("Importar configuracoes");
  openLogs.setButtonText("Abrir logs");
  clearLogs.setButtonText("Limpar logs");
  clearCache.setButtonText("Limpar cache");
  checkUpdates.setButtonText("Versao 1.0.0 - sem servidor de atualizacao");
  checkUpdates.setEnabled(false);
  backupButton.setButtonText("Criar backup local");
  for (auto *b : {&openDevices, &exportButton, &importButton, &openLogs,
                  &clearLogs, &clearCache, &checkUpdates, &backupButton})
    canvas.addAndMakeVisible(b);
  exportButton.onClick = [this] {
    auto f = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                 .getChildFile("ModificadorDeVoz-settings.json");
    f.replaceWithText(juce::JSON::toString(
        SettingsManager::parametersToJson(engine.parameters()), true));
    saveStatus.setText("Exportado para Documentos", juce::dontSendNotification);
  };
  importButton.onClick = [this] {
    auto f = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                 .getChildFile("ModificadorDeVoz-settings.json");
    auto backup = AppPaths::data().getChildFile("settings-before-import.json");
    backup.replaceWithText(juce::JSON::toString(
        SettingsManager::parametersToJson(engine.parameters()), true));
    if (f.existsAsFile() && SettingsManager::jsonToParameters(
                                juce::JSON::parse(f), engine.parameters()))
      markDirty();
    else
      saveStatus.setText("Arquivo de importacao nao encontrado",
                         juce::dontSendNotification);
  };
  openLogs.onClick = [] { AppPaths::logs().startAsProcess(); };
  clearLogs.onClick = []() {
    for (auto &f :
         AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log"))
      f.deleteFile();
  };
  clearCache.onClick = []() {
    for (auto &f : AppPaths::cache().findChildFiles(
             juce::File::findFilesAndDirectories, false))
      f.deleteRecursively();
  };
  backupButton.onClick = [this] {
    auto f = AppPaths::data().getChildFile(
        "settings-backup-" +
        juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + ".json");
    f.replaceWithText(juce::JSON::toString(
        SettingsManager::parametersToJson(engine.parameters()), true));
    saveStatus.setText("Backup local criado", juce::dontSendNotification);
  };
  const juce::Array<juce::Component *> controls{
      &themeBox,        &language,         &fontSize,        &scale,
      &compact,         &reduceAnimations, &showTips,        &autoContext,
      &quality,         &buffer,           &sampleRate,      &audioMode,
      &channels,        &monitorInput,     &safetyLimiter,   &reconnect,
      &syncDevices,     &startWindows,     &startMinimized,  &startTray,
      &restorePreset,   &autoProcess,      &processPriority, &cpuOptimization,
      &adaptiveQuality, &meterRate,        &economy,         &highContrast,
      &focusVisible,    &largeClick,       &keyboardNav,     &saveLogs,
      &logLevel,        &telemetry,        &updateChannel,   &autoUpdates};
  for (auto *c : controls)
    canvas.addAndMakeVisible(c);
  languageListener=localization.addListener([this]{updateTexts();});
  updateTexts();
}
SettingsPage::~SettingsPage() {
  localization.removeListener(languageListener);
  stopTimer();
  if (dirty)
    settings.save(engine.parameters());
}
bool SettingsPage::runLanguageSmokeTest(){const auto original=localization.currentLanguage();if(!localization.validateAll())return false;for(const auto&code:localization.languageCodes()){if(!localization.setLanguage(code)||localization.text("settings.title").isEmpty()||localization.text("settings.title").startsWithChar('['))return false;}return localization.setLanguage(original);}
void SettingsPage::updateTexts(){title.setText(localization.text("settings.title"),juce::dontSendNotification);subtitle.setText(localization.text("settings.subtitle"),juce::dontSendNotification);restore.setButtonText(localization.text("settings.restore"));languageTitle.setText(localization.text("settings.language"),juce::dontSendNotification);languageDescription.setText(localization.text("settings.language.description"),juce::dontSendNotification);languageDescription.setTooltip(localization.text("settings.language.description"));repaint();resized();}
void SettingsPage::setupCombo(juce::ComboBox &c, const juce::StringArray &i,
                              int selected) {
  c.addItemList(i, 1);
  c.setSelectedId(selected, juce::dontSendNotification);
}
void SettingsPage::setupToggle(juce::ToggleButton &t, const juce::String &n,
                               bool state) {
  t.setButtonText(n);
  t.setToggleState(state, juce::dontSendNotification);
}
void SettingsPage::markDirty() {
  persistPreferences();
  dirty = true;
  saveStatus.setText(localization.text("settings.saving"), juce::dontSendNotification);
  startTimer(550);
}
void SettingsPage::loadPreferences() {
  auto loadToggle = [this](juce::ToggleButton &toggle, const char *key) {
    const auto fallback = toggle.getToggleState() ? "1" : "0";
    toggle.setToggleState(settings.preference(key, fallback) == "1",
                          juce::dontSendNotification);
  };
  auto loadCombo = [this](juce::ComboBox &combo, const char *key) {
    combo.setSelectedId(settings.preference(key, juce::String(combo.getSelectedId()))
                            .getIntValue(),
                        juce::dontSendNotification);
  };
  loadToggle(compact, "compact");
  loadToggle(reduceAnimations, "reduceAnimations");
  loadToggle(showTips, "showTips");
  loadToggle(autoContext, "autoContext");
  loadToggle(cpuOptimization, "cpuOptimization");
  loadToggle(adaptiveQuality, "adaptiveQuality");
  loadToggle(economy, "economy");
  loadToggle(highContrast, "highContrast");
  loadToggle(focusVisible, "focusVisible");
  loadToggle(largeClick, "largeClick");
  loadToggle(keyboardNav, "keyboardNav");
  loadToggle(saveLogs, "saveLogs");
  loadToggle(autoUpdates, "autoUpdates");
  loadCombo(themeBox, "theme");
  loadCombo(fontSize, "fontSize");
  loadCombo(scale, "scale");
  loadCombo(processPriority, "processPriority");
  loadCombo(meterRate, "meterRate");
  loadCombo(logLevel, "logLevel");
}
void SettingsPage::persistPreferences() {
  auto saveToggle = [this](const char *key, const juce::ToggleButton &toggle) {
    settings.setPreference(key, toggle.getToggleState() ? "1" : "0");
  };
  auto saveCombo = [this](const char *key, const juce::ComboBox &combo) {
    settings.setPreference(key, juce::String(combo.getSelectedId()));
  };
  saveToggle("compact", compact);
  saveToggle("reduceAnimations", reduceAnimations);
  saveToggle("showTips", showTips);
  saveToggle("autoContext", autoContext);
  saveToggle("cpuOptimization", cpuOptimization);
  saveToggle("adaptiveQuality", adaptiveQuality);
  saveToggle("economy", economy);
  saveToggle("highContrast", highContrast);
  saveToggle("focusVisible", focusVisible);
  saveToggle("largeClick", largeClick);
  saveToggle("keyboardNav", keyboardNav);
  saveToggle("saveLogs", saveLogs);
  saveToggle("autoUpdates", autoUpdates);
  saveCombo("theme", themeBox);
  saveCombo("fontSize", fontSize);
  saveCombo("scale", scale);
  saveCombo("processPriority", processPriority);
  saveCombo("meterRate", meterRate);
  saveCombo("logLevel", logLevel);
  if (onPreferencesChanged)
    onPreferencesChanged();
}
void SettingsPage::timerCallback() {
  stopTimer();
  const bool ok = settings.save(engine.parameters());
  dirty = !ok;
  saveStatus.setText(ok ? localization.text("settings.saved") : localization.text("settings.failed"),
                     juce::dontSendNotification);
  saveStatus.setColour(juce::Label::textColourId,
                       ok ? theme::green : theme::red);
}
void SettingsPage::applyDevice(int block, double rate) {
  auto setup = engine.deviceManager().getAudioDeviceSetup();
  if (block)
    setup.bufferSize = block;
  if (rate)
    setup.sampleRate = rate;
  auto error = engine.deviceManager().setAudioDeviceSetup(setup, true);
  if (error.isEmpty())
    markDirty();
  else {
    saveStatus.setText("Falha no audio: " + error, juce::dontSendNotification);
    saveStatus.setColour(juce::Label::textColourId, theme::red);
  }
}
void SettingsPage::restoreDefaults() {
  auto &p = engine.parameters();
  p.inputGainDb = 0;
  p.outputGainDb = 0;
  p.mix = 1;
  p.pitchSemitones = 0;
  p.fineCents = 0;
  p.formant = 0;
  p.noiseReduction = .25f;
  p.distortion = 0;
  p.chorus = 0;
  p.delay = 0;
  p.reverb = 0;
  p.ringMod = 0;
  p.bitCrush = 0;
  p.limiterEnabled = true;
  markDirty();
}
void SettingsPage::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  const juce::StringArray names{localization.text("settings.interface"),localization.text("settings.audio"),localization.text("settings.devices"),localization.text("settings.startup"),localization.text("settings.performance"),localization.text("settings.accessibility"),localization.text("settings.privacy"),localization.text("settings.updates")};
  const auto viewOffset=viewport.getPosition()-viewport.getViewPosition();
  juce::Graphics::ScopedSaveState state(g);
  g.reduceClipRegion(viewport.getBounds());
  for (int i = 0; i < cards.size(); ++i) {
    const auto card=cards[i].translated(viewOffset.x,viewOffset.y);
    theme::roundedPanel(g,card.toFloat(),12,theme::panel);
    g.setColour(i % 2 ? theme::cyan : theme::purple);
    g.setFont(juce::Font(juce::FontOptions(16, juce::Font::bold)));
    auto header=card.reduced(settingsLayout::cardPaddingX,settingsLayout::cardPaddingY).removeFromTop(settingsLayout::headerHeight);
    g.drawText(names[i], header,
               juce::Justification::centredLeft);
  }
}
void SettingsPage::row(juce::Rectangle<int> &r, juce::Component &c) {
  c.setBounds(r.removeFromTop(settingsLayout::controlHeight));
  r.removeFromTop(settingsLayout::controlGap);
}
void SettingsPage::layoutCard(int i, juce::Rectangle<int> r) {
  r=r.reduced(settingsLayout::cardPaddingX,settingsLayout::cardPaddingY);
  r.removeFromTop(settingsLayout::headerHeight);
  r.removeFromTop(settingsLayout::headerGap);
  switch (i) {
  case 0:
    row(r, themeBox);
    languageTitle.setBounds(r.removeFromTop(20));
    row(r, language);
    languageDescription.setBounds(r.removeFromTop(34));
    r.removeFromTop(settingsLayout::controlGap);
    row(r, fontSize);
    row(r, scale);
    row(r, compact);
    row(r, reduceAnimations);
    row(r, showTips);
    row(r, autoContext);
    break;
  case 1:
    row(r, quality);
    row(r, sampleRate);
    row(r, buffer);
    row(r, audioMode);
    row(r, channels);
    row(r, monitorInput);
    row(r, safetyLimiter);
    break;
  case 2:
    row(r, reconnect);
    row(r, syncDevices);
    row(r, openDevices);
    break;
  case 3:
    row(r, startWindows);
    row(r, startMinimized);
    row(r, startTray);
    row(r, restorePreset);
    row(r, autoProcess);
    break;
  case 4:
    row(r, processPriority);
    row(r, cpuOptimization);
    row(r, adaptiveQuality);
    row(r, meterRate);
    row(r, economy);
    break;
  case 5:
    row(r, highContrast);
    row(r, focusVisible);
    row(r, largeClick);
    row(r, keyboardNav);
    break;
  case 6:
    row(r, telemetry);
    row(r, saveLogs);
    row(r, logLevel);
    row(r, openLogs);
    row(r, clearLogs);
    row(r, clearCache);
    break;
  case 7:
    row(r, updateChannel);
    row(r, autoUpdates);
    row(r, checkUpdates);
    row(r, exportButton);
    row(r, importButton);
    row(r, backupButton);
    break;
  }
}
void SettingsPage::resized() {
  auto a = getLocalBounds().reduced(8);
  title.setBounds(a.getX(), 0, 380, 34);
  subtitle.setBounds(a.getX(), 32, juce::jmax(420,getWidth()-510), 22);
  saveStatus.setBounds(getWidth() - 470, 12, 170, 30);
  restore.setBounds(getWidth() - 290, 10, 275, 36);
  a.removeFromTop(60);
  viewport.setBounds(a);
  const int gap = settingsLayout::columnGap;
  const int rowGap = settingsLayout::rowGap;
  const int w = (a.getWidth() - gap) / 2;
  const int h = settingsLayout::cardHeight;
  cards.clear();
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 2; ++col)
      cards.add({8 + col * (w + gap), 8 + row * (h + rowGap), w, h});
  canvas.setSize(a.getWidth() - 14, 8 + 4 * h + 3 * rowGap + 18);
  for (int i = 0; i < cards.size(); ++i)
    layoutCard(i, cards[i]);
}
} // namespace vox
