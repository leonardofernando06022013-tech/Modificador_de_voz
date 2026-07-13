#pragma once
#include "Theme.h"
#include "Localization/LocalizationManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox {
class AudioLevelMeter final : public juce::Component {
public:
  void setLevels(float a, float b) {
    in = a;
    out = b;
    peak = juce::jmax(peak * .985f, juce::jmax(a, b));
    repaint();
  }
  void paint(juce::Graphics &) override;

private:
  float in = 0, out = 0, peak = 0;
};
class VoiceCardComponent final : public juce::Button {
public:
  VoiceCardComponent(juce::String, juce::String, int);
  void setSelected(bool s) {
    selected = s;
    repaint();
  }
  void setFavourite(bool f) {
    favourite = f;
    repaint();
  }
  bool isFavourite() const { return favourite; }
  const juce::String &presetName() const { return name; }
  const juce::String &presetCategory() const { return category; }
  std::function<void(bool)> onFavourite;
  void paintButton(juce::Graphics &, bool, bool) override;
  void mouseUp(const juce::MouseEvent &) override;

private:
  void drawAvatar(juce::Graphics &, juce::Rectangle<float>);
  juce::String name, category;
  int colourIndex = 0;
  bool selected = false, favourite = false;
};
class SidebarComponent final : public juce::Component {
public:
  SidebarComponent();
  std::function<void(int)> onSelect;
  void setSelected(int);
  void updateTexts();
  void setCompact(bool shouldBeCompact) {
    if (compact == shouldBeCompact) return;
    compact = shouldBeCompact;
    updateTexts();
    resized();
  }
  void setAdminVisible(bool visible){if(buttons.size()>11)buttons[11]->setVisible(visible);}
  void resized() override;
  void paint(juce::Graphics &) override;

private:
  juce::OwnedArray<juce::TextButton> buttons;
  int selected = 0;
  bool compact = false;
};
class NotificationComponent final : public juce::Component,
                                    private juce::Timer {
public:
  NotificationComponent();
  void showMessage(const juce::String &, bool error = false);
  void paint(juce::Graphics &) override;

private:
  void timerCallback() override;
  juce::String message;
  juce::Colour colour;
};
} // namespace vox
