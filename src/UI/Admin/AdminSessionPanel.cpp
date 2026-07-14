#include "AdminSessionPanel.h"
#include "UI/Theme.h"

namespace vox {

namespace {
static void sLabel(juce::Label &l, float sz, juce::Colour c, bool bold = false,
                   juce::Justification j = juce::Justification::centredLeft) {
    l.setColour(juce::Label::textColourId, c);
    l.setFont(juce::Font(juce::FontOptions(sz, bold ? juce::Font::bold : juce::Font::plain)));
    l.setJustificationType(j);
}
} // namespace

AdminSessionPanel::AdminSessionPanel(AdminSessionManager &mgr)
    : session(mgr)
{
    sLabel(headingLabel, 13, theme::text, true);
    headingLabel.setText(juce::String::fromUTF8("Sessão Administrativa"),
                         juce::dontSendNotification);

    sLabel(avatarLabel, 22, theme::cyan, true, juce::Justification::centred);
    sLabel(nameLabel,   14, theme::text, true);
    sLabel(roleLabel,   11, theme::muted);
    sLabel(stateLabel,  11, theme::green, true);

    sLabel(startedLabel,  11, theme::muted);
    sLabel(lastLabel,     11, theme::muted);
    sLabel(expiresLabel,  11, theme::muted);
    sLabel(remainingLabel,12, theme::yellow, true);

    for (auto *l : { &headingLabel, &avatarLabel, &nameLabel, &roleLabel,
                     &stateLabel, &startedLabel, &lastLabel,
                     &expiresLabel, &remainingLabel })
        addAndMakeVisible(l);

    renewBtn.setButtonText(juce::String::fromUTF8("↻  Renovar sessão"));
    renewBtn.setColour(juce::TextButton::buttonColourId, theme::blue);
    renewBtn.setColour(juce::TextButton::textColourOffId, theme::text);
    renewBtn.onClick = [this] { if (onRenew) onRenew(); };

    lockBtn.setButtonText(juce::String::fromUTF8("🔒  Bloquear sessão"));
    lockBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    lockBtn.setColour(juce::TextButton::textColourOffId, theme::yellow);
    lockBtn.onClick = [this] { if (onLock) onLock(); };

    endBtn.setButtonText(juce::String::fromUTF8("✕  Encerrar sessão"));
    endBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    endBtn.setColour(juce::TextButton::textColourOffId, theme::red);
    endBtn.onClick = [this] { if (onEnd) onEnd(); };

    for (auto *b : { &renewBtn, &lockBtn, &endBtn })
        addAndMakeVisible(b);

    refresh();
}

juce::Colour AdminSessionPanel::stateColour() const noexcept {
    switch (session.state()) {
        case AdminSessionState::Active:        return theme::green;
        case AdminSessionState::ExpiringSoon:  return theme::yellow;
        case AdminSessionState::Expired:       return theme::red;
        case AdminSessionState::Locked:        return theme::yellow;
        default:                               return theme::muted;
    }
}

void AdminSessionPanel::refresh() {
    const auto &u    = session.user();
    const auto &info = session.session();
    const bool  active = session.isActive();

    // Avatar: inicial do nome
    const juce::String initial = u.name.isNotEmpty()
        ? u.name.substring(0, 1).toUpperCase() : "?";
    avatarLabel.setText(initial, juce::dontSendNotification);

    nameLabel.setText(u.name.isEmpty()
        ? juce::String::fromUTF8("Não autenticado") : u.name,
        juce::dontSendNotification);
    roleLabel.setText(u.id.isNotEmpty() ? roleName(u.role) : juce::String{},
                      juce::dontSendNotification);

    juce::String stateText;
    switch (session.state()) {
        case AdminSessionState::Active:
            stateText = juce::String::fromUTF8("● Sessão ativa"); break;
        case AdminSessionState::ExpiringSoon:
            stateText = juce::String::fromUTF8("⚠ Expirando em breve"); break;
        case AdminSessionState::Expired:
            stateText = juce::String::fromUTF8("✕ Sessão expirada"); break;
        case AdminSessionState::Locked:
            stateText = juce::String::fromUTF8("🔒 Sessão bloqueada"); break;
        default:
            stateText = juce::String::fromUTF8("○ Inativa"); break;
    }
    stateLabel.setText(stateText, juce::dontSendNotification);
    stateLabel.setColour(juce::Label::textColourId, stateColour());

    if (active && info.startedAt.toMilliseconds() > 0) {
        startedLabel.setText(
            juce::String::fromUTF8("Iniciada: ") +
            info.startedAt.toString(true, true),
            juce::dontSendNotification);
        lastLabel.setText(
            juce::String::fromUTF8("Último acesso: ") +
            info.lastActivity.toString(true, true),
            juce::dontSendNotification);
        expiresLabel.setText(
            juce::String::fromUTF8("Expira: ") +
            info.expiresAt.toString(true, true),
            juce::dontSendNotification);
        remainingLabel.setText(
            info.remainingText() + juce::String::fromUTF8(" restantes"),
            juce::dontSendNotification);
    } else {
        startedLabel.setText({}, juce::dontSendNotification);
        lastLabel.setText({}, juce::dontSendNotification);
        expiresLabel.setText({}, juce::dontSendNotification);
        remainingLabel.setText({}, juce::dontSendNotification);
    }

    const bool expiringSoon = session.state() == AdminSessionState::ExpiringSoon;
    remainingLabel.setColour(juce::Label::textColourId,
                             expiringSoon ? theme::yellow : theme::green);
    renewBtn.setVisible(active || session.state() == AdminSessionState::Expired ||
                        session.state() == AdminSessionState::Locked);
    lockBtn.setVisible(active);
    endBtn.setVisible(active || session.state() == AdminSessionState::Locked);

    repaint();
}

void AdminSessionPanel::paint(juce::Graphics &g) {
    theme::roundedPanel(g, getLocalBounds().toFloat(), 12, theme::elevated);

    // Avatar circular
    auto avatarCircle = juce::Rectangle<int>(
        getLocalBounds().getCentreX() - 24,
        getY() + 36, 48, 48);
    g.setColour(stateColour().withAlpha(0.18f));
    g.fillEllipse(avatarCircle.toFloat());
    g.setColour(stateColour().withAlpha(0.6f));
    g.drawEllipse(avatarCircle.toFloat(), 2.0f);

    // Indicador de estado (ponto colorido)
    auto dot = juce::Rectangle<float>(
        (float)avatarCircle.getRight() - 12.0f,
        (float)avatarCircle.getBottom() - 12.0f,
        10.0f, 10.0f);
    g.setColour(theme::background);
    g.fillEllipse(dot.expanded(2));
    g.setColour(stateColour());
    g.fillEllipse(dot);
}

void AdminSessionPanel::resized() {
    auto a = getLocalBounds().reduced(12, 10);
    headingLabel.setBounds(a.removeFromTop(24));
    a.removeFromTop(6);

    // Avatar (desenhado no paint, label flutuante)
    avatarLabel.setBounds(
        juce::Rectangle<int>(a.getCentreX() - 24, a.getY(), 48, 48));
    a.removeFromTop(56);

    nameLabel.setBounds(a.removeFromTop(22));
    roleLabel.setBounds(a.removeFromTop(18));
    stateLabel.setBounds(a.removeFromTop(18));
    a.removeFromTop(8);

    startedLabel.setBounds(a.removeFromTop(17));
    lastLabel.setBounds(a.removeFromTop(17));
    expiresLabel.setBounds(a.removeFromTop(17));
    remainingLabel.setBounds(a.removeFromTop(20));
    a.removeFromTop(10);

    const int btnH = 34;
    if (renewBtn.isVisible()) {
        renewBtn.setBounds(a.removeFromTop(btnH));
        a.removeFromTop(4);
    }
    if (lockBtn.isVisible()) {
        lockBtn.setBounds(a.removeFromTop(btnH));
        a.removeFromTop(4);
    }
    if (endBtn.isVisible()) {
        endBtn.setBounds(a.removeFromTop(btnH));
    }
}

} // namespace vox
