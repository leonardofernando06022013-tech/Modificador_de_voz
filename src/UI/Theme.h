#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
namespace vox::theme {
inline std::atomic<bool> reduceAnimations{false};
inline std::atomic<bool> highContrast{false};
inline std::atomic<bool> focusVisible{true};
inline std::atomic<bool> largeClickAreas{false};
inline constexpr float smallRadius = 8.0f;
inline constexpr float panelRadius = 14.0f;
inline constexpr int spacingSmall = 8;
inline constexpr int spacing = 14;
inline constexpr int spacingLarge = 22;
inline const juce::Colour background{0xff080b12}, secondary{0xff0d1320},
    panel{0xff121a28}, elevated{0xff182235};
inline const juce::Colour border{0xff26334a}, text{0xfff4f7ff},
    muted{0xffa7b0c3}, disabled{0xff656f82};
inline const juce::Colour blue{0xff5865ff}, purple{0xff7c4dff},
    cyan{0xff14d9d2}, green{0xff31d889}, yellow{0xfff2c94c}, red{0xffff5364};
inline void roundedPanel(juce::Graphics &g, juce::Rectangle<float> r,
                         float radius = panelRadius, juce::Colour fill = panel) {
  juce::DropShadow(juce::Colours::black.withAlpha(.28f), 9, {0, 3})
      .drawForRectangle(g, r.getSmallestIntegerContainer());
  g.setColour(fill);
  g.fillRoundedRectangle(r, radius);
  g.setColour(juce::Colours::white.withAlpha(.035f));
  g.drawRoundedRectangle(r.reduced(.5f), radius, 1.0f);
  g.setColour(highContrast.load() ? muted : border);
  g.drawRoundedRectangle(r, radius, highContrast.load() ? 1.5f : 1.0f);
}
inline void paintBackground(juce::Graphics &g, juce::Rectangle<int> bounds) {
  juce::ColourGradient base(juce::Colour(0xff080b13), 0.0f, 0.0f,
                            juce::Colour(0xff0b1020),
                            static_cast<float>(bounds.getWidth()),
                            static_cast<float>(bounds.getHeight()), false);
  g.setGradientFill(base);
  g.fillRect(bounds);
  const auto b = bounds.toFloat();
  g.setColour(purple.withAlpha(.055f));
  g.fillEllipse(b.getRight() - 430.0f, b.getY() - 260.0f, 560.0f, 560.0f);
  g.setColour(cyan.withAlpha(.035f));
  g.fillEllipse(b.getX() - 260.0f, b.getBottom() - 360.0f, 500.0f, 500.0f);
  juce::Path wave;
  wave.startNewSubPath(b.getX(), b.getBottom() - 90.0f);
  wave.cubicTo(b.getX() + b.getWidth() * .25f, b.getBottom() - 130.0f,
               b.getX() + b.getWidth() * .68f, b.getBottom() - 35.0f,
               b.getRight(), b.getBottom() - 75.0f);
  g.setColour(blue.withAlpha(.045f));
  g.strokePath(wave, juce::PathStrokeType(2.0f));
}
} // namespace vox::theme
