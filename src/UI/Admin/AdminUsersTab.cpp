#include "AdminUsersTab.h"
#include "UI/Theme.h"

namespace vox {

AdminUsersTab::AdminUsersTab(AdminSessionManager &s, UserManager &u)
    : session(s), users(u)
{
    search.setTextToShowWhenEmpty(
        juce::String::fromUTF8("Buscar por nome, e-mail ou ID…"), theme::muted);
    search.setColour(juce::TextEditor::backgroundColourId, theme::elevated);
    search.onTextChange = [this] { applyFilter(); };
    addAndMakeVisible(search);

    roleFilter.addItem(juce::String::fromUTF8("Todas as funções"), 1);
    roleFilter.addItem(juce::String::fromUTF8("Usuário"),  2);
    roleFilter.addItem("Moderador",          3);
    roleFilter.addItem("Administrador",      4);
    roleFilter.addItem("Superadministrador", 5);
    roleFilter.setSelectedId(1);
    roleFilter.onChange = [this] { applyFilter(); };
    addAndMakeVisible(roleFilter);

    statusFilter.addItem("Todos os status", 1);
    statusFilter.addItem("Ativo",   2);
    statusFilter.addItem("Inativo", 3);
    statusFilter.addItem("Bloqueado", 4);
    statusFilter.addItem("Pendente",  5);
    statusFilter.setSelectedId(1);
    statusFilter.onChange = [this] { applyFilter(); };
    addAndMakeVisible(statusFilter);

    addButton.setButtonText(juce::String::fromUTF8("+ Adicionar usuário"));
    addButton.setColour(juce::TextButton::buttonColourId, theme::blue);
    addButton.setColour(juce::TextButton::textColourOffId, theme::text);
    addButton.onClick = [this] {
        if (!session.can(Permission::ManageUsers)) {
            if (notify) notify("Acesso negado");
            return;
        }
        showUserEditor(UserAccount{});
    };
    addAndMakeVisible(addButton);

    list.setRowHeight(60);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(list);

    // Editor inline
    editorHeading.setColour(juce::Label::textColourId, theme::text);
    editorHeading.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    addChildComponent(editorHeading);

    nameField.setTextToShowWhenEmpty("Nome", theme::muted);
    emailField.setTextToShowWhenEmpty(juce::String::fromUTF8("E-mail (opcional)"), theme::muted);
    for (auto *e : { &nameField, &emailField }) {
        e->setColour(juce::TextEditor::backgroundColourId, theme::elevated);
        addChildComponent(e);
    }

    roleCombo.addItem(juce::String::fromUTF8("Usuário"), 1);
    roleCombo.addItem("Moderador",          2);
    roleCombo.addItem("Administrador",      3);
    roleCombo.addItem("Superadministrador", 4);
    addChildComponent(roleCombo);

    statusCombo.addItem("Ativo",    1);
    statusCombo.addItem("Inativo",  2);
    statusCombo.addItem("Bloqueado",3);
    statusCombo.addItem("Pendente", 4);
    addChildComponent(statusCombo);

    saveBtn.setButtonText("Salvar");
    saveBtn.setColour(juce::TextButton::buttonColourId, theme::blue);
    saveBtn.onClick = [this] { saveUser(editingUser); };
    addChildComponent(saveBtn);

    blockBtn.setButtonText("Bloquear");
    blockBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    blockBtn.setColour(juce::TextButton::textColourOffId, theme::yellow);
    blockBtn.onClick = [this] { blockUser(editingUser); };
    addChildComponent(blockBtn);

    removeBtn.setButtonText("Remover");
    removeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    removeBtn.setColour(juce::TextButton::textColourOffId, theme::red);
    removeBtn.onClick = [this] { removeUser(editingUser); };
    addChildComponent(removeBtn);

    cancelBtn.setButtonText("Cancelar");
    cancelBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    cancelBtn.onClick = [this] {
        editorVisible = false;
        juce::Component* editorWidgets[] = {
            (juce::Component*)&editorHeading, &nameField,
            &emailField, &roleCombo, &statusCombo,
            &saveBtn, &blockBtn, &removeBtn, &cancelBtn
        };
        for (auto *c : editorWidgets)
            c->setVisible(false);
        resized();
    };
    addChildComponent(cancelBtn);

    refresh();
}

void AdminUsersTab::refresh() {
    all = users.users();
    applyFilter();
}

void AdminUsersTab::applyFilter() {
    visible.clear();
    const auto q   = search.getText().trim();
    const int  rId = roleFilter.getSelectedId();
    const int  sId = statusFilter.getSelectedId();
    for (auto &u : all) {
        if (q.isNotEmpty() && !u.name.containsIgnoreCase(q) &&
            !u.email.containsIgnoreCase(q) && !u.id.containsIgnoreCase(q))
            continue;
        if (rId > 1 && (int)u.role != rId - 2)   continue;
        if (sId > 1 && (int)u.status != sId - 2) continue;
        visible.add(u);
    }
    list.updateContent();
    list.repaint();
}

void AdminUsersTab::paintListBoxItem(int row, juce::Graphics &g,
                                     int width, int height, bool sel) {
    if (!juce::isPositiveAndBelow(row, visible.size())) return;
    const auto &u = visible.getReference(row);

    auto bounds = juce::Rectangle<int>(0, 2, width, height - 4).reduced(4);
    g.setColour(sel ? theme::elevated.brighter(0.1f) : theme::elevated);
    g.fillRoundedRectangle(bounds.toFloat(), 8);

    // Avatar circular
    auto av = bounds.removeFromLeft(44).withSizeKeepingCentre(36, 36);
    const juce::Colour roleColour =
        u.role == Role::SuperAdministrator ? theme::purple :
        u.role == Role::Administrator      ? theme::blue   :
        u.role == Role::Moderator          ? theme::cyan   : theme::muted;
    g.setColour(roleColour.withAlpha(0.15f));
    g.fillEllipse(av.toFloat());
    g.setColour(roleColour);
    g.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
    g.drawText(u.name.substring(0, 1).toUpperCase(), av, juce::Justification::centred);

    bounds.removeFromLeft(4);

    // Status badge (direita)
    auto statusBadge = bounds.removeFromRight(70);
    const juce::Colour statusColour =
        u.status == UserStatus::Active   ? theme::green  :
        u.status == UserStatus::Blocked  ? theme::red    :
        u.status == UserStatus::Pending  ? theme::yellow : theme::muted;
    g.setColour(statusColour.withAlpha(0.15f));
    g.fillRoundedRectangle(statusBadge.withSizeKeepingCentre(62, 20).toFloat(), 8);
    g.setColour(statusColour);
    g.setFont(juce::Font(juce::FontOptions(10, juce::Font::bold)));
    g.drawText(statusName(u.status), statusBadge, juce::Justification::centred);

    // Nome + função
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    g.drawText(u.name, bounds.removeFromTop(22), juce::Justification::centredLeft);
    g.setColour(theme::muted);
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText(roleName(u.role) +
               (u.email.isNotEmpty() ? "  ·  " + u.email : juce::String{}),
               bounds.removeFromTop(16), juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(10)));
    g.drawText(juce::String::fromUTF8("Último acesso: ") +
               (u.lastAccess.toMilliseconds() > 0 ?
                u.lastAccess.toString(true, false) :
                juce::String::fromUTF8("Nunca")),
               bounds, juce::Justification::centredLeft);
}

void AdminUsersTab::listBoxItemClicked(int row, const juce::MouseEvent &) {
    if (!juce::isPositiveAndBelow(row, visible.size())) return;
    showUserEditor(visible.getReference(row));
}

void AdminUsersTab::showUserEditor(const UserAccount &u) {
    if (!session.can(Permission::ManageUsers)) {
        if (notify) notify("Acesso negado");
        return;
    }
    editingUser = u;
    const bool isNew = u.id.isEmpty();
    editorHeading.setText(isNew ? juce::String::fromUTF8("Novo usuário")
                                : juce::String::fromUTF8("Editar: ") + u.name,
                          juce::dontSendNotification);
    nameField.setText(u.name);
    emailField.setText(u.email);
    roleCombo.setSelectedId((int)u.role + 1);
    statusCombo.setSelectedId((int)u.status + 1);

    editorVisible = true;
    const bool canRemove = !isNew && u.id != session.user().id;
    juce::Component* showWidgets[] = {
        (juce::Component*)&editorHeading, &nameField,
        &emailField, &roleCombo, &statusCombo,
        &saveBtn, &cancelBtn
    };
    for (auto *c : showWidgets)
        c->setVisible(true);
    blockBtn.setVisible(!isNew);
    removeBtn.setVisible(canRemove);
    resized();
}

void AdminUsersTab::saveUser(UserAccount u) {
    if (!session.can(Permission::ManageUsers)) {
        if (notify) notify("Acesso negado");
        return;
    }
    if (nameField.getText().trim().isEmpty()) {
        if (notify) notify(juce::String::fromUTF8("Nome é obrigatório"));
        return;
    }
    const bool isNew = u.id.isEmpty();
    if (isNew) {
        u.id      = juce::Uuid().toString();
        u.created = juce::Time::getCurrentTime();
    }
    u.name        = nameField.getText().trim();
    u.email       = emailField.getText().trim();
    u.role        = (Role)juce::jlimit(0, 3, roleCombo.getSelectedId() - 1);
    u.status      = (UserStatus)juce::jlimit(0, 3, statusCombo.getSelectedId() - 1);
    u.lastAccess  = juce::Time::getCurrentTime();
    users.upsert(u);
    if (notify) notify(isNew ? juce::String::fromUTF8("Usuário criado")
                             : juce::String::fromUTF8("Usuário salvo"));
    cancelBtn.triggerClick();
    refresh();
}

void AdminUsersTab::blockUser(UserAccount u) {
    if (!session.can(Permission::ManageUsers)) return;
    if (u.id == session.user().id) {
        if (notify) notify(juce::String::fromUTF8("Não é possível bloquear a conta atual"));
        return;
    }
    u.status = u.status == UserStatus::Blocked ? UserStatus::Active : UserStatus::Blocked;
    users.upsert(u);
    if (notify) notify(juce::String::fromUTF8("Status alterado: ") + statusName(u.status));
    cancelBtn.triggerClick();
    refresh();
}

void AdminUsersTab::removeUser(const UserAccount &u) {
    if (!session.can(Permission::ManageUsers)) return;
    if (u.id == session.user().id) return;
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        juce::String::fromUTF8("Confirmar remoção"),
        juce::String::fromUTF8("Remover o usuário \"") + u.name + "\"?",
        "Remover", "Cancelar", nullptr,
        juce::ModalCallbackFunction::create([this, id = u.id](int result) {
            if (!result) return;
            users.remove(id);
            if (notify) notify(juce::String::fromUTF8("Usuário removido"));
            cancelBtn.triggerClick();
            refresh();
        }));
}

void AdminUsersTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);

    // Toolbar
    auto toolbar = a.removeFromTop(42);
    search.setBounds(toolbar.removeFromLeft(toolbar.getWidth() / 2).reduced(2));
    roleFilter.setBounds(toolbar.removeFromLeft(toolbar.getWidth() / 2).reduced(2));
    statusFilter.setBounds(toolbar.reduced(2));
    a.removeFromTop(4);

    addButton.setBounds(a.removeFromTop(34).removeFromRight(180).reduced(2));
    a.removeFromTop(4);

    if (editorVisible) {
        // Editor no fundo
        auto editorArea = a.removeFromBottom(180).reduced(0, 4);
        editorHeading.setBounds(editorArea.removeFromTop(24));
        editorArea.removeFromTop(4);
        auto row1 = editorArea.removeFromTop(34);
        nameField.setBounds(row1.removeFromLeft(row1.getWidth() / 2).reduced(2));
        emailField.setBounds(row1.reduced(2));
        editorArea.removeFromTop(4);
        auto row2 = editorArea.removeFromTop(34);
        roleCombo.setBounds(row2.removeFromLeft(row2.getWidth() / 2).reduced(2));
        statusCombo.setBounds(row2.reduced(2));
        editorArea.removeFromTop(4);
        auto row3 = editorArea.removeFromTop(36);
        saveBtn.setBounds(row3.removeFromLeft(100).reduced(2));
        blockBtn.setBounds(row3.removeFromLeft(100).reduced(2));
        removeBtn.setBounds(row3.removeFromLeft(100).reduced(2));
        cancelBtn.setBounds(row3.removeFromLeft(100).reduced(2));
    }

    list.setBounds(a);
}

} // namespace vox
