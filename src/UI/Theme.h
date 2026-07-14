#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
namespace vox::theme {
inline std::atomic<bool> reduceAnimations{false};
inline const juce::Colour background{0xff080b12}, secondary{0xff0d1320},
    panel{0xff121a28}, elevated{0xff182235};
inline const juce::Colour border{0xff26334a}, text{0xfff4f7ff},
    muted{0xffa7b0c3}, disabled{0xff656f82};
inline const juce::Colour blue{0xff5865ff}, purple{0xff7c4dff},
    cyan{0xff14d9d2}, green{0xff31d889}, yellow{0xfff2c94c}, red{0xffff5364};
inline void roundedPanel(juce::Graphics &g, juce::Rectangle<float> r,
                         float radius = 12.0f, juce::Colour fill = panel) {
  g.setColour(fill);
  g.fillRoundedRectangle(r, radius);
  g.setColour(border);
  g.drawRoundedRectangle(r, radius, 1.0f);
}
} // namespace vox::theme
