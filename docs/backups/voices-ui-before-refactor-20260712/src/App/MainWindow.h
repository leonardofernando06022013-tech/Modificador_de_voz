#pragma once
#include "UI/MainComponent.h"
namespace vox { class MainWindow final:public juce::DocumentWindow {public:MainWindow(AudioEngine&e,SettingsManager&s):DocumentWindow("Modificador de Voz",juce::Colour(0xff0d0f15),allButtons){setUsingNativeTitleBar(true);setContentOwned(new MainComponent(e,s),true);setResizable(true,true);setResizeLimits(1100,700,2200,1400);centreWithSize(1450,900);setVisible(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}void bringToFront(){setVisible(true);setMinimised(false);toFront(true);}};}
