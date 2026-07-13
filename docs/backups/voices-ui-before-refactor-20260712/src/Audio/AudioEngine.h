#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "DSP/VoiceProcessor.h"
#include <atomic>

namespace vox {
class AudioEngine final : public juce::AudioIODeviceCallback, private juce::Timer {
public:
    AudioEngine(); ~AudioEngine() override;
    juce::String initialise(); void start(); void stop(); bool isRunning() const noexcept { return running.load(); }
    juce::AudioDeviceManager& deviceManager() noexcept { return devices; }
    const juce::AudioDeviceManager& deviceManager() const noexcept { return devices; }
    Parameters& parameters() noexcept { return params; }
    VoiceProcessor& processor() noexcept { return voice; }
    double cpuUsage() const noexcept { return cpu.load(); } uint64_t underruns() const noexcept { return xruns.load(); }
    juce::String lastError() const { const juce::ScopedLock lock(errorLock); return error; }
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override; void audioDeviceStopped() override;
private:
    void timerCallback() override;
    static constexpr int maximumRealtimeBlockSize = 8192;
    juce::AudioDeviceManager devices; Parameters params; VoiceProcessor voice{params}; juce::AudioBuffer<float> work;
    std::atomic<bool> running{false}; std::atomic<double> cpu{0}; std::atomic<uint64_t> xruns{0};
    std::atomic<double> activeSampleRate{48000.0}; std::atomic<int> preparedSamples{0};
    juce::String error; mutable juce::CriticalSection errorLock;
};
}
