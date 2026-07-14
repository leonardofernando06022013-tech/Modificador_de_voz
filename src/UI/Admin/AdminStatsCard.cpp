#include "AdminStatsCard.h"
#include "UI/Theme.h"

namespace vox {

// ─── AdminStatsCard ─────────────────────────────────────────────────────────

AdminStatsCard::AdminStatsCard(const juce::String &title,
                               const juce::String &icon,
                               juce::Colour accentColour)
    : cardTitle(title), cardIcon(icon), accent(accentColour)
{
    setRepaintsOnMouseActivity(true);
    valueText  = "0";
    detailText = {};
    badgeText  = {};
}

void AdminStatsCard::setValue(int main, const juce::String &detail,
                              const juce::String &badge) {
    valueText  = juce::String(main);
    detailText = detail;
    badgeText  = badge;
    repaint();
}

void AdminStatsCard::setValue(const juce::String &text,
                              const juce::String &detail,
                              const juce::String &badge) {
    valueText  = text;
    detailText = detail;
    badgeText  = badge;
    repaint();
}

void AdminStatsCard::mouseDown(const juce::MouseEvent &) {
    if (onClick) onClick();
}

void AdminStatsCard::mouseEnter(const juce::MouseEvent &) { hovered = true;  repaint(); }
void AdminStatsCard::mouseExit (const juce::MouseEvent &) { hovered = false; repaint(); }

void AdminStatsCard::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds().toFloat().reduced(2);

    // Fundo do cartão
    g.setColour(hovered ? theme::elevated.brighter(0.08f) : theme::elevated);
    g.fillRoundedRectangle(bounds, 10);

    // Borda discreta
    g.setColour(theme::border);
    g.drawRoundedRectangle(bounds, 10, 1.0f);

    // Barra lateral colorida
    g.setColour(accent);
    g.fillRoundedRectangle(bounds.getX(), bounds.getY(),
                           4.0f, bounds.getHeight(), 2.0f);

    // Ícone circular (topo direito)
    float iconSize = 36.0f;
    auto iconRect = juce::Rectangle<float>(
        bounds.getRight() - iconSize - 10,
        bounds.getY() + 10, iconSize, iconSize);
    g.setColour(accent.withAlpha(0.15f));
    g.fillEllipse(iconRect);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(16, juce::Font::bold)));
    g.drawText(cardIcon, iconRect, juce::Justification::centred);

    // Valor principal
    auto inner = bounds.reduced(14, 8).withTrimmedRight(iconSize + 8);
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(26, juce::Font::bold)));
    g.drawText(valueText, inner.removeFromTop(32), juce::Justification::centredLeft);

    // Título
    g.setColour(theme::muted);
    g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText(cardTitle, inner.removeFromTop(16), juce::Justification::centredLeft);

    // Detalhe
    if (detailText.isNotEmpty()) {
        g.setFont(juce::Font(juce::FontOptions(10)));
        g.drawFittedText(detailText, inner.withHeight(16).toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }

    // Badge
    if (badgeText.isNotEmpty()) {
        g.setFont(juce::Font(juce::FontOptions(9, juce::Font::bold)));
        float bw = juce::jmax(36.0f, (float)badgeText.length() * 5.5f + 12.0f);
        auto badge = juce::Rectangle<float>(
            bounds.getRight() - bw - 6,
            bounds.getBottom() - 20, bw, 14);
        g.setColour(accent.withAlpha(0.18f));
        g.fillRoundedRectangle(badge, 4);
        g.setColour(accent);
        g.drawText(badgeText, badge, juce::Justification::centred);
    }
}

void AdminStatsCard::resized() {}

// ─── AdminStatsGrid ──────────────────────────────────────────────────────────

AdminStatsGrid::AdminStatsGrid() {
    // Criar 6 cartões com configurações fixas
    struct Config { const char *title; const char *icon; uint32_t colour; };
    static const Config configs[6] = {
        { "Usu\xC3\xA1rios cadastrados", "U",  0xff5865ff },
        { "Usu\xC3\xA1rios ativos",      "\xe2\x9c\x93", 0xff31d889 },
        { "Administradores",             "A",  0xff7c4dff },
        { "A\xC3\xa7\xC3\xb5\x65s (7 dias)", "!", 0xfff2c94c },
        { "Presets",                     "P",  0xff14d9d2 },
        { "Alertas",                     "\xe2\x9a\xa0", 0xffff5364 },
    };
    for (auto &cfg : configs) {
        auto *card = cards.add(new AdminStatsCard(
            juce::String::fromUTF8(cfg.title),
            juce::String::fromUTF8(cfg.icon),
            juce::Colour(cfg.colour)));
        addAndMakeVisible(card);
    }
}

void AdminStatsGrid::setData(int totalUsers, int activeUsers, int adminCount,
                             int recentActions, int totalPresets, int alerts) {
    if (cards.size() < 6) return;

    cards[0]->setValue(totalUsers,
        juce::String(activeUsers) + juce::String::fromUTF8(" ativos"),
        totalUsers > 1 ? juce::String::fromUTF8("Registrados") : juce::String::fromUTF8("Registrado"));

    cards[1]->setValue(activeUsers,
        juce::String::fromUTF8("Online agora"),
        activeUsers > 0 ? juce::String::fromUTF8("Ativo") : juce::String::fromUTF8("Nenhum"));

    cards[2]->setValue(adminCount,
        adminCount == 1 ? juce::String::fromUTF8("Administrador") :
                          juce::String::fromUTF8("Administradores"),
        adminCount > 0  ? juce::String::fromUTF8("Privilegiado") : juce::String{});

    cards[3]->setValue(recentActions,
        juce::String::fromUTF8("nos \xC3\xbaltimos 7 dias"),
        recentActions > 0 ? juce::String::fromUTF8("Recente") : juce::String{});

    cards[4]->setValue(totalPresets,
        juce::String::fromUTF8("presets locais"),
        juce::String::fromUTF8("Dispon\xC3\xadveis"));

    cards[5]->setValue(alerts,
        alerts > 0 ? juce::String::fromUTF8("Requer aten\xC3\xa7\xC3\xa3o") :
                     juce::String::fromUTF8("Sistema saud\xC3\xa1vel"),
        alerts > 0 ? juce::String::fromUTF8("Alerta") : juce::String::fromUTF8("OK"));
}

void AdminStatsGrid::resized() {
    if (cards.isEmpty()) return;
    const int n = cards.size();
    const int w = getWidth();
    const int cols = w < 700 ? 3 : 6;
    const int rows = (n + cols - 1) / cols;
    const int gap  = 8;
    const int cardW = (w - (cols - 1) * gap) / cols;
    const int cardH = (getHeight() - (rows - 1) * gap) / rows;

    for (int i = 0; i < n; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        cards[i]->setBounds(col * (cardW + gap), row * (cardH + gap), cardW, cardH);
    }
}

} // namespace vox
