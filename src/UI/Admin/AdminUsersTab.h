#pragma once
#include "Admin/AdminSessionManager.h"
#include "Admin/UserManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Aba "Usuários" — tabela moderna com busca, filtros e ações por linha.
class AdminUsersTab final : public juce::Component,
                            private juce::ListBoxModel {
public:
    std::function<void(const juce::String &)> notify;

    AdminUsersTab(AdminSessionManager &, UserManager &);

    void refresh();
    void resized() override;

private:
    // ListBoxModel
    int  getNumRows() override { return (int)visible.size(); }
    void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
    void listBoxItemClicked(int, const juce::MouseEvent &) override;

    void applyFilter();
    void showUserEditor(const UserAccount &u);
    void saveUser(UserAccount u);
    void blockUser(UserAccount u);
    void removeUser(const UserAccount &u);

    AdminSessionManager &session;
    UserManager         &users;

    juce::Array<UserAccount> all, visible;

    // Controles de filtro
    juce::TextEditor  search;
    juce::ComboBox    roleFilter, statusFilter;
    juce::TextButton  addButton;

    juce::ListBox list{"users", this};

    // Editor inline
    juce::Label       editorHeading;
    juce::TextEditor  nameField, emailField;
    juce::ComboBox    roleCombo, statusCombo;
    juce::TextButton  saveBtn, blockBtn, removeBtn, cancelBtn;
    bool              editorVisible = false;
    UserAccount       editingUser;
};

} // namespace vox
