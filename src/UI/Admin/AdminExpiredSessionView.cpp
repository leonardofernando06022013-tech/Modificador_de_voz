#include "AdminExpiredSessionView.h"
#include "UI/Theme.h"

namespace vox {

namespace {
static void styleLabel(juce::Label &l, float size,
                       juce::Colour col, bool bold = false) {
    l.setColour(juce::Label::textColourId, col);
    l.setFont(juce::Font(juce::FontOptions(size,
                         bold ? juce::Font::bold : juce::Font::plain)));
    l.setJustificationType(juce::Justification::centred);
}
} // namespace

AdminExpiredSessionView::AdminExpiredSessionView(const AdminSessionManager &mgr)
    : session(mgr)
{
    styleLabel(iconLabel,    48, theme::yellow, true);
    styleLabel(titleLabel,   26, theme::text,   true);
    styleLabel(subtitleLabel,14, theme::muted);
    styleLabel(userLabel,    13, theme::text);
    styleLabel(roleLabel,    12, theme::muted);
    styleLabel(timeLabel,    11, theme::muted);
    styleLabel(reasonLabel,  11, theme::yellow);

    for (auto *l : { &iconLabel, &titleLabel, &subtitleLabel,
                     &userLabel, &roleLabel, &timeLabel, &reasonLabel })
        addAndMakeVisible(l);

    renewButton.setButtonText(
        juce::String::fromUTF8("Renovar sessão"));
    renewButton.setColour(juce::TextButton::buttonColourId, theme::blue);
    renewButton.setColour(juce::TextButton::textColourOffId, theme::text);
    renewButton.onClick = [this] { if (onRenew) onRenew(); };

    backButton.setButtonText("Voltar ao aplicativo");
    backButton.setColour(juce::TextButton::buttonColourId, theme::panel);
    backButton.setColour(juce::TextButton::textColourOffId, theme::muted);
    backButton.onClick = [this] { if (onBack) onBack(); };

    endButton.setButtonText(
        juce::String::fromUTF8("Encerrar conta administrativa"));
    endButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    endButton.setColour(juce::TextButton::textColourOffId, theme::red);
    endButton.onClick = [this] { if (onEndAdmin) onEndAdmin(); };

    for (auto *b : { &renewButton, &backButton, &endButton })
        addAndMakeVisible(b);

    refresh();
}

void AdminExpiredSessionView::refresh() {
    iconLabel.setText(stateColour() == theme::red ? "!" :
                      stateColour() == theme::yellow ? "⚠" : "🔒",
                      juce::dontSendNotification);
    titleLabel.setText(stateTitle(), juce::dontSendNotification);
    subtitleLabel.setText(stateSubtitle(), juce::dontSendNotification);

    const auto &u = session.user();
    if (u.id.isNotEmpty()) {
        userLabel.setText(u.name, juce::dontSendNotification);
        roleLabel.setText(roleName(u.role), juce::dontSendNotification);
    } else {
        userLabel.setText(juce::String::fromUTF8("Nenhuma conta administrativa"),
                          juce::dontSendNotification);
        roleLabel.setText({}, juce::dontSendNotification);
    }

    const auto &info = session.session();
    if (info.expiresAt.toMilliseconds() > 0) {
        timeLabel.setText(
            juce::String::fromUTF8("Expirou em: ") +
            info.expiresAt.toString(true, true),
            juce::dontSendNotification);
    } else {
        timeLabel.setText({}, juce::dontSendNotification);
    }

    reasonLabel.setText(info.expiredReason, juce::dontSendNotification);

    // "Renovar sessão" só faz sentido se havia sessão antes
    const bool canRenew = (session.state() == AdminSessionState::Expired ||
                           session.state() == AdminSessionState::Locked ||
                           session.state() == AdminSessionState::ExpiringSoon) &&
                          u.id.isNotEmpty();
    renewButton.setVisible(canRenew);

    repaint();
}

juce::Colour AdminExpiredSessionView::stateColour() const noexcept {
    switch (session.state()) {
        case AdminSessionState::ExpiringSoon: return theme::yellow;
        case AdminSessionState::Locked:       return theme::yellow;
        case AdminSessionState::Expired:      return theme::red;
        default:                              return theme::muted;
    }
}

juce::String AdminExpiredSessionView::stateTitle() const {
    switch (session.state()) {
        case AdminSessionState::Expired:
            return juce::String::fromUTF8("Sessão administrativa expirada");
        case AdminSessionState::Locked:
            return juce::String::fromUTF8("Sessão administrativa bloqueada");
        case AdminSessionState::ExpiringSoon:
            return juce::String::fromUTF8("Sessão expirando em breve");
        default:
            return juce::String::fromUTF8("Acesso administrativo inativo");
    }
}

juce::String AdminExpiredSessionView::stateSubtitle() const {
    switch (session.state()) {
        case AdminSessionState::Expired:
            return juce::String::fromUTF8(
                "Por segurança, sua sessão administrativa foi encerrada.\n"
                "Confirme sua identidade para continuar.");
        case AdminSessionState::Locked:
            return juce::String::fromUTF8(
                "A sessão foi bloqueada. Renove para continuar.");
        case AdminSessionState::ExpiringSoon:
            return juce::String::fromUTF8(
                "Sua sessão está prestes a expirar. Renove para evitar perda de acesso.");
        default:
            return juce::String::fromUTF8(
                "Nenhuma sessão administrativa ativa.\n"
                "O acesso ao painel requer autenticação local.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminExpiredSessionView::paint(juce::Graphics &g) {
    // Fundo semi-transparente cobrindo tudo
    g.fillAll(theme::background.withAlpha(0.97f));

    // Card central
    auto card = getLocalBounds().withSizeKeepingCentre(
        juce::jmin(520, getWidth() - 48),
        juce::jmin(460, getHeight() - 48));
    theme::roundedPanel(g, card.toFloat(), 16, theme::panel);

    // Barra colorida no topo do card
    auto topBar = card.withHeight(4);
    g.setColour(stateColour());
    g.fillRoundedRectangle(topBar.toFloat(), 16.0f);
    g.fillRect(topBar.withTrimmedTop(2));

    // Ícone circular
    auto iconCircle = juce::Rectangle<int>(card.getCentreX() - 36,
                                           card.getY() + 28, 72, 72);
    g.setColour(stateColour().withAlpha(0.12f));
    g.fillEllipse(iconCircle.toFloat());
    g.setColour(stateColour().withAlpha(0.6f));
    g.drawEllipse(iconCircle.toFloat(), 2.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
void AdminExpiredSessionView::resized() {
    auto card = getLocalBounds().withSizeKeepingCentre(
        juce::jmin(520, getWidth() - 48),
        juce::jmin(460, getHeight() - 48));

    auto inner = card.reduced(32, 16);
    inner.removeFromTop(16);

    // Ícone
    iconLabel.setBounds(inner.removeFromTop(72));
    inner.removeFromTop(8);

    // Título e subtítulo
    titleLabel.setBounds(inner.removeFromTop(34));
    inner.removeFromTop(6);
    subtitleLabel.setBounds(inner.removeFromTop(44));
    inner.removeFromTop(16);

    // Separador visual: info do usuário
    userLabel.setBounds(inner.removeFromTop(24));
    roleLabel.setBounds(inner.removeFromTop(20));
    timeLabel.setBounds(inner.removeFromTop(18));
    reasonLabel.setBounds(inner.removeFromTop(18));
    inner.removeFromTop(16);

    // Botões
    const int btnH = 40;
    const int btnW = juce::jmin(320, inner.getWidth());
    auto btnArea   = inner.withSizeKeepingCentre(btnW, btnH * 3 + 12);

    if (renewButton.isVisible()) {
        renewButton.setBounds(btnArea.removeFromTop(btnH));
        btnArea.removeFromTop(8);
    }
    backButton.setBounds(btnArea.removeFromTop(btnH));
    btnArea.removeFromTop(8);
    endButton.setBounds(btnArea.removeFromTop(btnH));
}

} // namespace vox
