#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox::theme {
inline const juce::Colour background{0xff0d0f15}, secondary{0xff131620}, panel{0xff181b26}, elevated{0xff202431};
inline const juce::Colour border{0xff2a2f40}, text{0xfff4f6ff}, muted{0xffa2a8bd}, disabled{0xff656b7c};
inline const juce::Colour blue{0xff5865ff}, purple{0xff814dff}, cyan{0xff15ddd2}, green{0xff38d996}, yellow{0xfff6c453}, red{0xffff5263};
inline void roundedPanel(juce::Graphics& g, juce::Rectangle<float> r, float radius=12.0f, juce::Colour fill=panel) { g.setColour(fill); g.fillRoundedRectangle(r, radius); g.setColour(border); g.drawRoundedRectangle(r, radius, 1.0f); }
}
