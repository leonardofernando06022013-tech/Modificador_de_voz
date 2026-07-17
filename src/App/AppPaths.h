#pragma once
#include <juce_core/juce_core.h>
namespace vox {
struct AppPaths {
    static juce::File executableDir(){return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();}
    static bool portable(){return executableDir().getChildFile("portable.flag").existsAsFile();}
    static juce::File data(){auto d=portable()?executableDir().getChildFile("Data"):juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("BlackVoice");d.createDirectory();return d;}
    static juce::File presets(){auto d=portable()?executableDir().getChildFile("Presets"):data().getChildFile("Presets");d.createDirectory();return d;}
    static juce::File localBase(){auto fallback=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);return juce::File(juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA",fallback.getFullPathName()));}
    static juce::File logs(){auto d=portable()?data().getChildFile("Logs"):localBase().getChildFile("BlackVoice").getChildFile("Logs");d.createDirectory();return d;}
    static juce::File cache(){auto d=portable()?data().getChildFile("Cache"):localBase().getChildFile("BlackVoice").getChildFile("Cache");d.createDirectory();return d;}
};
}
