#pragma once
#include "AdminStatsCard.h"
#include "AdminSystemInfo.h"
#include "AdminActivityFeed.h"
#include "Admin/AdminSessionManager.h"
#include "Admin/UserManager.h"
#include "Presets/PresetManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Aba "Visão Geral" — cartões de estatísticas + atividades + acesso rápido.
class AdminOverviewTab final : public juce::Component {
public:
    AdminOverviewTab(AdminSessionManager &, UserManager &,
                     PresetManager &, AuditLogManager &);

    void refresh();
    void resized() override;

    // Callbacks para navegação de aba
    std::function<void(int)> onNavigateTab; // 0-7

private:
    AdminSessionManager &session;
    UserManager         &users;
    PresetManager       &presets;
    AuditLogManager     &audit;

    AdminStatsGrid statsGrid;
    AdminActivityFeed activityFeed;

    // Acesso rápido
    juce::Label quickLabel;
    struct QuickItem {
        juce::String icon, title, detail;
        int          tab;
    };
    juce::OwnedArray<juce::TextButton> quickButtons;
};

} // namespace vox
