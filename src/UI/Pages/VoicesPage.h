#pragma once
#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/ModernComponents.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {
class VoicesPage final : public juce::Component {
public:
  VoicesPage(AudioEngine &, SettingsManager &, PresetManager &);
  void paint(juce::Graphics &) override;
  void resized() override;
  void setSearchText(const juce::String &);
  bool runSmokeTest();
  juce::String selectedVoice() const;
  juce::String selectedCategory() const;
  std::function<void(const juce::String &)> notify;

private:
  enum class DetailsSection {
    None,
    Voice,
    Equalizer,
    Dynamics,
    Environment,
    Effects,
    Advanced
  };
  void applyVoice(int);
  void applyVoiceByName(const juce::String &, bool preview = false);
  void rebuildCards();
  bool matchesFilter(const juce::String &) const;
  juce::String languageFor(const juce::String &) const;
  void addCustomVoiceCard(const juce::String &, bool selectAfterAdding);
  void updateFilter();
  void markDirty();
  void syncControls();
  void selectDetailsSection(DetailsSection);
  AudioEngine &engine;
  SettingsManager &settings;
  PresetManager &presets;
  juce::Label title, subtitle, selectedCaption, selectedName, selectedType,
      dirtyLabel;
  juce::TextButton createVoice{"+  Criar Voz Personalizada"},
      advancedFilter{juce::String::fromUTF8("⚲")},
      favouriteButton{juce::String::fromUTF8("♡")},
      menuButton{juce::String::fromUTF8("•••")};
  juce::TextButton saveButton{"Salvar"}, resetButton{"Redefinir"};
  juce::TextButton loadMore{"Mostrar mais vozes"};
  juce::OwnedArray<juce::TextButton> chips, accordions;
  juce::ComboBox languageFilter;
  juce::Label resultsLabel;
  juce::Viewport sectionViewport;
  juce::Component sectionContent;
  juce::Label sectionTitle, sectionBody;
  juce::Viewport viewport;
  juce::Component grid;
  juce::OwnedArray<VoiceCardComponent> cards;
  juce::OwnedArray<juce::Label> controlLabels, valueLabels;
  juce::OwnedArray<juce::Slider> sliders;
  juce::StringArray catalogNames;
  juce::StringArray favouriteNames;
  juce::String searchText, activeFilter{"Todas"}, selectedPresetName,
      selectedPresetCategory;
  int selectedIndex = 0;
  int visibleLimit = 72, matchingCount = 0;
  bool dirty = false;
  DetailsSection selectedSection = DetailsSection::Voice;
  juce::Rectangle<int> headerArea, filterArea, gridArea, detailsArea,
      quickControlsHeaderArea;
};
} // namespace vox
