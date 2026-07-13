#pragma once
#include "Admin/AuditLogManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox {
class AdminActivityFeed final : public juce::Component, private juce::ListBoxModel {
public:
  AdminActivityFeed();
  void setEvents(juce::Array<AuditEvent>);
  void setFilter(const juce::String &);
  void resized() override;
private:
  int getNumRows() override;
  void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
  juce::String friendly(const AuditEvent &) const;
  void applyFilter();

  juce::Array<AuditEvent> all, visible;
  juce::String query;
  juce::Label heading, summary, empty;
  juce::ComboBox severity;
  juce::ListBox list{"Atividades", this};
};
} // namespace vox
