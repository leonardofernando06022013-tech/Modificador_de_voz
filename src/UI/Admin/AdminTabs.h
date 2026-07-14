#pragma once
#include "Admin/AdminSessionManager.h"
#include "Admin/BackupManager.h"
#include "Admin/PermissionManager.h"
#include "Presets/PresetManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Aba "Permissões" — matriz visual de permissões por função.
class AdminPermissionsTab final : public juce::Component {
public:
    explicit AdminPermissionsTab(AdminSessionManager &);

    void resized() override;
    void paint(juce::Graphics &) override;

private:
    AdminSessionManager &session;
    PermissionManager    permissions;

    juce::ComboBox roleSelector;
    juce::Label    heading, description;
    juce::OwnedArray<juce::ToggleButton> checks;
};

/// Aba "Presets" — lista de presets com ações.
class AdminPresetsTab final : public juce::Component {
public:
    std::function<void(const juce::String &)> notify;

    AdminPresetsTab(AdminSessionManager &, PresetManager &);

    void refresh();
    void resized() override;
    void paint(juce::Graphics &) override;

private:
    AdminSessionManager &session;
    PresetManager       &presets;
    juce::Label heading;
    juce::TextEditor presetList;
};

/// Aba "Segurança" — configurações de política local.
class AdminSecurityTab final : public juce::Component {
public:
    explicit AdminSecurityTab(AdminSessionManager &);

    void resized() override;
    void paint(juce::Graphics &) override;

private:
    AdminSessionManager &session;
    juce::Label heading, disclaimer;
    juce::OwnedArray<juce::Label> items;
};

/// Aba "Logs" — visualizador de arquivos de log.
class AdminLogsTab final : public juce::Component {
public:
    std::function<void(const juce::String &)> notify;

    explicit AdminLogsTab(AdminSessionManager &);

    void refresh();
    void resized() override;

private:
    AdminSessionManager &session;

    juce::Label      heading;
    juce::ComboBox   fileSelector;
    juce::TextEditor content;
    juce::TextButton exportBtn, openFolderBtn, clearBtn;

    void loadSelected();
    void exportLog();
    void clearOldLogs();
};

/// Aba "Backups" — lista e ações de backup.
class AdminBackupsTab final : public juce::Component {
public:
    std::function<void(const juce::String &)> notify;

    AdminBackupsTab(AdminSessionManager &, BackupManager &);

    void refresh();
    void resized() override;
    void paint(juce::Graphics &) override;

private:
    AdminSessionManager &session;
    BackupManager       &backups;

    juce::Label      heading;
    juce::TextEditor list;
    juce::TextButton createBtn, openFolderBtn;

    juce::Array<juce::File> backupList;
};

/// Aba "Configurações" — configurações administrativas locais.
class AdminSettingsTab final : public juce::Component {
public:
    explicit AdminSettingsTab(AdminSessionManager &);

    void resized() override;
    void paint(juce::Graphics &) override;

private:
    AdminSessionManager &session;
    juce::Label heading, disclaimer;
    juce::OwnedArray<juce::Label> items;
    juce::OwnedArray<juce::ToggleButton> toggles;
};

} // namespace vox
