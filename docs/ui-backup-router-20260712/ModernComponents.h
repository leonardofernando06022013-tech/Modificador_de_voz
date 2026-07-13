#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
namespace vox {
class AudioLevelMeter final:public juce::Component{public:void setLevels(float a,float b){in=a;out=b;repaint();}void paint(juce::Graphics&)override;private:float in=0,out=0;};
class VoiceCardComponent final:public juce::Button{public:VoiceCardComponent(juce::String,juce::String,int);void setSelected(bool s){selected=s;repaint();}const juce::String& presetName()const{return name;}const juce::String& presetCategory()const{return category;}void paintButton(juce::Graphics&,bool,bool)override;private:juce::String name,category;int colourIndex=0;bool selected=false;};
class SidebarComponent final:public juce::Component{public:SidebarComponent();std::function<void(int)>onSelect;void setSelected(int);void resized()override;void paint(juce::Graphics&)override;private:juce::OwnedArray<juce::TextButton>buttons;int selected=0;};
class NotificationComponent final:public juce::Component,private juce::Timer{public:NotificationComponent();void showMessage(const juce::String&,bool error=false);void paint(juce::Graphics&)override;private:void timerCallback()override;juce::String message;juce::Colour colour;};
}
