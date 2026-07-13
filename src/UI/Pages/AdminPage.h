#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Admin/AdminAccessController.h"
#include "Admin/AuditLogManager.h"
#include "Admin/BackupManager.h"
#include "Admin/UserManager.h"
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/Navigation/PageRouter.h"
#include "UI/Admin/AdminActivityFeed.h"

namespace vox {
class AdminPage final : public juce::Component, private juce::Timer {
public:
    AdminPage(AudioEngine &, SettingsManager &, PresetManager &);
    void paint(juce::Graphics &) override;
    void resized() override;
    bool hasAccess() const { return access.active(); }
    bool runSmokeTest();

    std::function<void(PageId)> navigate;
    std::function<void(const juce::String &)> notify;

private:
    enum class Tab { Overview, Users, Permissions, Presets, Security, Logs, Backups, Settings };

    void timerCallback() override;
    void switchTab(Tab);
    void refresh();
    void refreshUsers();
    void saveUser();
    void blockUser();
    void removeUser();
    void createBackup();
    void exportLogs();
    void clearOldLogs();
    void record(const juce::String &action, const juce::String &target,
                const juce::String &result, const juce::String &severity = "INFO");

    AudioEngine &engine;
    SettingsManager &settings;
    PresetManager &presets;
    UserManager users;
    PermissionManager permissions;
    AdminAccessController access;
    AuditLogManager audit;
    BackupManager backups;
    Tab activeTab = Tab::Overview;

    juce::Label title, subtitle, sessionLabel, statusLabel;
    juce::OwnedArray<juce::TextButton> tabButtons;
    juce::TextEditor content, systemInfo, search, userName, userEmail;
    AdminActivityFeed activityFeed;
    juce::ComboBox roleFilter, statusFilter, userList, userRole, userStatus;
    juce::OwnedArray<juce::ToggleButton> permissionChecks;
    juce::TextButton refreshButton{"Atualizar"}, helpButton{"Ajuda"},
        primaryAction{juce::String::fromUTF8("Adicionar usuário")}, saveUserButton{"Salvar"},
        blockUserButton{"Bloquear"}, removeUserButton{"Remover"},
        backupButton{"Criar backup"}, exportButton{juce::String::fromUTF8("Exportar relatório")},
        openLogsButton{"Abrir logs"}, clearLogsButton{"Limpar logs antigos"};
    juce::Rectangle<int> headerArea, tabsArea, mainArea, rightArea, statsArea;
};
} // namespace vox
