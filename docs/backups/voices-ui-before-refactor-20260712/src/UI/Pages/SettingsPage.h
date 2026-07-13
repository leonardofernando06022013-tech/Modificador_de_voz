#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Audio/AudioEngine.h"
#include "Settings/SettingsManager.h"
namespace vox {
class SettingsPage final:public juce::Component,private juce::Timer{
public:SettingsPage(AudioEngine&,SettingsManager&);~SettingsPage()override;void paint(juce::Graphics&)override;void resized()override;std::function<void()>onOpenDevices;
private:
 void timerCallback()override;void markDirty();void applyDevice(int,double);void restoreDefaults();void setupCombo(juce::ComboBox&,const juce::StringArray&,int);void setupToggle(juce::ToggleButton&,const juce::String&,bool);void row(juce::Rectangle<int>&,juce::Component&);void layoutCard(int,juce::Rectangle<int>);
 AudioEngine&engine;SettingsManager&settings;bool dirty=false;juce::Label title,subtitle,saveStatus;juce::TextButton restore,openDevices,exportButton,importButton,openLogs,clearLogs,clearCache,checkUpdates,backupButton;
 juce::ComboBox themeBox,language,fontSize,scale,quality,buffer,sampleRate,audioMode,channels,processPriority,meterRate,logLevel,updateChannel;
 juce::ToggleButton compact,reduceAnimations,showTips,autoContext,monitorInput,safetyLimiter,reconnect,syncDevices,startWindows,startMinimized,startTray,restorePreset,autoProcess,cpuOptimization,adaptiveQuality,economy,highContrast,focusVisible,largeClick,keyboardNav,saveLogs,telemetry,autoUpdates;
 juce::Array<juce::Rectangle<int>>cards;juce::Viewport viewport;juce::Component canvas;
};}
