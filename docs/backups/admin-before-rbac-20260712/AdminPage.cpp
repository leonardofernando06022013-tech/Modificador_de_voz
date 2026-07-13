#include "AdminPage.h"
#include "App/AppPaths.h"
#include "UI/Theme.h"
namespace vox {
static void adminLabel(juce::Label &l, float size, juce::Colour colour,
                       bool bold = false) {
  l.setColour(juce::Label::textColourId, colour);
  l.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}
AdminPage::AdminPage(AudioEngine &e, SettingsManager &s, PresetManager &p)
    : engine(e), settings(s), presets(p) {
  title.setText(juce::String::fromUTF8("Painel de Administração"), juce::dontSendNotification);
  subtitle.setText(
      juce::String::fromUTF8("Gerencie presets, preferências, segurança, backups e logs locais."),
      juce::dontSendNotification);
  adminLabel(title, 25, theme::text, true);
  adminLabel(subtitle, 13, theme::muted);
  addAndMakeVisible(title);
  addAndMakeVisible(subtitle);
  juce::TextButton *tabs[]{&overview, &presetsTab, &securityTab, &logsTab,
                           &settingsTab};
  for (int i = 0; i < 5; ++i) {
    tabs[i]->setColour(juce::TextButton::buttonColourId,
                       i == 0 ? theme::blue : theme::panel);
    addAndMakeVisible(tabs[i]);
  }
  overview.onClick = [this] { refresh(); };
  presetsTab.onClick = [this] {
    if (navigate)
      navigate(PageId::Presets);
  };
  securityTab.onClick = [this] {
    if (navigate)
      navigate(PageId::Settings);
  };
  logsTab.onClick = [this] { AppPaths::logs().startAsProcess(); };
  settingsTab.onClick = [this] {
    if (navigate)
      navigate(PageId::Settings);
  };
  for (auto *l : {&presetsValue, &favouritesValue, &logsValue, &storageValue,
                  &statusValue}) {
    adminLabel(*l, 16, theme::text, true);
    addAndMakeVisible(l);
  }
  activities.setMultiLine(true);
  activities.setReadOnly(true);
  activities.setScrollbarsShown(true);
  activities.setColour(juce::TextEditor::backgroundColourId,
                       juce::Colours::transparentBlack);
  activities.setColour(juce::TextEditor::outlineColourId,
                       juce::Colours::transparentBlack);
  addAndMakeVisible(activities);
  for (auto *b : {&managePresets, &managePermissions, &security, &openLogs,
                  &backup, &exportButton, &clearButton})
    addAndMakeVisible(b);
  managePresets.onClick = [this] {
    if (navigate)
      navigate(PageId::Presets);
  };
  managePermissions.onClick = [this] {
    if (navigate)
      navigate(PageId::Settings);
  };
  security.onClick = [this] {
    if (navigate)
      navigate(PageId::Settings);
  };
  openLogs.onClick = []() { AppPaths::logs().startAsProcess(); };
  backup.onClick = [this] { createBackup(); };
  exportButton.onClick = [this] { exportLogs(); };
  clearButton.setColour(juce::TextButton::textColourOffId, theme::red);
  clearButton.onClick = [this] { clearOldLogs(); };
  refresh();
  startTimer(2500);
}
void AdminPage::timerCallback() { refresh(); }
juce::String AdminPage::storageText() const {
  int64_t bytes = 0;
  for (auto file : AppPaths::data().findChildFiles(juce::File::findFiles, true))
    bytes += file.getSize();
  for (auto file : AppPaths::logs().findChildFiles(juce::File::findFiles, true))
    bytes += file.getSize();
  return juce::File::descriptionOfSizeInBytes(bytes);
}
void AdminPage::refresh() {
  auto presetFiles = presets.directory().findChildFiles(juce::File::findFiles,
                                                        false, "*.json");
  auto logFiles =
      AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log");
  presetsValue.setText(juce::String(presetFiles.size()) + "\nPresets locais",
                       juce::dontSendNotification);
  favouritesValue.setText(juce::String(settings.favourites().size()) +
                              "\nItens favoritos",
                          juce::dontSendNotification);
  logsValue.setText(juce::String(logFiles.size()) + "\nArquivos de log",
                    juce::dontSendNotification);
  storageValue.setText("Armazenamento\n" + storageText(),
                       juce::dontSendNotification);
  statusValue.setText(engine.deviceManager().getCurrentAudioDevice()
                          ? juce::String::fromUTF8("● Sistema online")
                          : juce::String::fromUTF8("● Áudio indisponível"),
                      juce::dontSendNotification);
  statusValue.setColour(juce::Label::textColourId,
                        engine.deviceManager().getCurrentAudioDevice()
                            ? theme::green
                            : theme::red);
  juce::String recent;
  std::sort(logFiles.begin(), logFiles.end(),
            [](const juce::File &a, const juce::File &b) {
              return a.getLastModificationTime() > b.getLastModificationTime();
            });
  for (int i = 0; i < juce::jmin(5, logFiles.size()); ++i) {
    auto lines = juce::StringArray::fromLines(logFiles[i].loadFileAsString());
    auto last = lines.isEmpty() ? juce::String("Log vazio")
                                : lines[lines.size() - 1];
    recent << logFiles[i].getFileName() << "\n  " << last.substring(0, 110)
           << "\n\n";
  }
  activities.setText(
      recent.isEmpty() ? "Nenhuma atividade registrada." : recent, false);
  repaint();
}
void AdminPage::createBackup() {
  auto target = AppPaths::data().getChildFile(
      "Backup-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"));
  target.createDirectory();
  settings.file().copyFileTo(target.getChildFile("settings.json"));
  auto preferences = settings.file().getSiblingFile("preferences.json");
  if (preferences.existsAsFile())
    preferences.copyFileTo(target.getChildFile("preferences.json"));
  for (auto file : presets.directory().findChildFiles(juce::File::findFiles,
                                                      false, "*.json"))
    file.copyFileTo(target.getChildFile(file.getFileName()));
  if (notify)
    notify("Backup criado em " + target.getFullPathName());
  refresh();
}
void AdminPage::exportLogs() {
  auto target =
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
          .getChildFile(
              "ModificadorDeVoz-Logs-" +
              juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"));
  target.createDirectory();
  for (auto file :
       AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log"))
    file.copyFileTo(target.getChildFile(file.getFileName()));
  if (notify)
    notify("Logs exportados para Documentos");
}
void AdminPage::clearOldLogs() {
  const auto limit =
      juce::Time::getCurrentTime() - juce::RelativeTime::days(30);
  int removed = 0;
  for (auto file :
       AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log"))
    if (file.getLastModificationTime() < limit && file.deleteFile())
      ++removed;
  if (notify)
    notify(juce::String(removed) + " logs antigos removidos");
  refresh();
}
void AdminPage::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  g.setColour(theme::border);
  g.drawLine(12, 112, (float)getWidth() - 12, 112);
  for (auto r : {activityArea, quickArea, infoArea, actionsArea})
    theme::roundedPanel(g, r.toFloat(), 12, theme::panel);
  auto summaries = summaryArea;
  const int gap = 12, w = (summaries.getWidth() - 2 * gap) / 3;
  for (int i = 0; i < 3; ++i)
    theme::roundedPanel(g, summaries.removeFromLeft(w).toFloat(), 12,
                        theme::panel),
        summaries.removeFromLeft(gap);
  g.setColour(theme::text);
  g.setFont(juce::Font(juce::FontOptions(17, juce::Font::bold)));
  g.drawText("Atividades Recentes", activityArea.reduced(18).removeFromTop(28),
             juce::Justification::centredLeft);
  g.drawText(juce::String::fromUTF8("Acesso Rápido"), quickArea.reduced(18).removeFromTop(28),
             juce::Justification::centredLeft);
  g.drawText(juce::String::fromUTF8("Informações do Sistema"), infoArea.reduced(18).removeFromTop(28),
             juce::Justification::centredLeft);
  g.drawText(juce::String::fromUTF8("Ações Rápidas"), actionsArea.reduced(18).removeFromTop(28),
             juce::Justification::centredLeft);
  g.setColour(theme::muted);
  g.setFont(12);
  g.drawText(
      juce::String::fromUTF8("Versão do App                                      1.0.0\n\nAmbiente                                             Local\n\nProcessamento                                  No dispositivo"),
      infoArea.reduced(18).withTrimmedTop(48).withTrimmedBottom(95),
      juce::Justification::topLeft);
}
void AdminPage::resized() {
  auto a = getLocalBounds().reduced(12);
  title.setBounds(a.removeFromTop(34));
  subtitle.setBounds(a.removeFromTop(24));
  auto tabs = a.removeFromTop(52);
  for (auto *b : {&overview, &presetsTab, &securityTab, &logsTab, &settingsTab})
    b->setBounds(tabs.removeFromLeft(145).reduced(3, 7));
  a.removeFromTop(12);
  auto right = a.removeFromRight(355);
  a.removeFromRight(14);
  summaryArea = a.removeFromTop(132);
  auto summaries = summaryArea;
  const int gap = 12, w = (summaries.getWidth() - 2 * gap) / 3;
  presetsValue.setBounds(summaries.removeFromLeft(w).reduced(20));
  summaries.removeFromLeft(gap);
  favouritesValue.setBounds(summaries.removeFromLeft(w).reduced(20));
  summaries.removeFromLeft(gap);
  logsValue.setBounds(summaries.reduced(20));
  a.removeFromTop(14);
  quickArea = a.removeFromRight(370);
  a.removeFromRight(14);
  activityArea = a;
  infoArea = right.removeFromTop(288);
  right.removeFromTop(14);
  actionsArea = right;
  auto activity = activityArea.reduced(18);
  activity.removeFromTop(42);
  activities.setBounds(activity);
  auto quick = quickArea.reduced(18);
  quick.removeFromTop(48);
  for (auto *b : {&managePresets, &managePermissions, &security, &openLogs})
    b->setBounds(quick.removeFromTop(68).reduced(0, 5));
  auto info = infoArea.reduced(18);
  info.removeFromTop(180);
  storageValue.setBounds(info.removeFromTop(48));
  statusValue.setBounds(info.removeFromTop(42));
  auto actions = actionsArea.reduced(18);
  actions.removeFromTop(40);
  for (auto *b : {&backup, &exportButton, &clearButton})
    b->setBounds(actions.removeFromTop(50).reduced(0, 4));
}
} // namespace vox
