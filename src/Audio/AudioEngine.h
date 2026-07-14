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
    juce::String setMonitoring(bool enabled);
    bool isMonitoring() const noexcept { return monitoring.load(); }
    juce::String monitorDeviceName() const;
    void saveDeviceState();
    juce::String loadSoundboardFile(const juce::File &);
    void playSoundboard();
    void stopSoundboard();
    bool isSoundboardPlaying() const noexcept { return soundboardPlaying.load(); }
    double soundboardPosition() const;
    double soundboardDuration() const;
    juce::String soundboardFileName() const;
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override; void audioDeviceStopped() override;
private:
    class MonitorCallback final : public juce::AudioIODeviceCallback {
    public:
      explicit MonitorCallback(AudioEngine &e) : owner(e) {}
      void audioDeviceIOCallbackWithContext(const float *const *, int,
          float *const *, int, int,
          const juce::AudioIODeviceCallbackContext &) override;
      void audioDeviceAboutToStart(juce::AudioIODevice *) override;
      void audioDeviceStopped() override {}
    private: AudioEngine &owner;
    };
    void pushMonitor(const juce::AudioBuffer<float> &, int) noexcept;
    void renderMonitor(float *const *, int, int) noexcept;
    void timerCallback() override;
    static constexpr int maximumRealtimeBlockSize = 8192;
    juce::AudioDeviceManager devices, monitorDevices; Parameters params; VoiceProcessor voice{params}; juce::AudioBuffer<float> work;
    juce::AudioFormatManager soundboardFormats;
    juce::TimeSliceThread soundboardReadAhead{"Soundboard read-ahead"};
    std::unique_ptr<juce::AudioFormatReaderSource> soundboardReader;
    juce::AudioTransportSource soundboardTransport;
    juce::AudioBuffer<float> soundboardBuffer;
    mutable juce::CriticalSection soundboardLock;
    juce::File loadedSoundboardFile;
    std::atomic<bool> soundboardPlaying{false};
    MonitorCallback monitorCallback{*this};
    juce::AudioBuffer<float> monitorRing{2, 96000};
    juce::AbstractFifo monitorFifo{96000};
    std::atomic<bool> monitoring{false};
    std::atomic<bool> running{false}; std::atomic<double> cpu{0}; std::atomic<uint64_t> xruns{0};
    std::atomic<double> activeSampleRate{48000.0}; std::atomic<int> preparedSamples{0};
    juce::String error; mutable juce::CriticalSection errorLock;
};
}
