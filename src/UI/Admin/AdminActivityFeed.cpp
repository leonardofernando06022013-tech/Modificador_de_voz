#include "AdminActivityFeed.h"
#include "UI/Theme.h"

namespace vox {

AdminActivityFeed::AdminActivityFeed() {
    heading.setText(juce::String::fromUTF8("Atividades recentes"), juce::dontSendNotification);
    heading.setFont(juce::Font(juce::FontOptions(16, juce::Font::bold)));
    heading.setColour(juce::Label::textColourId, theme::text);
    summary.setColour(juce::Label::textColourId, theme::muted);
    summary.setJustificationType(juce::Justification::centredRight);
    severity.addItem(juce::String::fromUTF8("Todas as severidades"), 1);
    severity.addItem(juce::String::fromUTF8("Informação"), 2);
    severity.addItem("Aviso", 3);
    severity.addItem(juce::String::fromUTF8("Crítico"), 4);
    severity.setSelectedId(1);
    severity.onChange = [this] { applyFilter(); };
    juce::Component *components[]{&heading, &summary, &severity, &list, &empty};
    for (auto *component : components)
        addAndMakeVisible(component);

    empty.setText(juce::String::fromUTF8("✓  Nenhuma atividade encontrada"),
                  juce::dontSendNotification);
    empty.setJustificationType(juce::Justification::centred);
    empty.setColour(juce::Label::textColourId, theme::muted);
    list.setRowHeight(76);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
}

void AdminActivityFeed::setEvents(juce::Array<AuditEvent> events) {
    all = std::move(events);
    applyFilter();
}

void AdminActivityFeed::setFilter(const juce::String &newQuery) {
    query = newQuery.trim();
    applyFilter();
}

void AdminActivityFeed::applyFilter() {
    visible.clear();
    for (auto &event : all) {
        const bool textMatches = query.isEmpty() ||
            event.action.containsIgnoreCase(query) || event.target.containsIgnoreCase(query) ||
            event.userId.containsIgnoreCase(query) || event.role.containsIgnoreCase(query) ||
            event.result.containsIgnoreCase(query);
        const int selected = severity.getSelectedId();
        const bool severityMatches = selected <= 1 ||
            (selected == 2 && !event.severity.containsIgnoreCase("WARN") &&
             !event.severity.containsIgnoreCase("CRIT")) ||
            (selected == 3 && event.severity.containsIgnoreCase("WARN")) ||
            (selected == 4 && event.severity.containsIgnoreCase("CRIT"));
        if (textMatches && severityMatches)
            visible.add(event);
    }
    summary.setText(juce::String(visible.size()) + juce::String::fromUTF8(" evento(s)"),
                    juce::dontSendNotification);
    empty.setVisible(visible.isEmpty());
    list.updateContent();
    list.repaint();
}

int AdminActivityFeed::getNumRows() { return visible.size(); }

juce::String AdminActivityFeed::friendly(const AuditEvent &event) const {
    if (event.action == "SESSION_START") return "Login administrativo";
    if (event.action == "USER_SAVE") return juce::String::fromUTF8("Usuário salvo");
    if (event.action == "USER_DELETE") return juce::String::fromUTF8("Usuário removido");
    if (event.action == "USER_BLOCK") return juce::String::fromUTF8("Status do usuário alterado");
    if (event.action == "BACKUP_CREATE") return "Backup criado";
    if (event.action == "LOG_EXPORT") return "Logs exportados";
    if (event.action == "LOG_DELETE") return "Logs antigos removidos";
    if (event.action == "ACCESS_DENIED") return "Tentativa de acesso negada";
    return event.action.replace("_", " ");
}

void AdminActivityFeed::paintListBoxItem(int row, juce::Graphics &g, int width,
                                         int height, bool selected) {
    if (!juce::isPositiveAndBelow(row, visible.size())) return;
    auto &event = visible.getReference(row);
    auto bounds = juce::Rectangle<int>(0, 2, width, height - 4).reduced(8);
    g.setColour(selected ? theme::elevated.brighter(0.12f) : theme::elevated);
    g.fillRoundedRectangle(bounds.toFloat(), 9);
    const auto colour = event.severity.containsIgnoreCase("CRIT") ? theme::red
        : event.severity.containsIgnoreCase("WARN") ? theme::yellow
        : event.result == "SUCCESS" ? theme::green : theme::blue;
    g.setColour(colour);
    g.fillRoundedRectangle(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()),
                           4.0f, static_cast<float>(bounds.getHeight()), 2.0f);
    bounds.removeFromLeft(14);
    const auto timeArea = bounds.removeFromRight(150);
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
    g.drawText(friendly(event), bounds.removeFromTop(25), juce::Justification::centredLeft);
    g.setColour(theme::muted);
    g.setFont(11.0f);
    g.drawText(event.role + juce::String::fromUTF8(" · ") + event.result,
               bounds.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText(event.target, bounds, juce::Justification::centredLeft);
    g.setColour(colour);
    g.drawText(event.time.toString(true, true), timeArea,
               juce::Justification::centredRight);
}

void AdminActivityFeed::resized() {
    auto area = getLocalBounds();
    auto header = area.removeFromTop(44);
    heading.setBounds(header.removeFromLeft(190));
    severity.setBounds(header.removeFromRight(180).reduced(2, 5));
    summary.setBounds(header.removeFromRight(100));
    list.setBounds(area);
    empty.setBounds(area);
}

} // namespace vox
