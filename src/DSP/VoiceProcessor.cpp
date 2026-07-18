#include "VoiceProcessor.h"

#include <algorithm>
#include <cmath>

namespace vox {
namespace {
constexpr float epsilon = 1.0e-9f;

float smoothStep(float value) noexcept {
  value = juce::jlimit(0.0f, 1.0f, value);
  return value * value * (3.0f - 2.0f * value);
}

float linkedPeak(const juce::AudioBuffer<float> &buffer, int sample,
                 int channels) noexcept {
  float peak = 0.0f;
  for (int channel = 0; channel < channels; ++channel)
    peak = juce::jmax(peak, std::abs(buffer.getSample(channel, sample)));
  return peak;
}
} // namespace

void PitchProcessor::prepare(double sampleRate, int, int channels) {
  rate = sampleRate;
  preparedChannels = juce::jlimit(1, maximumChannels, channels);
  ratioSmoothing = 1.0f -
                   std::exp(-1.0f / (0.035f * static_cast<float>(rate)));
  mixSmoothing = 1.0f -
                 std::exp(-1.0f / (0.04f * static_cast<float>(rate)));
  reset();
}

void PitchProcessor::reset() noexcept {
  for (auto &channel : delay)
    channel.fill(0.0f);
  for (auto &coefficients : lpc) {
    coefficients.fill(0.0f);
    coefficients[0] = 1.0f;
  }
  for (auto &history : analysisHistory)
    history.fill(0.0f);
  for (auto &history : synthesisHistory)
    history.fill(0.0f);
  writePosition = 0;
  phase = 0.0f;
  currentRatio = targetRatio = 1.0f;
  pitchMix = targetPitchMix = 0.0f;
}

void PitchProcessor::setSemitones(float semitones,
                                  bool preserveFormants) noexcept {
  formantPreserving = preserveFormants;
  const float limit = preserveFormants ? 6.0f : 12.0f;
  const float safeSemitones = juce::jlimit(-limit, limit, semitones);
  targetRatio = std::pow(2.0f, safeSemitones / 12.0f);
  targetPitchMix = std::abs(safeSemitones) > 0.015f ? 1.0f : 0.0f;
}

void PitchProcessor::pushHistory(LpcHistory &history, float value) noexcept {
  for (int i = lpcOrder - 1; i > 0; --i)
    history[static_cast<size_t>(i)] =
        history[static_cast<size_t>(i - 1)];
  history[0] = value;
}

void PitchProcessor::updateLpc(const juce::AudioBuffer<float> &buffer,
                               int numSamples) noexcept {
  const bool analyse = formantPreserving &&
                       (targetPitchMix > 0.0f || pitchMix > 0.001f) &&
                       numSamples > lpcOrder * 2;

  for (int channel = 0; channel < preparedChannels; ++channel) {
    LpcCoefficients target{};
    target[0] = 1.0f;

    if (analyse) {
      std::array<double, lpcOrder + 1> correlation{};
      const auto *samples = buffer.getReadPointer(channel);
      for (int lag = 0; lag <= lpcOrder; ++lag) {
        double sum = 0.0;
        for (int i = lag; i < numSamples; ++i)
          sum += static_cast<double>(samples[i]) * samples[i - lag];
        correlation[static_cast<size_t>(lag)] = sum;
      }

      if (correlation[0] > 1.0e-8) {
        std::array<double, lpcOrder + 1> coefficients{};
        coefficients[0] = 1.0;
        double error = correlation[0];

        for (int order = 1; order <= lpcOrder; ++order) {
          double numerator = correlation[static_cast<size_t>(order)];
          for (int j = 1; j < order; ++j)
            numerator += coefficients[static_cast<size_t>(j)] *
                         correlation[static_cast<size_t>(order - j)];

          const double reflection =
              juce::jlimit(-0.96, 0.96, -numerator / juce::jmax(error, 1.0e-12));
          auto previous = coefficients;
          coefficients[static_cast<size_t>(order)] = reflection;
          for (int j = 1; j < order; ++j)
            coefficients[static_cast<size_t>(j)] =
                previous[static_cast<size_t>(j)] +
                reflection * previous[static_cast<size_t>(order - j)];
          error *= 1.0 - reflection * reflection;
        }

        for (int i = 1; i <= lpcOrder; ++i)
          target[static_cast<size_t>(i)] =
              static_cast<float>(coefficients[static_cast<size_t>(i)]);
      }
    }

    // Block-to-block interpolation avoids clicks when the vocal envelope moves.
    for (int i = 1; i <= lpcOrder; ++i)
      lpc[static_cast<size_t>(channel)][static_cast<size_t>(i)] +=
          0.18f * (target[static_cast<size_t>(i)] -
                   lpc[static_cast<size_t>(channel)][static_cast<size_t>(i)]);
  }
}

float PitchProcessor::readDelay(int channel, float samples) const noexcept {
  float position = static_cast<float>(writePosition) - samples;
  while (position < 0.0f)
    position += static_cast<float>(delaySize);
  while (position >= static_cast<float>(delaySize))
    position -= static_cast<float>(delaySize);

  const int first = static_cast<int>(position) & (delaySize - 1);
  const int second = (first + 1) & (delaySize - 1);
  const float fraction = position - std::floor(position);
  const auto &channelDelay = delay[static_cast<size_t>(channel)];
  return channelDelay[static_cast<size_t>(first)] +
         fraction * (channelDelay[static_cast<size_t>(second)] -
                     channelDelay[static_cast<size_t>(first)]);
}

void PitchProcessor::process(juce::AudioBuffer<float> &buffer,
                             int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  if (channels <= 0 || numSamples <= 0)
    return;

  updateLpc(buffer, numSamples);

  for (int sample = 0; sample < numSamples; ++sample) {
    currentRatio += ratioSmoothing * (targetRatio - currentRatio);
    pitchMix += mixSmoothing * (targetPitchMix - pitchMix);
    phase += (1.0f - currentRatio) / grainSize;
    phase -= std::floor(phase);

    const float secondPhase = phase < 0.5f ? phase + 0.5f : phase - 0.5f;
    const float firstWeight =
        0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * phase);
    const float secondWeight = 1.0f - firstWeight;

    for (int channel = 0; channel < channels; ++channel) {
      const size_t index = static_cast<size_t>(channel);
      const float input = buffer.getSample(channel, sample);
      float residual = input;

      if (formantPreserving) {
        for (int order = 1; order <= lpcOrder; ++order)
          residual += lpc[index][static_cast<size_t>(order)] *
                      analysisHistory[index][static_cast<size_t>(order - 1)];
      }
      pushHistory(analysisHistory[index], input);
      delay[index][static_cast<size_t>(writePosition)] = residual;

      float shifted =
          readDelay(channel, 64.0f + phase * grainSize) * firstWeight +
          readDelay(channel, 64.0f + secondPhase * grainSize) * secondWeight;

      if (formantPreserving) {
        for (int order = 1; order <= lpcOrder; ++order)
          shifted -= lpc[index][static_cast<size_t>(order)] *
                     synthesisHistory[index][static_cast<size_t>(order - 1)];
        shifted = juce::jlimit(-4.0f, 4.0f, shifted);
      }
      pushHistory(synthesisHistory[index], shifted);
      buffer.setSample(channel, sample,
                       input + pitchMix * (shifted - input));
    }

    writePosition = (writePosition + 1) & (delaySize - 1);
  }
}

float VoiceProcessor::coefficient(float milliseconds,
                                  double currentSampleRate) noexcept {
  return std::exp(-1.0f /
                  (0.001f * juce::jmax(0.05f, milliseconds) *
                   static_cast<float>(currentSampleRate)));
}

void VoiceProcessor::prepare(double newSampleRate, int blockSize,
                             int channels) {
  sampleRate = newSampleRate;
  preparedChannels = juce::jlimit(1, maximumChannels, channels);
  const juce::dsp::ProcessSpec spec{
      newSampleRate, static_cast<juce::uint32>(blockSize),
      static_cast<juce::uint32>(preparedChannels)};

  pitch.prepare(newSampleRate, blockSize, preparedChannels);
  compressor.prepare(spec);
  limiter.prepare(spec);
  chorus.prepare(spec);
  delay.prepare(spec);
  flangerDelay.prepare(spec);
  reverb.prepare(spec);
  dry.setSize(preparedChannels, blockSize, false, true, false);

  inputGain.reset(newSampleRate, 0.02);
  outputGain.reset(newSampleRate, 0.02);
  mix.reset(newSampleRate, 0.02);
  formantAmount.reset(newSampleRate, 0.04);
  distortionAmount.reset(newSampleRate, 0.04);
  chorusAmount.reset(newSampleRate, 0.06);
  delayAmount.reset(newSampleRate, 0.06);
  reverbAmount.reset(newSampleRate, 0.08);
  bitCrushAmount.reset(newSampleRate, 0.04);
  reset();
}

void VoiceProcessor::reset() {
  pitch.reset();
  compressor.reset();
  limiter.reset();
  chorus.reset();
  delay.reset();
  flangerDelay.reset();
  reverb.reset();
  dry.clear();

  gateDetector = 0.0f;
  gateEnvelope = 0.0f;
  gateHoldRemaining = 0;
  gateOpen = false;
  noiseFloor = 0.001f;
  noiseGain = 1.0f;
  deEsserBandPower = deEsserBroadPower = 0.0f;
  deEsserGain = 1.0f;
  agcPower = 0.0f;
  agcGain = 1.0f;
  multibandLowEnvelope = multibandHighEnvelope = 0.0f;
  multibandLowGain = multibandHighGain = 1.0f;
  ringPhase = flangerPhase = 0.0f;

  for (auto *state : {&hpX, &hpY, &lpY, &toneLow, &toneHigh, &formantLow,
                      &dcX, &dcY, &deEsserLow5, &deEsserLow9,
                      &multibandLow, &delayDamping, &distortionPrevious,
                      &distortionLowpass, &bitCrushHeld})
    state->fill(0.0f);
}

void VoiceProcessor::process(juce::AudioBuffer<float> &buffer) noexcept {
  process(buffer, buffer.getNumSamples());
}

void VoiceProcessor::processCleanup(juce::AudioBuffer<float> &buffer,
                                    int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float hp = juce::jlimit(20.0f, 2000.0f, params.hpFreq.load());
  const float lp = juce::jlimit(2000.0f, 20000.0f, params.lpFreq.load());
  const float dt = 1.0f / static_cast<float>(sampleRate);
  const float hpRc = 1.0f / (juce::MathConstants<float>::twoPi * hp);
  const float hpAlpha = hpRc / (hpRc + dt);
  const float lpAlpha =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * lp * dt);
  const float reduction =
      juce::jlimit(0.0f, 1.0f, params.noiseReduction.load());
  const float floorGain = 1.0f - reduction * 0.78f;
  const float noiseRise = coefficient(1200.0f, sampleRate);
  const float noiseFall = coefficient(40.0f, sampleRate);
  const float gainAttack = coefficient(6.0f, sampleRate);
  const float gainRelease = coefficient(90.0f, sampleRate);

  for (int sample = 0; sample < numSamples; ++sample) {
    float peak = 0.0f;
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float input = buffer.getSample(channel, sample);
      hpY[state] = hpAlpha * (hpY[state] + input - hpX[state]);
      hpX[state] = input;
      lpY[state] += lpAlpha * (hpY[state] - lpY[state]);
      peak = juce::jmax(peak, std::abs(lpY[state]));
    }

    const float noiseCoefficient =
        peak < noiseFloor
            ? noiseFall
            : (peak < 0.02f || peak < noiseFloor * 3.0f ? noiseRise : 1.0f);
    noiseFloor = noiseCoefficient * noiseFloor +
                 (1.0f - noiseCoefficient) * peak;
    noiseFloor = juce::jlimit(1.0e-5f, 0.08f, noiseFloor);
    const float threshold = juce::jmax(1.0e-4f,
                                      noiseFloor * (1.8f + 4.2f * reduction));
    const float activity = smoothStep(peak / threshold);
    const float targetGain = floorGain + (1.0f - floorGain) * activity;
    const float gainCoefficient =
        targetGain > noiseGain ? gainAttack : gainRelease;
    noiseGain = gainCoefficient * noiseGain +
                (1.0f - gainCoefficient) * targetGain;

    for (int channel = 0; channel < channels; ++channel)
      buffer.setSample(channel, sample,
                       lpY[static_cast<size_t>(channel)] * noiseGain);
  }
}

void VoiceProcessor::processTone(juce::AudioBuffer<float> &buffer,
                                 int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float lowAlpha =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * 220.0f /
                      static_cast<float>(sampleRate));
  const float highAlpha =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * 4800.0f /
                      static_cast<float>(sampleRate));
  const float lowGain = juce::Decibels::decibelsToGain(
      juce::jlimit(-6.0f, 6.0f, params.bassDb.load()));
  const float midGain = juce::Decibels::decibelsToGain(
      juce::jlimit(-6.0f, 6.0f, params.midDb.load()));
  const float highGain = juce::Decibels::decibelsToGain(
      juce::jlimit(-6.0f, 6.0f, params.trebleDb.load()));

  for (int channel = 0; channel < channels; ++channel) {
    const size_t state = static_cast<size_t>(channel);
    for (int sample = 0; sample < numSamples; ++sample) {
      const float input = buffer.getSample(channel, sample);
      toneLow[state] += lowAlpha * (input - toneLow[state]);
      toneHigh[state] += highAlpha * (input - toneHigh[state]);
      const float high = input - toneHigh[state];
      const float middle = input - toneLow[state] - high;
      buffer.setSample(channel, sample,
                       toneLow[state] * lowGain + middle * midGain +
                           high * highGain);
    }
  }
}

void VoiceProcessor::processGate(juce::AudioBuffer<float> &buffer,
                                 int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float openThreshold = juce::Decibels::decibelsToGain(
      juce::jlimit(-80.0f, -3.0f, params.gateThreshold.load()));
  const float closeThreshold =
      openThreshold * juce::Decibels::decibelsToGain(-4.0f);
  const float detectorAttack = coefficient(1.5f, sampleRate);
  const float detectorRelease = coefficient(65.0f, sampleRate);
  const float gainAttack = coefficient(
      juce::jlimit(0.5f, 100.0f, params.gateAttack.load()), sampleRate);
  const float gainRelease = coefficient(
      juce::jlimit(10.0f, 1000.0f, params.gateRelease.load()), sampleRate);
  const int holdSamples = static_cast<int>(
      0.001 * sampleRate * juce::jlimit(0.0f, 500.0f, params.gateHold.load()));

  for (int sample = 0; sample < numSamples; ++sample) {
    const float peak = linkedPeak(buffer, sample, channels);
    const float detectorCoefficient =
        peak > gateDetector ? detectorAttack : detectorRelease;
    gateDetector = detectorCoefficient * gateDetector +
                   (1.0f - detectorCoefficient) * peak;

    if (gateDetector >= openThreshold) {
      gateOpen = true;
      gateHoldRemaining = holdSamples;
    } else if (gateOpen && gateHoldRemaining > 0) {
      --gateHoldRemaining;
    } else if (gateDetector < closeThreshold) {
      gateOpen = false;
    }

    const float target = gateOpen ? 1.0f : 0.03f;
    const float gainCoefficient = target > gateEnvelope ? gainAttack : gainRelease;
    gateEnvelope = gainCoefficient * gateEnvelope +
                   (1.0f - gainCoefficient) * target;
    for (int channel = 0; channel < channels; ++channel)
      buffer.setSample(channel, sample,
                       buffer.getSample(channel, sample) * gateEnvelope);
  }
}

void VoiceProcessor::processDeEsser(juce::AudioBuffer<float> &buffer,
                                    int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float amount =
      juce::jlimit(0.0f, 1.0f, params.deEsserAmount.load());
  if (amount <= 0.0001f)
    return;

  const float alpha5 =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * 5000.0f /
                      static_cast<float>(sampleRate));
  const float alpha9 =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * 9000.0f /
                      static_cast<float>(sampleRate));
  const float detector = 1.0f - coefficient(8.0f, sampleRate);
  const float attack = coefficient(3.0f, sampleRate);
  const float release = coefficient(85.0f, sampleRate);
  const float maximumReductionDb = 8.0f * amount;

  for (int sample = 0; sample < numSamples; ++sample) {
    std::array<float, maximumChannels> band{};
    float bandPeak = 0.0f;
    float broadPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float input = buffer.getSample(channel, sample);
      deEsserLow5[state] += alpha5 * (input - deEsserLow5[state]);
      deEsserLow9[state] += alpha9 * (input - deEsserLow9[state]);
      // The difference of the two one-pole low-passes is the desired
      // 5-9 kHz band scaled by (fHigh - fLow) / fHigh. Compensate that
      // attenuation so the requested reduction is meaningful in dB.
      band[state] = 2.1f * (deEsserLow9[state] - deEsserLow5[state]);
      bandPeak = juce::jmax(bandPeak, std::abs(band[state]));
      broadPeak = juce::jmax(broadPeak, std::abs(input));
    }

    deEsserBandPower +=
        detector * (bandPeak * bandPeak - deEsserBandPower);
    deEsserBroadPower +=
        detector * (broadPeak * broadPeak - deEsserBroadPower);
    const float bandRms = std::sqrt(juce::jmax(0.0f, deEsserBandPower));
    const float broadRms = std::sqrt(juce::jmax(0.0f, deEsserBroadPower));
    const float spectralRatio = bandRms / (broadRms + 0.0001f);
    const float prominence = smoothStep((spectralRatio - 0.14f) / 0.32f);
    const float audible = smoothStep((bandRms - 0.0015f) / 0.025f);
    const float targetGain = juce::Decibels::decibelsToGain(
        -maximumReductionDb * prominence * audible);
    const float gainCoefficient =
        targetGain < deEsserGain ? attack : release;
    deEsserGain = gainCoefficient * deEsserGain +
                   (1.0f - gainCoefficient) * targetGain;

    for (int channel = 0; channel < channels; ++channel) {
      const float input = buffer.getSample(channel, sample);
      buffer.setSample(channel, sample,
                       input - (1.0f - deEsserGain) *
                                   band[static_cast<size_t>(channel)]);
    }
  }
}

void VoiceProcessor::processAgc(juce::AudioBuffer<float> &buffer,
                                int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float amount = juce::jlimit(0.0f, 1.0f, params.agcAmount.load());
  const float detector = 1.0f - coefficient(180.0f, sampleRate);
  const float lowerGain = coefficient(90.0f, sampleRate);
  const float raiseGain = coefficient(850.0f, sampleRate);
  constexpr float targetRms = 0.1259f; // -18 dBFS

  for (int sample = 0; sample < numSamples; ++sample) {
    const float peak = linkedPeak(buffer, sample, channels);
    agcPower += detector * (peak * peak - agcPower);
    const float rms = std::sqrt(juce::jmax(0.0f, agcPower));
    float requested = 1.0f;
    if (rms > 0.008f)
      requested = juce::jlimit(0.55f, 2.0f, targetRms / rms);
    const float target = 1.0f + amount * (requested - 1.0f);
    const float gainCoefficient = target < agcGain ? lowerGain : raiseGain;
    agcGain = gainCoefficient * agcGain +
              (1.0f - gainCoefficient) * target;
    for (int channel = 0; channel < channels; ++channel)
      buffer.setSample(channel, sample,
                       buffer.getSample(channel, sample) * agcGain);
  }
}

void VoiceProcessor::processMultiband(juce::AudioBuffer<float> &buffer,
                                      int numSamples, float amount) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const float splitAlpha =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * 260.0f /
                      static_cast<float>(sampleRate));
  const float attack = coefficient(8.0f, sampleRate);
  const float release = coefficient(140.0f, sampleRate);
  constexpr float lowThreshold = 0.1585f;  // -16 dBFS
  constexpr float highThreshold = 0.1f;   // -20 dBFS

  for (int sample = 0; sample < numSamples; ++sample) {
    std::array<float, maximumChannels> low{};
    std::array<float, maximumChannels> high{};
    float lowPeak = 0.0f;
    float highPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float input = buffer.getSample(channel, sample);
      multibandLow[state] += splitAlpha * (input - multibandLow[state]);
      low[state] = multibandLow[state];
      high[state] = input - low[state];
      lowPeak = juce::jmax(lowPeak, std::abs(low[state]));
      highPeak = juce::jmax(highPeak, std::abs(high[state]));
    }

    const float lowCoefficient =
        lowPeak > multibandLowEnvelope ? attack : release;
    const float highCoefficient =
        highPeak > multibandHighEnvelope ? attack : release;
    multibandLowEnvelope = lowCoefficient * multibandLowEnvelope +
                           (1.0f - lowCoefficient) * lowPeak;
    multibandHighEnvelope = highCoefficient * multibandHighEnvelope +
                            (1.0f - highCoefficient) * highPeak;

    const auto compressedGain = [](float envelope, float threshold,
                                   float ratio) noexcept {
      if (envelope <= threshold)
        return 1.0f;
      const float compressed = threshold + (envelope - threshold) / ratio;
      return compressed / (envelope + epsilon);
    };
    const float targetLow =
        compressedGain(multibandLowEnvelope, lowThreshold, 2.0f);
    const float targetHigh =
        compressedGain(multibandHighEnvelope, highThreshold, 2.4f);
    multibandLowGain += 0.02f * (targetLow - multibandLowGain);
    multibandHighGain += 0.02f * (targetHigh - multibandHighGain);

    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float original = low[state] + high[state];
      const float processed =
          low[state] * multibandLowGain + high[state] * multibandHighGain;
      buffer.setSample(channel, sample,
                       original + amount * (processed - original));
    }
  }
}

void VoiceProcessor::processCreativeEffects(juce::AudioBuffer<float> &buffer,
                                            int numSamples) noexcept {
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  const bool economy = params.economyMode.load();

  distortionAmount.setTargetValue(
      juce::jlimit(0.0f, 1.0f, params.distortion.load()));
  const float distortion = distortionAmount.skip(numSamples);
  if (distortion > 0.0001f) {
    const float drive = 1.0f + distortion * 16.0f;
    const float normalise = 1.0f / std::tanh(drive);
    const bool oversample = distortion > 0.2f && !economy;
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      for (int sample = 0; sample < numSamples; ++sample) {
        const float input = buffer.getSample(channel, sample);
        float shaped = std::tanh(input * drive) * normalise;
        if (oversample) {
          const float midpoint = 0.5f * (distortionPrevious[state] + input);
          const float first = std::tanh(midpoint * drive) * normalise;
          distortionLowpass[state] +=
              0.55f * (0.5f * (first + shaped) - distortionLowpass[state]);
          shaped = distortionLowpass[state];
        }
        distortionPrevious[state] = input;
        buffer.setSample(channel, sample, shaped);
      }
    }
  }

  const float ring = juce::jlimit(0.0f, 1.0f, params.ringMod.load());
  if (ring > 0.0001f) {
    for (int sample = 0; sample < numSamples; ++sample) {
      const float modulation = std::sin(ringPhase);
      ringPhase += juce::MathConstants<float>::twoPi * (30.0f + ring * 470.0f) /
                   static_cast<float>(sampleRate);
      if (ringPhase >= juce::MathConstants<float>::twoPi)
        ringPhase -= juce::MathConstants<float>::twoPi;
      for (int channel = 0; channel < channels; ++channel)
        buffer.setSample(channel, sample,
                         buffer.getSample(channel, sample) *
                             ((1.0f - ring) + ring * modulation));
    }
  }

  bitCrushAmount.setTargetValue(
      juce::jlimit(0.0f, 1.0f, params.bitCrush.load()));
  const float crush = bitCrushAmount.skip(numSamples);
  if (crush > 0.0001f) {
    const float steps = std::pow(2.0f, 16.0f - crush * 10.0f);
    const float smoothing = 0.15f + 0.35f * (1.0f - crush);
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      for (int sample = 0; sample < numSamples; ++sample) {
        const float quantised =
            std::round(buffer.getSample(channel, sample) * steps) / steps;
        bitCrushHeld[state] += smoothing * (quantised - bitCrushHeld[state]);
        buffer.setSample(channel, sample, bitCrushHeld[state]);
      }
    }
  }

  const float flanger =
      juce::jlimit(0.0f, 1.0f, params.flanger.load());
  if (flanger > 0.0001f) {
    for (int sample = 0; sample < numSamples; ++sample) {
      const float lfo = 0.5f + 0.5f * std::sin(flangerPhase);
      flangerPhase += juce::MathConstants<float>::twoPi * 0.24f /
                      static_cast<float>(sampleRate);
      if (flangerPhase >= juce::MathConstants<float>::twoPi)
        flangerPhase -= juce::MathConstants<float>::twoPi;
      const float delaySamples =
          static_cast<float>(sampleRate) * (0.0008f + 0.0026f * lfo);
      for (int channel = 0; channel < channels; ++channel) {
        const float input = buffer.getSample(channel, sample);
        const float delayed = flangerDelay.popSample(channel, delaySamples);
        flangerDelay.pushSample(channel, input + delayed * 0.16f * flanger);
        buffer.setSample(channel, sample,
                         input * (1.0f - 0.22f * flanger) +
                             delayed * 0.45f * flanger);
      }
    }
  }

  chorusAmount.setTargetValue(juce::jlimit(0.0f, 1.0f, params.chorus.load()));
  const float chorusMix = 0.45f * chorusAmount.skip(numSamples);
  chorus.setMix(chorusMix);
  chorus.setRate(0.28f);
  chorus.setDepth(0.2f);
  chorus.setCentreDelay(9.0f);
  chorus.setFeedback(0.05f);
  {
    juce::dsp::AudioBlock<float> block(buffer);
    auto active = block.getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(active);
    chorus.process(context);
  }

  delayAmount.setTargetValue(juce::jlimit(0.0f, 1.0f, params.delay.load()));
  const float delayMix = delayAmount.skip(numSamples);
  float speechEnvelope = gateDetector;
  for (int sample = 0; sample < numSamples; ++sample) {
    const float duck =
        1.0f - 0.72f * smoothStep((speechEnvelope - 0.015f) / 0.12f);
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float delayed =
          delay.popSample(channel, static_cast<float>(sampleRate * 0.2));
      delayDamping[state] += 0.22f * (delayed - delayDamping[state]);
      const float input = buffer.getSample(channel, sample);
      delay.pushSample(channel, input + delayDamping[state] * 0.24f);
      buffer.setSample(channel, sample,
                       input + delayDamping[state] * delayMix * duck * 0.7f);
    }
  }

  reverbAmount.setTargetValue(
      juce::jlimit(0.0f, 1.0f, params.reverb.load()));
  const float reverbMix =
      juce::jmin(0.15f, reverbAmount.skip(numSamples) * 0.35f);
  juce::dsp::Reverb::Parameters reverbParameters;
  reverbParameters.roomSize = 0.34f;
  reverbParameters.damping = 0.58f;
  reverbParameters.wetLevel = reverbMix;
  reverbParameters.dryLevel = 1.0f - reverbMix;
  reverbParameters.width = 0.85f;
  reverbParameters.freezeMode = 0.0f;
  reverb.setParameters(reverbParameters);
  {
    juce::dsp::AudioBlock<float> block(buffer);
    auto active = block.getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(active);
    reverb.process(context);
  }
}

void VoiceProcessor::process(juce::AudioBuffer<float> &buffer,
                             int requestedSamples) noexcept {
  juce::ScopedNoDenormals noDenormals;
  const int numSamples =
      juce::jlimit(0, buffer.getNumSamples(), requestedSamples);
  const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
  if (numSamples == 0 || channels == 0)
    return;

  inPeak.store(buffer.getMagnitude(0, numSamples), std::memory_order_relaxed);
  if (params.muted.load()) {
    for (int channel = 0; channel < channels; ++channel)
      juce::FloatVectorOperations::clear(buffer.getWritePointer(channel),
                                         numSamples);
    outPeak.store(0.0f, std::memory_order_relaxed);
    return;
  }
  if (params.bypass.load()) {
    outPeak.store(inPeak.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
    return;
  }

  inputGain.setTargetValue(
      juce::Decibels::decibelsToGain(params.inputGainDb.load()));
  outputGain.setTargetValue(
      juce::Decibels::decibelsToGain(params.outputGainDb.load()));
  mix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.mix.load()));
  formantAmount.setTargetValue(
      juce::jlimit(-0.65f, 0.65f, params.formant.load()));

  const float polarity = params.phaseInvert.load() ? -1.0f : 1.0f;
  for (int sample = 0; sample < numSamples; ++sample) {
    const float gain = inputGain.getNextValue() * polarity;
    for (int channel = 0; channel < channels; ++channel) {
      const float value = buffer.getSample(channel, sample) * gain;
      buffer.setSample(channel, sample, value);
      dry.setSample(channel, sample, value);
    }
  }

  if (params.cleanupEnabled.load())
    processCleanup(buffer, numSamples);
  processTone(buffer, numSamples);
  if (params.gateEnabled.load())
    processGate(buffer, numSamples);
  processDeEsser(buffer, numSamples);

  if (!params.economyMode.load())
    processAgc(buffer, numSamples);

  if (params.compressorEnabled.load()) {
    compressor.setThreshold(juce::jlimit(
        -42.0f, -3.0f, params.compressorThreshold.load()));
    compressor.setRatio(
        juce::jlimit(1.0f, 8.0f, params.compressorRatio.load()));
    compressor.setAttack(
        juce::jlimit(0.5f, 80.0f, params.compressorAttack.load()));
    compressor.setRelease(
        juce::jlimit(20.0f, 800.0f, params.compressorRelease.load()));
    juce::dsp::AudioBlock<float> block(buffer);
    auto active = block.getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(active);
    compressor.process(context);
  }

  const float multiband = params.economyMode.load()
                              ? 0.0f
                              : juce::jlimit(0.0f, 1.0f,
                                            params.multibandAmount.load());
  if (multiband > 0.0001f)
    processMultiband(buffer, numSamples, multiband);

  const float pitchShift = params.pitchSemitones.load() +
                           params.fineCents.load() / 100.0f;
  const bool preserveFormants =
      params.formantPreserve.load() && !params.economyMode.load();
  pitch.setSemitones(pitchShift, preserveFormants);
  pitch.process(buffer, numSamples);

  const float formantCutoff = juce::jlimit(
      650.0f, 1900.0f, 1100.0f + params.formant.load() * 520.0f);
  const float formantAlpha =
      1.0f - std::exp(-juce::MathConstants<float>::twoPi * formantCutoff /
                      static_cast<float>(sampleRate));
  for (int sample = 0; sample < numSamples; ++sample) {
    const float amount = formantAmount.getNextValue();
    for (int channel = 0; channel < channels; ++channel) {
      const size_t state = static_cast<size_t>(channel);
      const float input = buffer.getSample(channel, sample);
      formantLow[state] += formantAlpha * (input - formantLow[state]);
      const float high = input - formantLow[state];
      const float shaped =
          amount >= 0.0f
              ? input + amount * (high * 0.48f - formantLow[state] * 0.08f)
              : input + (-amount) *
                            (formantLow[state] * 0.46f - high * 0.12f);
      buffer.setSample(channel, sample, shaped);
    }
  }

  processCreativeEffects(buffer, numSamples);

  for (int sample = 0; sample < numSamples; ++sample) {
    const float wetMix = mix.getNextValue();
    const float gain = outputGain.getNextValue();
    for (int channel = 0; channel < channels; ++channel) {
      const float value =
          (dry.getSample(channel, sample) * (1.0f - wetMix) +
           buffer.getSample(channel, sample) * wetMix) *
          gain;
      buffer.setSample(channel, sample, value);
    }
  }

  if (params.limiterEnabled.load()) {
    limiter.setThreshold(-0.5f);
    limiter.setRelease(70.0f);
    juce::dsp::AudioBlock<float> block(buffer);
    auto active = block.getSubBlock(0, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(active);
    limiter.process(context);
  }

  for (int channel = 0; channel < channels; ++channel) {
    const size_t state = static_cast<size_t>(channel);
    for (int sample = 0; sample < numSamples; ++sample) {
      const float input = buffer.getSample(channel, sample);
      const float output = input - dcX[state] + 0.995f * dcY[state];
      dcX[state] = input;
      dcY[state] = output;
      buffer.setSample(channel, sample,
                       juce::jlimit(-1.0f, 1.0f, output));
    }
  }

  outPeak.store(buffer.getMagnitude(0, numSamples),
                std::memory_order_relaxed);
}

} // namespace vox
