#include "AdminPage.h"
#include "App/AppPaths.h"
#include "UI/Theme.h"

namespace vox {

static void sLabel(juce::Label &l, float sz, juce::Colour c, bool bold = false) {
    l.setColour(juce::Label::textColourId, c);
    l.setFont(juce::Font(juce::FontOptions(sz, bold ? juce::Font::bold : juce::Font::plain)));
}

// ─────────────────────────────────────────────────────────────────────────────
AdminPage::AdminPage(AudioEngine &e, SettingsManager &s, PresetManager &p)
    : engine(e), settings(s), presets(p)
{
    // ── Sessão ─────────────────────────────────────────────────────────────
    session.setStateCallback([this](AdminSessionState st) {
        juce::MessageManager::callAsync([this, st] { onSessionStateChanged(st); });
    });
    session.begin(30);

    // ── Header ─────────────────────────────────────────────────────────────
    sLabel(titleLabel,    22, theme::text, true);
    sLabel(subtitleLabel, 11, theme::muted);
    titleLabel.setText(
        juce::String::fromUTF8("Painel de Administração"),
        juce::dontSendNotification);
    subtitleLabel.setText(
        juce::String::fromUTF8(
            "Gerencie usuários, permissões, segurança, presets, backups e registros do sistema."),
        juce::dontSendNotification);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(subtitleLabel);

    refreshButton.setButtonText(juce::String::fromUTF8("↻  Atualizar"));
    refreshButton.setColour(juce::TextButton::buttonColourId, theme::elevated);
    refreshButton.setColour(juce::TextButton::textColourOffId, theme::cyan);
    refreshButton.onClick = [this] {
        refreshAll();
        if (notify) notify(juce::String::fromUTF8("Painel atualizado."));
    };
    addAndMakeVisible(refreshButton);

    helpButton.setButtonText("?");
    helpButton.setColour(juce::TextButton::buttonColourId, theme::elevated);
    helpButton.setColour(juce::TextButton::textColourOffId, theme::muted);
    helpButton.onClick = [this] {
        if (notify) notify(
            juce::String::fromUTF8(
                "Controle administrativo local. Não representa segurança de servidor."));
    };
    addAndMakeVisible(helpButton);

    // ── Abas ───────────────────────────────────────────────────────────────
    const juce::StringArray tabNames {
        juce::String::fromUTF8("Visão Geral"),
        juce::String::fromUTF8("Usuários"),
        juce::String::fromUTF8("Permissões"),
        "Presets",
        juce::String::fromUTF8("Segurança"),
        "Logs",
        "Backups",
        juce::String::fromUTF8("Configurações")
    };
    for (int i = 0; i < tabNames.size(); ++i) {
        auto *b = tabButtons.add(new juce::TextButton(tabNames[i]));
        b->setColour(juce::TextButton::buttonColourId,
                     i == 0 ? theme::purple : juce::Colours::transparentBlack);
        b->setColour(juce::TextButton::textColourOffId,
                     i == 0 ? theme::text : theme::muted);
        b->onClick = [this, i] { switchTab((Tab)i); };
        addAndMakeVisible(b);
    }

    // ── Abas de conteúdo ────────────────────────────────────────────────────
    overviewTab    = std::make_unique<AdminOverviewTab>(session, users, presets, audit);
    usersTab       = std::make_unique<AdminUsersTab>(session, users);
    permissionsTab = std::make_unique<AdminPermissionsTab>(session);
    presetsTab     = std::make_unique<AdminPresetsTab>(session, presets);
    securityTab    = std::make_unique<AdminSecurityTab>(session);
    logsTab        = std::make_unique<AdminLogsTab>(session);
    backupsTab     = std::make_unique<AdminBackupsTab>(session, backups);
    settingsTab    = std::make_unique<AdminSettingsTab>(session);

    overviewTab->onNavigateTab = [this](int t) { switchTab((Tab)t); };

    // (notify wired below)

    auto wireNotify = [this](auto &tab) {
        tab->notify = [this](const juce::String &msg) {
            if (notify) notify(msg);
        };
    };
    wireNotify(usersTab);
    wireNotify(presetsTab);
    wireNotify(logsTab);
    wireNotify(backupsTab);

    juce::Component* allTabs[] = {
        overviewTab.get(), usersTab.get(), permissionsTab.get(), presetsTab.get(),
        securityTab.get(), logsTab.get(), backupsTab.get(), settingsTab.get()
    };
    for (auto *tab : allTabs)
        addChildComponent(tab);

    // ── Painel direito ──────────────────────────────────────────────────────
    sessionPanel = std::make_unique<AdminSessionPanel>(session);
    systemInfo   = std::make_unique<AdminSystemInfo>(engine);
    storagePanel = std::make_unique<AdminStoragePanel>();
    healthPanel  = std::make_unique<AdminHealthPanel>();

    sessionPanel->onRenew = [this] {
        session.renew();
        sessionPanel->refresh();
        showExpiredView(false);
        if (notify) notify(juce::String::fromUTF8("Sessão renovada."));
    };
    sessionPanel->onLock = [this] {
        session.lock();
        sessionPanel->refresh();
        showExpiredView(true);
        if (notify) notify(juce::String::fromUTF8("Sessão bloqueada."));
    };
    sessionPanel->onEnd = [this] {
        session.end();
        sessionPanel->refresh();
        showExpiredView(true);
        if (notify) notify(juce::String::fromUTF8("Sessão encerrada."));
    };

    addAndMakeVisible(*sessionPanel);
    addAndMakeVisible(*systemInfo);
    addAndMakeVisible(*storagePanel);
    addAndMakeVisible(*healthPanel);

    // Botões do painel direito
    backupBtn.setButtonText(juce::String::fromUTF8("Criar backup"));
    backupBtn.setColour(juce::TextButton::buttonColourId, theme::blue);
    backupBtn.setColour(juce::TextButton::textColourOffId, theme::text);
    backupBtn.onClick = [this] { createBackup(); };

    exportBtn.setButtonText(juce::String::fromUTF8("Exportar relatório"));
    exportBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    exportBtn.setColour(juce::TextButton::textColourOffId, theme::text);
    exportBtn.onClick = [this] { exportReport(); };

    openLogsBtn.setButtonText(juce::String::fromUTF8("Abrir pasta de logs"));
    openLogsBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    openLogsBtn.setColour(juce::TextButton::textColourOffId, theme::muted);
    openLogsBtn.onClick = [] { AppPaths::logs().startAsProcess(); };

    clearLogsBtn.setButtonText(juce::String::fromUTF8("Limpar logs antigos"));
    clearLogsBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    clearLogsBtn.setColour(juce::TextButton::textColourOffId, theme::red);
    clearLogsBtn.onClick = [this] {
        if (!session.can(Permission::DeleteLogs)) {
            if (notify) notify("Acesso negado");
            return;
        }
        auto files = AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log");
        const auto limit = juce::Time::getCurrentTime() - juce::RelativeTime::days(30);
        int count = 0;
        for (auto &f : files) if (f.getLastModificationTime() < limit) ++count;

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
                record("LOG_DELETE", juce::String(removed), "SUCCESS", "CRITICAL");
                if (notify) notify(juce::String(removed) + juce::String::fromUTF8(" logs removidos"));
            }));
    };

    for (auto *b : { &backupBtn, &exportBtn, &openLogsBtn, &clearLogsBtn })
        addAndMakeVisible(b);

    // ── Tela de sessão expirada ─────────────────────────────────────────────
    expiredView = std::make_unique<AdminExpiredSessionView>(session);
    expiredView->onRenew = [this] {
        session.renew();
        sessionPanel->refresh();
        showExpiredView(false);
        if (notify) notify(juce::String::fromUTF8("Sessão renovada."));
    };
    expiredView->onBack = [this] {
        if (navigate) navigate(PageId::Home);
    };
    expiredView->onEndAdmin = [this] {
        session.end();
        expiredView->refresh();
        if (navigate) navigate(PageId::Home);
    };
    addChildComponent(*expiredView);

    // ── Estado inicial ──────────────────────────────────────────────────────
    if (!session.isActive()) {
        record("ACCESS_DENIED", "Admin", "DENIED", "WARN");
        showExpiredView(true);
    } else {
        showExpiredView(false);
        refreshAll();
    }

    switchTab(Tab::Overview);
    storagePanel->startCalculation();
    healthPanel->runCheck();
    startTimer(1000); // tick da sessão a cada segundo
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminPage::timerCallback() {
    session.tick();
    sessionPanel->refresh();
    systemInfo->refresh();

    // Aviso de expiração em breve
    if (session.state() == AdminSessionState::ExpiringSoon) {
        const auto remaining = session.session().remainingText();
        static juce::String lastWarn;
        if (remaining != lastWarn) {
            lastWarn = remaining;
            if (notify) notify(juce::String::fromUTF8("Sessão administrativa expira em ") +
                               remaining + ".");
        }
    }
}

void AdminPage::onSessionStateChanged(AdminSessionState newState) {
    if (newState == AdminSessionState::Expired ||
        newState == AdminSessionState::Locked  ||
        newState == AdminSessionState::Inactive) {
        showExpiredView(true);
        expiredView->refresh();
    } else {
        showExpiredView(false);
    }
    sessionPanel->refresh();
    resized();
}

void AdminPage::showExpiredView(bool show) {
    expiredView->setVisible(show);
    // Quando expirado, esconder TODO conteúdo protegido
    juce::Component* protectedTabs[] = {
        overviewTab.get(), usersTab.get(), permissionsTab.get(), presetsTab.get(),
        securityTab.get(), logsTab.get(), backupsTab.get(), settingsTab.get()
    };
    for (auto *tab : protectedTabs)
        tab->setVisible(!show);

    for (auto *b : tabButtons)
        b->setEnabled(!show);

    systemInfo->setVisible(!show);
    storagePanel->setVisible(!show);
    healthPanel->setVisible(!show);
    backupBtn.setVisible(!show);
    exportBtn.setVisible(!show);
    openLogsBtn.setVisible(!show);
    clearLogsBtn.setVisible(!show);
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminPage::switchTab(Tab tab) {
    if (!session.isActive()) return;
    activeTab = tab;

    for (int i = 0; i < tabButtons.size(); ++i) {
        const bool sel = i == (int)tab;
        tabButtons[i]->setColour(juce::TextButton::buttonColourId,
                                  sel ? theme::purple : juce::Colours::transparentBlack);
        tabButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  sel ? theme::text : theme::muted);
    }

    const juce::Component *tabs[] = {
        overviewTab.get(), usersTab.get(), permissionsTab.get(), presetsTab.get(),
        securityTab.get(), logsTab.get(), backupsTab.get(), settingsTab.get()
    };
    for (int i = 0; i < 8; ++i)
        const_cast<juce::Component*>(tabs[i])->setVisible(i == (int)tab);

    // Refresh da aba activa
    switch (tab) {
        case Tab::Overview:    overviewTab->refresh();    break;
        case Tab::Users:       usersTab->refresh();       break;
        case Tab::Presets:     presetsTab->refresh();     break;
        case Tab::Logs:        logsTab->refresh();        break;
        case Tab::Backups:     backupsTab->refresh();     break;
        default: break;
    }

    resized();
}

void AdminPage::refreshAll() {
    if (!session.isActive()) return;
    overviewTab->refresh();
    systemInfo->refresh();
    storagePanel->startCalculation();
    healthPanel->runCheck();
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminPage::createBackup() {
    if (!session.can(Permission::CreateBackup)) {
        if (notify) notify("Acesso negado");
        return;
    }
    auto f = backups.create(juce::String::fromUTF8("Backup manual"));
    record("BACKUP_CREATE", f.getFileName(), f.exists() ? "SUCCESS" : "FAILED");
    if (notify) notify(f.exists()
        ? juce::String::fromUTF8("Backup criado: ") + f.getFileName()
        : juce::String::fromUTF8("Falha ao criar backup"));
    refreshAll();
}

void AdminPage::exportReport() {
    if (!session.can(Permission::ExportLogs)) {
        if (notify) notify("Acesso negado");
        return;
    }
    auto out = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile(juce::String::fromUTF8("Relatório-Admin-") +
                                 juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"));
    out.createDirectory();
    for (auto &f : AppPaths::logs().findChildFiles(juce::File::findFiles, false, "*.log"))
        f.copyFileTo(out.getChildFile(f.getFileName()));
    record("LOG_EXPORT", out.getFullPathName(), "SUCCESS");
    if (notify) notify(juce::String::fromUTF8("Relatório exportado: ") + out.getFileName());
}

void AdminPage::record(const juce::String &action, const juce::String &target,
                       const juce::String &result, const juce::String &severity) {
    audit.record(session.user(), action, target, result, severity);
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminPage::paint(juce::Graphics &g) {
    g.fillAll(theme::background);

    // Linha separadora abaixo das abas
    if (!tabsArea.isEmpty()) {
        g.setColour(theme::border);
        g.drawLine((float)tabsArea.getX(), (float)tabsArea.getBottom(),
                   (float)tabsArea.getRight(), (float)tabsArea.getBottom(), 1.0f);
    }

    // Painel direito (fundo)
    if (!rightArea.isEmpty() && session.isActive())
        theme::roundedPanel(g, rightArea.toFloat(), 12, theme::panel);
}

void AdminPage::resized() {
    auto a = getLocalBounds().reduced(12);

    // ── Header ─────────────────────────────────────────────────────────────
    headerArea = a.removeFromTop(64);
    auto headerLeft  = headerArea.removeFromLeft(getWidth() - 280);
    auto headerRight = headerArea;

    titleLabel.setBounds(headerLeft.removeFromTop(32));
    subtitleLabel.setBounds(headerLeft.removeFromTop(20));

    helpButton.setBounds(headerRight.removeFromRight(44).withSizeKeepingCentre(36, 32));
    refreshButton.setBounds(headerRight.removeFromRight(120).reduced(4, 8));

    // ── Abas ───────────────────────────────────────────────────────────────
    tabsArea = a.removeFromTop(44);
    {
        auto row = tabsArea;
        const int tabW = juce::jmax(80, row.getWidth() / tabButtons.size());
        for (auto *b : tabButtons)
            b->setBounds(row.removeFromLeft(tabW).reduced(2, 6));
    }
    a.removeFromTop(8);

    // ── Tela de sessão expirada (cobre tudo) ──────────────────────────────
    expiredView->setBounds(a);

    if (!session.isActive()) return;

    // ── Layout principal + direito ─────────────────────────────────────────
    const int rightW = juce::jmin(300, getWidth() / 4);
    rightArea  = a.removeFromRight(rightW);
    a.removeFromRight(10);
    mainArea   = a;

    // Painel direito
    {
        auto right = rightArea.reduced(8, 6);
        const int panelH = juce::jmin(280, right.getHeight() / 3);
        sessionPanel->setBounds(right.removeFromTop(panelH));
        right.removeFromTop(8);
        const int infoH = 210;
        systemInfo->setBounds(right.removeFromTop(infoH));
        right.removeFromTop(8);
        const int storageH = 140;
        storagePanel->setBounds(right.removeFromTop(storageH));
        right.removeFromTop(8);
        // Botões de ação
        for (auto *b : { &backupBtn, &exportBtn, &openLogsBtn, &clearLogsBtn }) {
            b->setBounds(right.removeFromTop(34).reduced(0, 2));
            right.removeFromTop(2);
        }
        // Health panel no que sobrar
        if (right.getHeight() > 80)
            healthPanel->setBounds(right);
    }

    // ── Conteúdo da aba activa ─────────────────────────────────────────────
    const juce::Component *tabs[] = {
        overviewTab.get(), usersTab.get(), permissionsTab.get(), presetsTab.get(),
        securityTab.get(), logsTab.get(), backupsTab.get(), settingsTab.get()
    };
    for (auto *tab : tabs)
        const_cast<juce::Component*>(tab)->setBounds(mainArea);
}

// ─────────────────────────────────────────────────────────────────────────────
bool AdminPage::runSmokeTest() {
    // 1. Verificar que usuário comum não tem acesso
    AdminSessionManager testSession;
    UserAccount common;
    common.id     = "test-id";
    common.name   = "Test";
    common.role   = Role::User;
    common.status = UserStatus::Active;
    if (testSession.begin() && testSession.can(Permission::ViewLogs))
        return false; // Usuário comum não pode ter ViewLogs via begin()

    // 2. Smoke de redimensionamento nas resoluções-alvo
    const juce::Point<int> sizes[]{ {1100,700},{1280,720},{1440,900},{1920,1080} };
    for (auto sz : sizes) {
        setBounds(0, 0, sz.x, sz.y);
        resized();
        // Verificar que nenhum botão de aba está fora dos limites
        for (auto *b : tabButtons) {
            if (!getLocalBounds().contains(b->getBounds()))
                return false;
        }
    }

    // 3. Verificar troca de todas as abas
    for (int i = 0; i < 8; ++i) {
        switchTab((Tab)i);
        if (activeTab != (Tab)i) return false;
    }

    return true;
}

} // namespace vox
