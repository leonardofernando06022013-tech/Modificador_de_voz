#include "AdminOverviewTab.h"
#include "UI/Theme.h"

namespace vox {

struct QuickItemDef {
    const char *icon;
    const char *title;
    const char *detail;
    int         tab;
};

static const QuickItemDef kQuickItems[] = {
    { "U",  "Gerenciar usu\xC3\xa1rios",   "Adicionar, editar, bloquear", 1 },
    { "P",  "Gerenciar permiss\xC3\xb5\x65s", "Matriz de acesso por fun\xC3\xa7\xC3\xa3o", 2 },
    { "\xe2\x98\x86", "Revisar presets",   "Aprovar, rejeitar, exportar", 3 },
    { "\xF0\x9F\x94\x92", "Abrir seguran\xC3\xa7\xC3\xa1", "Configura\xC3\xa7\xC3\xb5\x65s de sess\xC3\xa3o", 4 },
    { "\xe2\x98\xb0", "Visualizar logs",   "Busca e exporta\xC3\xa7\xC3\xa3o", 5 },
    { "\xe2\x96\xa3", "Criar backup",       "Salvar snapshot atual", 6 },
};

AdminOverviewTab::AdminOverviewTab(AdminSessionManager &s, UserManager &u,
                                   PresetManager &p, AuditLogManager &a)
    : session(s), users(u), presets(p), audit(a)
{
    addAndMakeVisible(statsGrid);
    addAndMakeVisible(activityFeed);

    quickLabel.setText(juce::String::fromUTF8("Acesso Rápido"),
                       juce::dontSendNotification);
    quickLabel.setColour(juce::Label::textColourId, theme::text);
    quickLabel.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    addAndMakeVisible(quickLabel);

    for (auto &item : kQuickItems) {
        auto *btn = quickButtons.add(new juce::TextButton(
            juce::String::fromUTF8(item.icon) + "  " +
            juce::String::fromUTF8(item.title)));
        btn->setTooltip(juce::String::fromUTF8(item.detail));
        btn->setColour(juce::TextButton::buttonColourId, theme::elevated);
        btn->setColour(juce::TextButton::textColourOffId, theme::text);
        const int tab = item.tab;
        btn->onClick = [this, tab] {
            if (onNavigateTab) onNavigateTab(tab);
        };
        addAndMakeVisible(btn);
    }
}

void AdminOverviewTab::refresh() {
    auto all = users.users();
    int admins = 0, active = 0, blocked = 0;
    for (auto &u : all) {
        if (u.role >= Role::Administrator) ++admins;
        if (u.status == UserStatus::Active)  ++active;
        if (u.status == UserStatus::Blocked) ++blocked;
    }
    const int recentActions = (int)audit.recentEvents(1000).size();
    const int totalPresets  = (int)presets.names().size();

    statsGrid.setData((int)all.size(), active, admins,
                      recentActions, totalPresets, blocked);
    activityFeed.setEvents(audit.recentEvents(50));
}

void AdminOverviewTab::resized() {
    auto a = getLocalBounds().reduced(0, 4);

    // Cartões de estatísticas
    const int statsH = getWidth() < 700 ? 180 : 100;
    statsGrid.setBounds(a.removeFromTop(statsH));
    a.removeFromTop(12);

    // Divisão: atividades (esq 65%) + acesso rápido (dir 35%)
    auto row = a;
    const int quickW = juce::jmin(260, getWidth() / 3);
    auto quickArea = row.removeFromRight(quickW);
    row.removeFromRight(10);

    activityFeed.setBounds(row);

    quickLabel.setBounds(quickArea.removeFromTop(28));
    quickArea.removeFromTop(4);
    const int btnH = 46;
    for (auto *btn : quickButtons) {
        btn->setBounds(quickArea.removeFromTop(btnH).reduced(0, 3));
    }
}

} // namespace vox
