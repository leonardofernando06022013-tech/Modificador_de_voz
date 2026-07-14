#include "AdminTabs.h"
#include "App/AppPaths.h"
#include "UI/Theme.h"

namespace vox {

// ─── AdminPermissionsTab ─────────────────────────────────────────────────────

AdminPermissionsTab::AdminPermissionsTab(AdminSessionManager &s) : session(s) {
    heading.setText(juce::String::fromUTF8("Matriz de Permissões"),
                    juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    description.setText(
        juce::String::fromUTF8(
            "As permissões abaixo são aplicadas pelo controlador local ao executar cada ação.\n"
            "Selecione uma função para visualizar suas permissões efetivas."),
        juce::dontSendNotification);
    description.setColour(juce::Label::textColourId, theme::muted);
    description.setFont(juce::Font(juce::FontOptions(11)));
    description.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(description);

    roleSelector.addItem(juce::String::fromUTF8("Usuário"),  1);
    roleSelector.addItem("Moderador",          2);
    roleSelector.addItem("Administrador",      3);
    roleSelector.addItem("Superadministrador", 4);
    roleSelector.setSelectedId(1);
    roleSelector.onChange = [this] {
        auto role = (Role)juce::jlimit(0, 3, roleSelector.getSelectedId() - 1);
        for (int i = 0; i < checks.size(); ++i) {
            checks[i]->setToggleState(
                permissions.allowed(role, (Permission)i),
                juce::dontSendNotification);
        }
    };
    addAndMakeVisible(roleSelector);

    for (int i = 0; i < 10; ++i) {
        auto *t = checks.add(new juce::ToggleButton(
            PermissionManager::name((Permission)i)));
        t->setEnabled(false); // Somente visualização
        t->setColour(juce::ToggleButton::textColourId, theme::text);
        addAndMakeVisible(t);
    }
    roleSelector.onChange();
}

void AdminPermissionsTab::paint(juce::Graphics &g) {
    auto info = getLocalBounds().reduced(4, 0).removeFromBottom(
        getHeight() - 140).reduced(0, 4).toFloat();
    theme::roundedPanel(g, info, 10, theme::elevated);

    g.setColour(theme::muted);
    g.setFont(juce::Font(juce::FontOptions(10)));
    auto note = juce::Rectangle<float>(
        info.getX() + 12, info.getBottom() - 28,
        info.getWidth() - 24, 20);
    g.drawText(
        juce::String::fromUTF8(
            "Controle local. Não representa segurança de servidor."),
        note, juce::Justification::centredLeft);
}

void AdminPermissionsTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    description.setBounds(a.removeFromTop(40));
    a.removeFromTop(6);
    roleSelector.setBounds(a.removeFromTop(36).removeFromLeft(220).reduced(2));
    a.removeFromTop(10);
    for (auto *t : checks)
        t->setBounds(a.removeFromTop(28).reduced(8, 2));
}

// ─── AdminPresetsTab ──────────────────────────────────────────────────────────

AdminPresetsTab::AdminPresetsTab(AdminSessionManager &s, PresetManager &p)
    : session(s), presets(p)
{
    heading.setText("Presets", juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    presetList.setMultiLine(true);
    presetList.setReadOnly(true);
    presetList.setScrollbarsShown(true);
    presetList.setColour(juce::TextEditor::backgroundColourId, theme::elevated);
    addAndMakeVisible(presetList);

    refresh();
}

void AdminPresetsTab::refresh() {
    juce::String text;
    for (auto &name : presets.names())
        text += juce::String::fromUTF8("●  ") + name +
                juce::String::fromUTF8("  |  Local  |  Disponível\n");
    presetList.setText(text.isEmpty()
        ? juce::String::fromUTF8("Nenhum preset encontrado.") : text, false);
}

void AdminPresetsTab::paint(juce::Graphics &) {}

void AdminPresetsTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    a.removeFromTop(8);
    presetList.setBounds(a);
}

// ─── AdminSecurityTab ─────────────────────────────────────────────────────────

AdminSecurityTab::AdminSecurityTab(AdminSessionManager &s) : session(s) {
    heading.setText(juce::String::fromUTF8("Segurança Local"),
                    juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    const char *secItems[] = {
        "Expira\xC3\xa7\xC3\xa3o da sess\xC3\xa3o: 30 minutos",
        "Aviso de expira\xC3\xa7\xC3\xa3o: 3 minutos antes",
        "Confirma\xC3\xa7\xC3\xa3o para a\xC3\xa7\xC3\xb5\x65s cr\xC3\xadticas: Ativa",
        "Auditoria obrigat\xC3\xb3ria: Ativa",
        "Identidade: conta atual do Windows",
        "Nenhuma senha ou token \xC3\xa9 armazenado",
        "Controle de acesso: local (sem servidor)",
    };
    for (auto *text : secItems) {
        auto *l = items.add(new juce::Label({}, juce::String::fromUTF8(text)));
        l->setColour(juce::Label::textColourId, theme::muted);
        l->setFont(juce::Font(juce::FontOptions(12)));
        addAndMakeVisible(l);
    }

    disclaimer.setText(
        juce::String::fromUTF8(
            "⚠  Este painel é um controle administrativo LOCAL.\n"
            "Não representa segurança de servidor ou autenticação remota.\n"
            "Não existem senhas armazenadas — a identidade é verificada pelo login do Windows."),
        juce::dontSendNotification);
    disclaimer.setColour(juce::Label::textColourId, theme::yellow);
    disclaimer.setFont(juce::Font(juce::FontOptions(11)));
    disclaimer.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(disclaimer);
}

void AdminSecurityTab::paint(juce::Graphics &g) {
    theme::roundedPanel(g,
        getLocalBounds().toFloat().reduced(0, 4).withTrimmedTop(36), 10, theme::elevated);
}

void AdminSecurityTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    a.removeFromTop(8);
    auto panel = a.reduced(12, 8);
    for (auto *l : items)
        l->setBounds(panel.removeFromTop(24));
    panel.removeFromTop(12);
    disclaimer.setBounds(panel.removeFromTop(70));
}

// ─── AdminLogsTab ─────────────────────────────────────────────────────────────

AdminLogsTab::AdminLogsTab(AdminSessionManager &s) : session(s) {
    heading.setText("Logs do Sistema", juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    const juce::StringArray logFiles {
        "startup.log", "audio.log", "errors.log",
        "admin.log", "audit.log", "security.log"
    };
    for (int i = 0; i < logFiles.size(); ++i)
        fileSelector.addItem(logFiles[i], i + 1);
    fileSelector.setSelectedId(1);
    fileSelector.onChange = [this] { loadSelected(); };
    addAndMakeVisible(fileSelector);

    content.setMultiLine(true);
    content.setReadOnly(true);
    content.setScrollbarsShown(true);
    content.setColour(juce::TextEditor::backgroundColourId, theme::elevated);
    content.setFont(juce::Font(juce::FontOptions(10)));
    addAndMakeVisible(content);

    exportBtn.setButtonText(juce::String::fromUTF8("Exportar"));
    exportBtn.setColour(juce::TextButton::buttonColourId, theme::blue);
    exportBtn.onClick = [this] { exportLog(); };
    addAndMakeVisible(exportBtn);

    openFolderBtn.setButtonText(juce::String::fromUTF8("Abrir pasta"));
    openFolderBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    openFolderBtn.onClick = [] { AppPaths::logs().startAsProcess(); };
    addAndMakeVisible(openFolderBtn);

    clearBtn.setButtonText(juce::String::fromUTF8("Limpar logs antigos"));
    clearBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    clearBtn.setColour(juce::TextButton::textColourOffId, theme::red);
    clearBtn.onClick = [this] { clearOldLogs(); };
    addAndMakeVisible(clearBtn);

    refresh();
}

void AdminLogsTab::refresh() { loadSelected(); }

void AdminLogsTab::loadSelected() {
    if (!session.can(Permission::ViewLogs)) {
        content.setText("Acesso negado.", false);
        return;
    }
    auto logName = fileSelector.getText();
    auto logFile = AppPaths::logs().getChildFile(logName);
    if (!logFile.existsAsFile()) {
        content.setText(juce::String::fromUTF8("Arquivo não encontrado: ") + logName, false);
        return;
    }
    // Leitura parcial para não travar (últimas 500 linhas)
    auto all = juce::StringArray::fromLines(logFile.loadFileAsString());
    const int maxLines = 500;
    while (all.size() > maxLines) all.remove(0);
    content.setText(all.joinIntoString("\n"), false);
    content.moveCaretToEnd();
}

void AdminLogsTab::exportLog() {
    if (!session.can(Permission::ExportLogs)) {
        if (notify) notify("Acesso negado");
        return;
    }
    auto logName = fileSelector.getText();
    auto src = AppPaths::logs().getChildFile(logName);
    if (!src.existsAsFile()) { if (notify) notify(juce::String::fromUTF8("Arquivo não encontrado")); return; }
    auto dst = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile(logName + "-export-" +
                                 juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + ".log");
    src.copyFileTo(dst);
    if (notify) notify(juce::String::fromUTF8("Exportado: ") + dst.getFileName());
}

void AdminLogsTab::clearOldLogs() {
    if (!session.can(Permission::DeleteLogs)) { if (notify) notify("Acesso negado"); return; }
    auto files = AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log");
    const auto limit = juce::Time::getCurrentTime() - juce::RelativeTime::days(30);
    int count = 0;
    for (auto &f : files)
        if (f.getLastModificationTime() < limit) ++count;

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        juce::String::fromUTF8("Limpar logs antigos"),
        juce::String(count) + juce::String::fromUTF8(" arquivo(s) com mais de 30 dias serão removidos."),
        "Remover", "Cancelar", nullptr,
        juce::ModalCallbackFunction::create([this, limit](int result) {
            if (!result) return;
            int removed = 0;
            for (auto &f : AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log"))
                if (f.getLastModificationTime() < limit && f.deleteFile()) ++removed;
            if (notify) notify(juce::String(removed) + juce::String::fromUTF8(" logs removidos"));
            refresh();
        }));
}

void AdminLogsTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    a.removeFromTop(6);
    auto toolbar = a.removeFromTop(36);
    fileSelector.setBounds(toolbar.removeFromLeft(200).reduced(2));
    exportBtn.setBounds(toolbar.removeFromLeft(100).reduced(2));
    openFolderBtn.setBounds(toolbar.removeFromLeft(120).reduced(2));
    clearBtn.setBounds(toolbar.reduced(2));
    a.removeFromTop(6);
    content.setBounds(a);
}

// ─── AdminBackupsTab ──────────────────────────────────────────────────────────

AdminBackupsTab::AdminBackupsTab(AdminSessionManager &s, BackupManager &b)
    : session(s), backups(b)
{
    heading.setText("Backups", juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    list.setMultiLine(true);
    list.setReadOnly(true);
    list.setScrollbarsShown(true);
    list.setColour(juce::TextEditor::backgroundColourId, theme::elevated);
    addAndMakeVisible(list);

    createBtn.setButtonText("Criar backup");
    createBtn.setColour(juce::TextButton::buttonColourId, theme::blue);
    createBtn.onClick = [this] {
        if (!session.can(Permission::CreateBackup)) {
            if (notify) notify("Acesso negado");
            return;
        }
        auto f = backups.create(juce::String::fromUTF8("Backup manual"));
        if (notify) notify(f.exists()
            ? juce::String::fromUTF8("Backup criado: ") + f.getFileName()
            : juce::String::fromUTF8("Falha ao criar backup"));
        refresh();
    };
    addAndMakeVisible(createBtn);

    openFolderBtn.setButtonText(juce::String::fromUTF8("Abrir pasta"));
    openFolderBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    openFolderBtn.onClick = [this] { backups.directory().startAsProcess(); };
    addAndMakeVisible(openFolderBtn);

    refresh();
}

void AdminBackupsTab::refresh() {
    backupList = backups.list();
    juce::String text;
    for (auto &f : backupList) {
        text += f.getFileName() + "  |  " +
                f.getLastModificationTime().toString(true, false) + "  |  " +
                juce::File::descriptionOfSizeInBytes(
                    f.findChildFiles(juce::File::findFiles, true).size() > 0
                    ? 0 : f.getSize()) + "\n";
    }
    list.setText(text.isEmpty()
        ? juce::String::fromUTF8("Nenhum backup encontrado.") : text, false);
}

void AdminBackupsTab::paint(juce::Graphics &) {}

void AdminBackupsTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    a.removeFromTop(6);
    auto toolbar = a.removeFromTop(36);
    createBtn.setBounds(toolbar.removeFromLeft(140).reduced(2));
    openFolderBtn.setBounds(toolbar.removeFromLeft(140).reduced(2));
    a.removeFromTop(6);
    list.setBounds(a);
}

// ─── AdminSettingsTab ─────────────────────────────────────────────────────────

AdminSettingsTab::AdminSettingsTab(AdminSessionManager &s) : session(s) {
    heading.setText(
        juce::String::fromUTF8("Configurações Administrativas"),
        juce::dontSendNotification);
    heading.setColour(juce::Label::textColourId, theme::text);
    heading.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    addAndMakeVisible(heading);

    const char *settingItems[] = {
        "Reten\xC3\xa7\xC3\xa3o de logs: 30 dias",
        "Auditoria obrigat\xC3\xb3ria: Ativa",
        "Backup autom\xC3\xa1tico: Desativado",
        "Sess\xC3\xa3o: 30 minutos",
        "Aprova\xC3\xa7\xC3\xa3o de presets: Requerida",
        "Notifica\xC3\xa7\xC3\xb5\x65s de seguran\xC3\xa7\xC3\xa1: Ativas",
    };
    for (auto *text : settingItems) {
        auto *l = items.add(new juce::Label({}, juce::String::fromUTF8(text)));
        l->setColour(juce::Label::textColourId, theme::muted);
        l->setFont(juce::Font(juce::FontOptions(12)));
        addAndMakeVisible(l);
    }

    disclaimer.setText(
        juce::String::fromUTF8(
            "As configurações abaixo afetam o comportamento local do painel administrativo.\n"
            "Somente usuários com permissão adequada podem alterá-las."),
        juce::dontSendNotification);
    disclaimer.setColour(juce::Label::textColourId, theme::muted);
    disclaimer.setFont(juce::Font(juce::FontOptions(11)));
    disclaimer.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(disclaimer);
}

void AdminSettingsTab::paint(juce::Graphics &g) {
    theme::roundedPanel(g,
        getLocalBounds().toFloat().reduced(0, 4).withTrimmedTop(36),
        10, theme::elevated);
}

void AdminSettingsTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);
    heading.setBounds(a.removeFromTop(28));
    a.removeFromTop(8);
    disclaimer.setBounds(a.removeFromTop(40));
    a.removeFromTop(8);
    auto panel = a.reduced(12, 8);
    for (auto *l : items)
        l->setBounds(panel.removeFromTop(26));
}

} // namespace vox
