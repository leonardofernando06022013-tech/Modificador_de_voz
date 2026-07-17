#include "IntegrationsPage.h"
#include "App/AppPaths.h"
#include "Platform/WindowsAudioSettingsLauncher.h"
#include "UI/Theme.h"
namespace vox {
static void integrationLabel(juce::Label &l, float size, juce::Colour colour,
                             bool bold = false) {
  l.setColour(juce::Label::textColourId, colour);
  l.setFont(juce::Font(
      juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
}
IntegrationsPage::IntegrationsPage(AudioEngine &e) : engine(e), routing(e) {
  title.setText(juce::String::fromUTF8("Integrações"), juce::dontSendNotification);
  subtitle.setText(juce::String::fromUTF8("Envie sua voz processada para Discord, FiveM, OBS, jogos e chamadas por um dispositivo virtual legítimo."),
                   juce::dontSendNotification);
  integrationLabel(title, 25, theme::text, true);
  integrationLabel(subtitle, 13, theme::muted);
  addAndMakeVisible(title);
  addAndMakeVisible(subtitle);
  const juce::StringArray names{"Discord", "FiveM",    "Jogos",
                                "OBS",     "Chamadas", "Manual"};
  for (auto name : names) {
    auto *b = targets.add(new juce::TextButton(name));
    b->setColour(juce::TextButton::buttonColourId,
                 name == target ? theme::blue : theme::panel);
    b->onClick = [this, name] { selectTarget(name); };
    addAndMakeVisible(b);
  }
  for (auto *c :
       {&inputBox, &outputBox, &profileBox, &sampleRateBox, &bufferBox})
    addAndMakeVisible(c);
  sampleRateBox.addItemList({"48000 Hz", "44100 Hz"}, 1);
  sampleRateBox.setSelectedId(1);
  bufferBox.addItemList({"128 samples", "256 samples", "480 samples",
                         "512 samples", "1024 samples"},
                        1);
  bufferBox.setSelectedId(2);
  monitoring.setTooltip(
      juce::String::fromUTF8("Monitorar a própria voz pode causar eco sem fones de ouvido"));
  addAndMakeVisible(monitoring);
  for (auto *b : {&reload, &apply, &test, &copyDevice, &windowsSound, &guide,
                  &saveProfileButton, &deleteProfile, &duplicateProfile,
                  &exportProfiles, &importProfiles})
    addAndMakeVisible(b);
  reload.onClick = [this] { refreshDevices(); };
  apply.setColour(juce::TextButton::buttonColourId, theme::blue);
  apply.onClick = [this] { applyRouting(); };
  test.onClick = [this] { testRouting(); };
  copyDevice.onClick = [this] {
    juce::SystemClipboard::copyTextToClipboard(outputBox.getText());
    if (notify)
      notify("Nome do dispositivo copiado");
  };
  windowsSound.onClick = []() { WindowsAudioSettingsLauncher::sound(); };
  guide.onClick = [this] {
    instructions.setCaretPosition(0);
    instructions.grabKeyboardFocus();
  };
  saveProfileButton.onClick = [this] { saveProfile(); };
  profileBox.setEditableText(true);
  duplicateProfile.onClick=[this]{auto items=profileManager.load();for(auto&p:items)if(p.name==profileBox.getText()){p.name+=juce::String::fromUTF8(" - cópia");profileManager.upsert(p);refreshProfiles();profileBox.setText(p.name,juce::dontSendNotification);if(notify)notify("Perfil duplicado");break;}};
  exportProfiles.onClick=[this]{auto file=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("integration-profiles.json");if(profileManager.exportTo(file)&&notify)notify("Perfis exportados para Documentos");};
  importProfiles.onClick=[this]{auto file=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("integration-profiles.json");if(profileManager.importFrom(file)){refreshProfiles();if(notify)notify("Perfis importados");}else if(notify)notify(juce::String::fromUTF8("Arquivo de perfis não encontrado ou inválido"));};
  deleteProfile.onClick = [this] {
    if (profileBox.getText().isNotEmpty()) {
      profileManager.remove(profileBox.getText());
      refreshProfiles();
      if (notify)
        notify("Perfil removido");
    }
  };
  profileBox.onChange = [this] {
    auto items = profileManager.load();
    for (auto &p : items)
      if (p.name == profileBox.getText()) {
        inputBox.setText(p.inputDevice, juce::dontSendNotification);
        outputBox.setText(p.virtualOutput, juce::dontSendNotification);
        sampleRateBox.setText(juce::String(p.sampleRate, 0) + " Hz",
                              juce::dontSendNotification);
        bufferBox.setText(juce::String(p.bufferSize) + " samples",
                          juce::dontSendNotification);
        target = p.target;
        selectTarget(target);
        break;
      }
  };
  instructions.setMultiLine(true);
  instructions.setReadOnly(true);
  instructions.setScrollbarsShown(true);
  instructions.setColour(juce::TextEditor::backgroundColourId, theme::panel);
  addAndMakeVisible(instructions);
  integrationLabel(virtualWarning, 12, theme::yellow, true);
  integrationLabel(diagnosticTitle, 15, theme::text, true);
  integrationLabel(diagnosticDetails, 11, theme::muted);
  integrationLabel(inputMeterLabel, 11, theme::muted, true);
  integrationLabel(outputMeterLabel, 11, theme::muted, true);
  for (auto *l : {&virtualWarning, &diagnosticTitle, &diagnosticDetails,
                  &inputMeterLabel, &outputMeterLabel})
    addAndMakeVisible(l);
  addAndMakeVisible(inputMeter);
  addAndMakeVisible(outputMeter);
  refreshDevices();
  refreshProfiles();
  selectTarget("Discord");
  startTimerHz(20);
}
void IntegrationsPage::refreshDevices() {
  auto input = inputBox.getText(), output = outputBox.getText();
  detected = VirtualDeviceDetector::scan(engine.deviceManager());
  auto inputs = VirtualDeviceDetector::inputs(detected),
       outputs = VirtualDeviceDetector::virtualOutputs(detected);
  inputBox.clear();
  inputBox.addItemList(inputs, 1);
  outputBox.clear();
  outputBox.addItemList(outputs, 1);
  auto setup = engine.deviceManager().getAudioDeviceSetup();
  inputBox.setText(input.isNotEmpty() ? input : setup.inputDeviceName,
                   juce::dontSendNotification);
  if (output.isNotEmpty())
    outputBox.setText(output, juce::dontSendNotification);
  else if (outputs.contains(setup.outputDeviceName))
    outputBox.setText(setup.outputDeviceName, juce::dontSendNotification);
  else if (!outputs.isEmpty())
    outputBox.setSelectedId(1);
  const bool found = !outputs.isEmpty();
  virtualWarning.setText(
      found ? juce::String::fromUTF8("● Dispositivo virtual detectado")
            : "Nenhum dispositivo virtual foi encontrado. Instale o VB-CABLE "
              "ou configure o VoiceMeeter para utilizar sua voz modificada.",
      juce::dontSendNotification);
  virtualWarning.setColour(juce::Label::textColourId,
                           found ? theme::green : theme::yellow);
  apply.setEnabled(found && inputBox.getText().isNotEmpty());
  repaint();
}
void IntegrationsPage::selectTarget(const juce::String &name) {
  target = name;
  for (auto *b : targets)
    b->setColour(juce::TextButton::buttonColourId,
                 b->getButtonText() == name ? theme::blue : theme::panel);
  juce::String text;
  if (name == "Discord")
    text = juce::String::fromUTF8("CONFIGURAR NO DISCORD\n\n1. No BlackVoice, escolha seu microfone real como entrada.\n2. No BlackVoice, escolha CABLE Input como saída e clique Aplicar roteamento.\n3. No Discord, abra Configurações > Voz e Vídeo.\n4. Em Dispositivo de entrada, escolha CABLE Output.\n5. Mantenha o BlackVoice ligado durante a chamada.\n\nSe os efeitos forem cortados, desative a supressão de ruído e o ganho automático do Discord.");
  else if (name == "FiveM")
    text = juce::String::fromUTF8("CONFIGURAR NO FIVEM\n\n1. No BlackVoice, envie a saída para CABLE Input.\n2. No Windows ou FiveM, selecione CABLE Output como microfone.\n3. Reinicie o FiveM se a lista não atualizar.\n4. Mantenha o BlackVoice aberto e ligado.\n5. Teste no servidor e ajuste o ganho para evitar distorção.\n\nNenhum arquivo do FiveM ou anticheat é modificado.");
  else if (name == "OBS")
    text = juce::String::fromUTF8("CONFIGURAR NO OBS\n\n1. Adicione uma fonte Captura de Entrada de Áudio.\n2. Selecione o dispositivo virtual.\n3. Desative fontes duplicadas.\n4. Monitore o nível e não adicione o microfone físico ao mesmo tempo.");
  else if (name == "Jogos")
    text = juce::String::fromUTF8("CONFIGURAR EM JOGOS\n\n1. No BlackVoice, escolha CABLE Input como saída e aplique o roteamento.\n2. No jogo, escolha CABLE Output como microfone.\n3. Se não houver seletor, defina CABLE Output como entrada padrão do Windows.\n4. Reinicie o jogo e teste o chat de voz.\n\nCompatível com jogos que usam a entrada normal do Windows, Steam Voice e chats próprios.");
  else if (name == "Chamadas")
    text = juce::String::fromUTF8("ZOOM, TEAMS, MEET E OUTROS\n\nNo BlackVoice, escolha CABLE Input como saída. No aplicativo de chamadas, escolha CABLE Output como microfone e execute o teste de áudio. Mantenha o BlackVoice ativo.");
  else
    text = juce::String::fromUTF8("CONFIGURAÇÃO MANUAL\n\nFluxo: microfone físico → processamento → saída virtual → aplicativo de destino.\nPrefira 48.000 Hz e buffer de 256 ou 480 samples. Use fones ao ativar monitoramento.");
  instructions.setText(text, false);
  repaint();
}
void IntegrationsPage::applyRouting() {
  IntegrationProfile p;
  p.name = profileBox.getText().isNotEmpty() ? profileBox.getText() : target;
  p.target = target;
  p.inputDevice = inputBox.getText();
  p.virtualOutput = outputBox.getText();
  p.sampleRate = sampleRateBox.getText().getDoubleValue();
  p.bufferSize = bufferBox.getText().getIntValue();
  p.monitoring = monitoring.getToggleState();
  p.gainDb = engine.parameters().inputGainDb.load();
  p.gateThreshold = engine.parameters().gateThreshold.load();
  p.limiter = engine.parameters().limiterEnabled.load();
  for (auto &d : detected)
    if (d.name == p.virtualOutput && !d.input) {
      p.audioType = d.type;
      break;
    }
  auto error = routing.apply(p);
  if (error.isEmpty()) {
    if (!engine.isRunning())
      engine.start();
    const auto monitoringError = engine.setMonitoring(p.monitoring);
    if (monitoringError.isNotEmpty()) {
      diagnosticTitle.setText("Roteamento aplicado; falha no monitoramento",
                              juce::dontSendNotification);
      diagnosticDetails.setText(monitoringError,
                                juce::dontSendNotification);
    }
    p.lastUsed = juce::Time::getCurrentTime();
    profileManager.upsert(p);
    refreshProfiles();
    if (notify)
      notify("Roteamento aplicado: " + p.inputDevice + juce::String::fromUTF8(" → ") + p.virtualOutput);
  } else {
    diagnosticTitle.setText(juce::String::fromUTF8("Falha ao abrir a saída"),
                            juce::dontSendNotification);
    diagnosticDetails.setText(error, juce::dontSendNotification);
    if (notify)
      notify("Falha no roteamento: " + error);
  }
}
void IntegrationsPage::testRouting() {
  auto result = RoutingDiagnostics::run(engine, detected);
  diagnosticTitle.setText(result.headline, juce::dontSendNotification);
  diagnosticDetails.setText(result.details + juce::String::fromUTF8("  Latência estimada: ") +
                                juce::String(routing.latencyMs(), 1) + " ms.",
                            juce::dontSendNotification);
  diagnosticTitle.setColour(juce::Label::textColourId,
                            result.success ? theme::green : theme::yellow);
}
void IntegrationsPage::saveProfile() {
  IntegrationProfile p;
  p.name = profileBox.getText().isNotEmpty() ? profileBox.getText() : target;
  p.target = target;
  p.inputDevice = inputBox.getText();
  p.virtualOutput = outputBox.getText();
  p.sampleRate = sampleRateBox.getText().getDoubleValue();
  p.bufferSize = bufferBox.getText().getIntValue();
  p.monitoring = monitoring.getToggleState();
  p.lastUsed = juce::Time::getCurrentTime();
  p.gainDb = engine.parameters().inputGainDb.load();
  p.gateThreshold = engine.parameters().gateThreshold.load();
  p.limiter = engine.parameters().limiterEnabled.load();
  profileManager.upsert(p);
  refreshProfiles();
  profileBox.setText(p.name, juce::dontSendNotification);
  if (notify)
    notify("Perfil salvo: " + p.name);
}
void IntegrationsPage::refreshProfiles() {
  auto selected = profileBox.getText();
  profileBox.clear();
  juce::StringArray names;
  for (auto &p : profileManager.load())
    names.add(p.name);
  profileBox.addItemList(names, 1);
  if (selected.isNotEmpty())
    profileBox.setText(selected, juce::dontSendNotification);
}
void IntegrationsPage::timerCallback() {
  const auto in = engine.processor().inputPeak(),
             out = engine.processor().outputPeak();
  inputMeter.setLevels(in, in);
  outputMeter.setLevels(out, out);
  inputMeterLabel.setText(juce::String::fromUTF8("Entrada física · ") + routing.currentInput(),
                          juce::dontSendNotification);
  outputMeterLabel.setText(juce::String::fromUTF8("Saída processada · ") + routing.currentOutput(),
                           juce::dontSendNotification);
}
void IntegrationsPage::paint(juce::Graphics &g) {
  g.fillAll(theme::background);
  for (auto r : {flowArea, setupArea, guideArea, statusArea})
    theme::roundedPanel(g, r.toFloat(), 12, theme::panel);
  g.setColour(theme::text);
  g.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
  auto flow = flowArea.reduced(16);
  const juce::StringArray steps{"1  Microfone real", "2  BlackVoice",
                                "3  Dispositivo virtual",
                                "4  Aplicativo de destino"};
  for (int i = 0; i < 4; ++i) {
    auto card =
        flow.removeFromLeft((flowArea.getWidth() - 70) / 4).reduced(3, 10);
    g.setColour(i == 1 ? theme::purple.withAlpha(.25f) : theme::elevated);
    g.fillRoundedRectangle(card.toFloat(), 9);
    g.setColour(i == 1 ? theme::cyan : theme::text);
    g.drawFittedText(steps[i], card, juce::Justification::centred, 2);
    if (i < 3) {
      g.setColour(theme::cyan);
      g.drawText(juce::String::fromUTF8("→"), flow.removeFromLeft(18), juce::Justification::centred);
    }
  }
  g.setColour(theme::text);
  g.setFont(juce::Font(juce::FontOptions(16, juce::Font::bold)));
  g.drawText(juce::String::fromUTF8("Assistente de configuração"),
             setupArea.reduced(16).removeFromTop(25),
             juce::Justification::centredLeft);
  g.drawText("Guia do aplicativo", guideArea.reduced(16).removeFromTop(25),
             juce::Justification::centredLeft);
  g.drawText("Teste de roteamento", statusArea.reduced(16).removeFromTop(25),
             juce::Justification::centredLeft);
}
void IntegrationsPage::resized() {
  auto a = getLocalBounds().reduced(12);
  title.setBounds(a.removeFromTop(34));
  subtitle.setBounds(a.removeFromTop(24));
  auto tabs = a.removeFromTop(48);
  for (auto *b : targets)
    b->setBounds(tabs.removeFromLeft(105).reduced(3, 7));
  flowArea = a.removeFromTop(94);
  a.removeFromTop(12);
  auto right = a.removeFromRight(390);
  a.removeFromRight(12);
  setupArea = a;
  guideArea = right.removeFromTop(right.getHeight() / 2 - 6);
  right.removeFromTop(12);
  statusArea = right;
  auto setup = setupArea.reduced(16);
  setup.removeFromTop(38);
  virtualWarning.setBounds(setup.removeFromTop(48));
  auto row = setup.removeFromTop(38);
  inputBox.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
  outputBox.setBounds(row.reduced(2));
  setup.removeFromTop(8);
  row = setup.removeFromTop(38);
  sampleRateBox.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
  bufferBox.setBounds(row.reduced(2));
  monitoring.setBounds(setup.removeFromTop(38));
  row = setup.removeFromTop(44);
  reload.setBounds(row.removeFromLeft(row.getWidth() / 3).reduced(2));
  apply.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2));
  test.setBounds(row.reduced(2));
  setup.removeFromTop(8);
  profileBox.setBounds(setup.removeFromTop(38));
  row = setup.removeFromTop(42);
  saveProfileButton.setBounds(
      row.removeFromLeft(row.getWidth() / 2).reduced(2));
  deleteProfile.setBounds(row.reduced(2));
  row=setup.removeFromTop(38);duplicateProfile.setBounds(row.removeFromLeft(row.getWidth()/3).reduced(2));exportProfiles.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));importProfiles.setBounds(row.reduced(2));
  auto guideBounds = guideArea.reduced(16);
  guideBounds.removeFromTop(38);
  instructions.setBounds(guideBounds.withTrimmedBottom(86));
  auto guideActions = guideBounds.removeFromBottom(80);
  copyDevice.setBounds(guideActions.removeFromTop(36).reduced(2));
  windowsSound.setBounds(
      guideActions.removeFromLeft(guideActions.getWidth() / 2).reduced(2));
  guide.setBounds(guideActions.reduced(2));
  auto status = statusArea.reduced(16);
  status.removeFromTop(38);
  inputMeterLabel.setBounds(status.removeFromTop(20));
  inputMeter.setBounds(status.removeFromTop(36));
  outputMeterLabel.setBounds(status.removeFromTop(20));
  outputMeter.setBounds(status.removeFromTop(36));
  status.removeFromTop(8);
  diagnosticTitle.setBounds(status.removeFromTop(28));
  diagnosticDetails.setBounds(status.removeFromTop(48));
}
bool IntegrationsPage::runSmokeTest() {
  refreshDevices();
  juce::String report = "Dispositivos detectados em " +
                        juce::Time::getCurrentTime().toString(true, true) + "\n";
  for (auto &d : detected)
    report << (d.input ? "ENTRADA | " : "SAIDA   | ")
           << (d.virtualDevice ? "VIRTUAL | " : "FISICO  | ") << d.type
           << " | " << d.name << "\n";
  AppPaths::logs().getChildFile("integration-detection.log").replaceWithText(report);
  selectTarget("Discord");
  selectTarget("FiveM");
  selectTarget("Jogos");
  selectTarget("OBS");
  selectTarget("Chamadas");
  selectTarget("Manual");
  auto profiles = profileManager.load();
  for (auto &d : detected)
    if (d.name.isEmpty() || d.type.isEmpty())
      return false;
  auto&manager=engine.deviceManager();const auto originalType=manager.getCurrentAudioDeviceType();const auto originalSetup=manager.getAudioDeviceSetup();DetectedAudioDevice physical,virtualOutput;for(auto&d:detected)if(d.type==originalType&&d.input&&!d.virtualDevice&&physical.name.isEmpty())physical=d;else if(d.type==originalType&&!d.input&&d.virtualDevice&&virtualOutput.name.isEmpty())virtualOutput=d;if(!virtualOutput.name.isEmpty()&&!physical.name.isEmpty()){IntegrationProfile route;route.name="Teste temporário";route.audioType=originalType;route.inputDevice=physical.name;route.virtualOutput=virtualOutput.name;route.sampleRate=48000;route.bufferSize=256;auto error=routing.apply(route);auto applied=manager.getAudioDeviceSetup();const bool valid=error.isEmpty()&&applied.inputDeviceName==physical.name&&applied.outputDeviceName==virtualOutput.name;manager.setCurrentAudioDeviceType(originalType,true);manager.setAudioDeviceSetup(originalSetup,true);if(!valid)return false;}
  setBounds(0, 0, 900, 600);
  resized();
  return inputBox.getNumItems() >= 0 && profileManager.save(profiles);
}
} // namespace vox
