#pragma once

#include "Audio/AudioEngine.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/ModernComponents.h"
#include "UI/Navigation/PageRouter.h"
#include "Integrations/ApplicationDetector.h"
#include "Integrations/VirtualDeviceDetector.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace vox {
class ModulePage final : public juce::Component, private juce::Timer {
public:
  enum class Kind { Home, Effects, Soundboard, Favorites, Equalizer, Presets };

  ModulePage(Kind, AudioEngine &, PresetManager &, SettingsManager &);
  ~ModulePage() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool runSmokeTest();
  std::function<void(PageId)> navigate;
  std::function<void(const juce::String &)> notify;

private:
  void timerCallback() override;
  void setup();
  void selectSound(int);
  void rebuildSoundPads();
  void rebuildFavorites();
  void buildEffects();
  void layoutCards();
  void refreshHomeFavorites();

  Kind kind;
  AudioEngine &engine;
  PresetManager &presets;
  SettingsManager &settings;
  juce::Label title, description, status, emptyState;
  juce::Label homeInput, homeOutput, homeVoice, homeIntegration;
  juce::TextButton primary, secondary, tertiary;
  juce::ComboBox presetList, soundList;
  juce::Slider hp, lp, bass, mid, treble, soundProgress;
  juce::Slider homeInputGain, homeOutputGain, homeMix, homeNoise;
  AudioLevelMeter homeInputMeter, homeOutputMeter;
  juce::OwnedArray<juce::TextButton> homeFavorites;
  juce::OwnedArray<juce::TextButton> soundPads;
  juce::Viewport cardViewport;
  juce::Component cardCanvas;
  juce::OwnedArray<VoiceCardComponent> cards;
  std::unique_ptr<juce::FileChooser> chooser;
  juce::Array<juce::File> importedSounds;
  juce::File selectedSound;
  juce::String favouriteSignature;
  juce::String homeFavouriteSignature;
  int homeRefreshCounter = 19;
  bool favoritesInitialized = false;
  juce::Array<juce::Rectangle<int>> panels;
  juce::Rectangle<int> equalizerGraphArea;
  juce::Rectangle<int> homeHeroArea, homeStatsArea, homeControlsArea,
      homeFavoritesArea;
};
} // namespace vox
