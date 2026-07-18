#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

namespace vox {

struct Parameters {
  std::atomic<float> inputGainDb{0.0f}, outputGainDb{0.0f}, mix{1.0f};
  std::atomic<float> pitchSemitones{0.0f}, fineCents{0.0f}, formant{0.0f};

  std::atomic<float> gateThreshold{-58.0f}, gateAttack{5.0f},
      gateRelease{160.0f}, gateHold{70.0f};
  std::atomic<float> compressorThreshold{-18.0f}, compressorRatio{3.0f},
      compressorAttack{8.0f}, compressorRelease{120.0f};

  std::atomic<float> hpFreq{70.0f}, lpFreq{18000.0f},
      noiseReduction{0.25f}, deEsserAmount{0.35f};
  std::atomic<float> bassDb{0.0f}, midDb{0.0f}, trebleDb{0.0f};
  std::atomic<float> agcAmount{0.0f}, multibandAmount{0.0f};

  std::atomic<float> distortion{0.0f}, chorus{0.0f}, flanger{0.0f},
      delay{0.0f}, reverb{0.0f}, ringMod{0.0f}, bitCrush{0.0f};

  std::atomic<bool> bypass{false}, muted{false}, gateEnabled{true},
      cleanupEnabled{true}, compressorEnabled{true}, limiterEnabled{true},
      phaseInvert{false}, formantPreserve{true}, economyMode{false};
};

class PitchProcessor {
public:
  void prepare(double sampleRate, int maximumBlock, int channels);
  void reset() noexcept;
  void setSemitones(float semitones, bool preserveFormants) noexcept;
  void process(juce::AudioBuffer<float> &, int numSamples) noexcept;

private:
  static constexpr int delaySize = 8192;
  static constexpr int maximumChannels = 2;
  static constexpr int lpcOrder = 10;
  static constexpr float grainSize = 2048.0f;

  using ChannelDelay = std::array<float, delaySize>;
  using LpcCoefficients = std::array<float, lpcOrder + 1>;
  using LpcHistory = std::array<float, lpcOrder>;

  void updateLpc(const juce::AudioBuffer<float> &, int numSamples) noexcept;
  float readDelay(int channel, float samples) const noexcept;
  static void pushHistory(LpcHistory &, float value) noexcept;

  std::array<ChannelDelay, maximumChannels> delay{};
  std::array<LpcCoefficients, maximumChannels> lpc{};
  std::array<LpcHistory, maximumChannels> analysisHistory{}, synthesisHistory{};
  int writePosition = 0;
  int preparedChannels = 1;
  float phase = 0.0f;
  float currentRatio = 1.0f;
  float targetRatio = 1.0f;
  float pitchMix = 0.0f;
  float targetPitchMix = 0.0f;
  float ratioSmoothing = 1.0f;
  float mixSmoothing = 1.0f;
  bool formantPreserving = true;
  double rate = 48000.0;
};

class VoiceProcessor {
public:
  explicit VoiceProcessor(Parameters &p) : params(p) {}

  void prepare(double sampleRate, int blockSize, int channels);
  void reset();
  void process(juce::AudioBuffer<float> &buffer) noexcept;
  void process(juce::AudioBuffer<float> &buffer, int numSamples) noexcept;

  float inputPeak() const noexcept { return inPeak.load(); }
  float outputPeak() const noexcept { return outPeak.load(); }

private:
  static constexpr int maximumChannels = 2;

  void processCleanup(juce::AudioBuffer<float> &, int) noexcept;
  void processTone(juce::AudioBuffer<float> &, int) noexcept;
  void processGate(juce::AudioBuffer<float> &, int) noexcept;
  void processDeEsser(juce::AudioBuffer<float> &, int) noexcept;
  void processAgc(juce::AudioBuffer<float> &, int) noexcept;
  void processMultiband(juce::AudioBuffer<float> &, int, float) noexcept;
  void processCreativeEffects(juce::AudioBuffer<float> &, int) noexcept;
  static float coefficient(float milliseconds, double sampleRate) noexcept;

  Parameters &params;
  PitchProcessor pitch;
  juce::dsp::Compressor<float> compressor;
  juce::dsp::Limiter<float> limiter;
  juce::dsp::Chorus<float> chorus;
  juce::dsp::DelayLine<float> delay{96000};
  juce::dsp::DelayLine<float> flangerDelay{4096};
  juce::dsp::Reverb reverb;
  juce::AudioBuffer<float> dry;

  juce::LinearSmoothedValue<float> inputGain, outputGain, mix, formantAmount,
      distortionAmount, chorusAmount, delayAmount, reverbAmount,
      bitCrushAmount;

  std::atomic<float> inPeak{0.0f}, outPeak{0.0f};
  double sampleRate = 48000.0;
  int preparedChannels = 1;

  float gateDetector = 0.0f;
  float gateEnvelope = 0.0f;
  int gateHoldRemaining = 0;
  bool gateOpen = false;

  float noiseFloor = 0.001f;
  float noiseGain = 1.0f;
  float deEsserBandPower = 0.0f;
  float deEsserBroadPower = 0.0f;
  float deEsserGain = 1.0f;
  float agcPower = 0.0f;
  float agcGain = 1.0f;
  float multibandLowEnvelope = 0.0f;
  float multibandHighEnvelope = 0.0f;
  float multibandLowGain = 1.0f;
  float multibandHighGain = 1.0f;

  float ringPhase = 0.0f;
  float flangerPhase = 0.0f;

  std::array<float, maximumChannels> hpX{}, hpY{}, lpY{}, toneLow{},
      toneHigh{}, formantLow{}, dcX{}, dcY{}, deEsserLow5{},
      deEsserLow9{}, multibandLow{}, delayDamping{}, distortionPrevious{},
      distortionLowpass{}, bitCrushHeld{};
};

} // namespace vox
