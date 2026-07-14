#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Cartão visual de estatística administrativa com ícone, valor, label e badge.
class AdminStatsCard final : public juce::Component {
public:
    AdminStatsCard(const juce::String &title,
                   const juce::String &icon,
                   juce::Colour       accentColour);

    void setValue(int main, const juce::String &detail = {},
                  const juce::String &badge = {});
    void setValue(const juce::String &text, const juce::String &detail = {},
                  const juce::String &badge = {});

    void paint(juce::Graphics &) override;
    void resized() override;

    std::function<void()> onClick;

private:
    void mouseDown(const juce::MouseEvent &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent &) override;

    juce::String cardTitle, cardIcon, valueText, detailText, badgeText;
    juce::Colour accent;
    bool hovered = false;
};

/// Grade de 6 cartões de estatísticas.
class AdminStatsGrid final : public juce::Component {
public:
    AdminStatsGrid();

    // Atualiza cada cartão com dados reais
    void setData(int totalUsers, int activeUsers, int adminCount,
                 int recentActions, int totalPresets, int alerts);

    void resized() override;

private:
    juce::OwnedArray<AdminStatsCard> cards;
};

} // namespace vox
