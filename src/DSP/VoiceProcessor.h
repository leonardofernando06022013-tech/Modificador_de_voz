#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace vox {
struct Parameters {
    std::atomic<float> inputGainDb{0}, outputGainDb{0}, mix{1}, pitchSemitones{0}, fineCents{0}, formant{0};
    std::atomic<float> gateThreshold{-55}, gateAttack{5}, gateRelease{120};
    std::atomic<float> compressorThreshold{-18}, compressorRatio{3}, compressorAttack{10}, compressorRelease{100};
    std::atomic<float> hpFreq{70}, lpFreq{18000}, noiseReduction{0.25f};
    std::atomic<float> distortion{0}, chorus{0}, flanger{0}, delay{0}, reverb{0}, ringMod{0}, bitCrush{0};
    std::atomic<bool> bypass{false}, muted{false}, gateEnabled{true}, cleanupEnabled{true}, compressorEnabled{true}, limiterEnabled{true}, phaseInvert{false};
};

class PitchProcessor {
public:
    void prepare(double sampleRate, int maximumBlock);
    void reset() noexcept;
    void setSemitones(float semitones) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
private:
    static constexpr int delaySize = 8192;
    std::array<float, delaySize> delay{};
    int write = 0; float phase = 0, phaseInc = 0; double rate = 48000;
    float readDelay(float samples) const noexcept;
};

class VoiceProcessor {
public:
    explicit VoiceProcessor(Parameters& p) : params(p) {}
    void prepare(double sampleRate, int blockSize, int channels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer) noexcept;
    float inputPeak() const noexcept { return inPeak.load(); }
    float outputPeak() const noexcept { return outPeak.load(); }
private:
    Parameters& params;
    PitchProcessor pitch;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float> delay{96000};
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> dry, wet;
    juce::LinearSmoothedValue<float> inputGain, outputGain, mix;
    juce::LinearSmoothedValue<float> formantAmount;
    std::atomic<float> inPeak{0}, outPeak{0};
    double sampleRate = 48000; float gateEnvelope = 0, ringPhase = 0, delayFeedback = 0;
    std::array<float, 2> hpX{}, hpY{}, lpY{}, formantLow{}, dcX{}, dcY{};
};
}
