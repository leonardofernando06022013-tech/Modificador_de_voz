#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Admin/AdminSessionManager.h"
#include "Admin/AdminAccessController.h"
#include "Admin/AuditLogManager.h"
#include "Admin/BackupManager.h"
#include "Admin/UserManager.h"
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/Navigation/PageRouter.h"

#include "UI/Admin/AdminActivityFeed.h"
#include "UI/Admin/AdminExpiredSessionView.h"
#include "UI/Admin/AdminOverviewTab.h"
#include "UI/Admin/AdminSessionPanel.h"
#include "UI/Admin/AdminStatsCard.h"
#include "UI/Admin/AdminSystemInfo.h"
#include "UI/Admin/AdminTabs.h"
#include "UI/Admin/AdminUsersTab.h"

namespace vox {

class AdminPage final : public juce::Component, private juce::Timer {
public:
    AdminPage(AudioEngine &, SettingsManager &, PresetManager &);
    ~AdminPage() override = default;

    void paint(juce::Graphics &) override;
    void resized() override;

    // Estado de acesso (para ocultar item de menu quando sem acesso)
    bool hasAccess() const { return session.isActive(); }

    // Callbacks para o exterior
    std::function<void(PageId)>           navigate;
    std::function<void(const juce::String &)> notify;

    // Smoke test de regressão
    bool runSmokeTest();

private:
    enum class Tab {
        Overview = 0, Users, Permissions, Presets,
        Security, Logs, Backups, Settings
    };

    void timerCallback() override;
    void switchTab(Tab t);
    void refreshAll();
    void onSessionStateChanged(AdminSessionState newState);
    void showExpiredView(bool show);
    void createBackup();
    void exportReport();
    void record(const juce::String &action, const juce::String &target,
                const juce::String &result, const juce::String &severity = "INFO");

    // ── Core ──────────────────────────────────────────────────────────────
    AudioEngine    &engine;
    SettingsManager &settings;
    PresetManager  &presets;
    UserManager     users;
    BackupManager   backups;
    AuditLogManager audit;
    AdminSessionManager session;

    Tab activeTab = Tab::Overview;

    // ── Header ────────────────────────────────────────────────────────────
    juce::Label titleLabel, subtitleLabel;
    juce::TextButton refreshButton, helpButton;

    // ── Abas ──────────────────────────────────────────────────────────────
    juce::OwnedArray<juce::TextButton> tabButtons;

    // ── Conteúdo principal ────────────────────────────────────────────────
    std::unique_ptr<AdminOverviewTab>    overviewTab;
    std::unique_ptr<AdminUsersTab>       usersTab;
    std::unique_ptr<AdminPermissionsTab> permissionsTab;
    std::unique_ptr<AdminPresetsTab>     presetsTab;
    std::unique_ptr<AdminSecurityTab>    securityTab;
    std::unique_ptr<AdminLogsTab>        logsTab;
    std::unique_ptr<AdminBackupsTab>     backupsTab;
    std::unique_ptr<AdminSettingsTab>    settingsTab;

    // ── Painel direito ────────────────────────────────────────────────────
    std::unique_ptr<AdminSessionPanel>   sessionPanel;
    std::unique_ptr<AdminSystemInfo>     systemInfo;
    std::unique_ptr<AdminStoragePanel>   storagePanel;
    std::unique_ptr<AdminHealthPanel>    healthPanel;
    juce::TextButton backupBtn, exportBtn, openLogsBtn, clearLogsBtn;

    // ── Tela de sessão expirada ───────────────────────────────────────────
    std::unique_ptr<AdminExpiredSessionView> expiredView;

    // ── Layout ────────────────────────────────────────────────────────────
    juce::Rectangle<int> headerArea, tabsArea, mainArea, rightArea;
};

} // namespace vox
