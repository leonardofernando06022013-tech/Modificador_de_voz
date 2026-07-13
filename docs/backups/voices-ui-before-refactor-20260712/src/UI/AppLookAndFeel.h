#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace vox { class AppLookAndFeel final : public juce::LookAndFeel_V4 {
public: AppLookAndFeel();
void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
}; }
