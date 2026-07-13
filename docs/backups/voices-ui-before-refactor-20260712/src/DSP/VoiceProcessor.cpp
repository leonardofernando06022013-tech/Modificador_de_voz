#include "VoiceProcessor.h"
#include <cmath>

namespace vox {
void PitchProcessor::prepare(double sr, int) { rate = sr; reset(); }
void PitchProcessor::reset() noexcept { delay.fill(0); write = 0; phase = 0; }
void PitchProcessor::setSemitones(float st) noexcept {
    const float ratio = std::pow(2.0f, juce::jlimit(-12.0f, 12.0f, st) / 12.0f);
    phaseInc = (1.0f - ratio) / 2048.0f;
}
float PitchProcessor::readDelay(float d) const noexcept {
    float pos = static_cast<float>(write) - d;
    while (pos < 0) pos += delaySize;
    const int a = static_cast<int>(pos) & (delaySize - 1), b = (a + 1) & (delaySize - 1);
    const float f = pos - std::floor(pos); return delay[static_cast<size_t>(a)] + f * (delay[static_cast<size_t>(b)] - delay[static_cast<size_t>(a)]);
}
void PitchProcessor::process(juce::AudioBuffer<float>& b) noexcept {
    const int n = b.getNumSamples();
    for (int i=0;i<n;++i) {
        float mono=0; for(int c=0;c<b.getNumChannels();++c) mono += b.getSample(c,i); mono /= static_cast<float>(juce::jmax(1,b.getNumChannels()));
        delay[static_cast<size_t>(write)] = mono;
        phase += phaseInc; phase -= std::floor(phase);
        const float p2 = std::fmod(phase + 0.5f, 1.0f);
        const float w1 = 0.5f - 0.5f*std::cos(juce::MathConstants<float>::twoPi*phase);
        const float w2 = 0.5f - 0.5f*std::cos(juce::MathConstants<float>::twoPi*p2);
        const float y = readDelay(64.0f + phase*2048.0f)*w1 + readDelay(64.0f + p2*2048.0f)*w2;
        for(int c=0;c<b.getNumChannels();++c) b.setSample(c,i,y); write=(write+1)&(delaySize-1);
    }
}

void VoiceProcessor::prepare(double sr, int block, int channels) {
    sampleRate=sr; juce::dsp::ProcessSpec spec{sr,static_cast<juce::uint32>(block),static_cast<juce::uint32>(channels)};
    pitch.prepare(sr,block); compressor.prepare(spec); limiter.prepare(spec); chorus.prepare(spec); delay.prepare(spec); reverb.prepare(spec);
    dry.setSize(channels,block); wet.setSize(channels,block);
    inputGain.reset(sr,0.02); outputGain.reset(sr,0.02); mix.reset(sr,0.02); reset();
}
void VoiceProcessor::reset(){ pitch.reset(); compressor.reset(); limiter.reset(); chorus.reset(); delay.reset(); reverb.reset(); gateEnvelope=0; hpX.fill(0); hpY.fill(0); lpY.fill(0); }
void VoiceProcessor::process(juce::AudioBuffer<float>& b) noexcept {
    juce::ScopedNoDenormals noDenormals; const int n=b.getNumSamples(), ch=b.getNumChannels(); if(n==0||ch==0)return;
    inPeak.store(b.getMagnitude(0,n),std::memory_order_relaxed); if(params.muted.load()){b.clear();outPeak.store(0);return;} if(params.bypass.load()){outPeak.store(inPeak.load());return;}
    inputGain.setTargetValue(juce::Decibels::decibelsToGain(params.inputGainDb.load())); outputGain.setTargetValue(juce::Decibels::decibelsToGain(params.outputGainDb.load())); mix.setTargetValue(juce::jlimit(0.0f,1.0f,params.mix.load()));
    for(int i=0;i<n;++i){const float g=inputGain.getNextValue()*(params.phaseInvert.load()?-1.0f:1.0f);for(int c=0;c<ch;++c)b.setSample(c,i,b.getSample(c,i)*g);} dry.makeCopyOf(b,true);
    if(params.cleanupEnabled.load()){
        const float hp=juce::jlimit(20.0f,2000.0f,params.hpFreq.load()),lp=juce::jlimit(2000.0f,20000.0f,params.lpFreq.load());
        const float hpRC=1.0f/(juce::MathConstants<float>::twoPi*hp),dt=1.0f/static_cast<float>(sampleRate),ha=hpRC/(hpRC+dt),la=1.0f-std::exp(-juce::MathConstants<float>::twoPi*lp/static_cast<float>(sampleRate));
        for(int c=0;c<ch;++c)for(int i=0;i<n;++i){const float x=b.getSample(c,i);hpY[(size_t)c]=ha*(hpY[(size_t)c]+x-hpX[(size_t)c]);hpX[(size_t)c]=x;lpY[(size_t)c]+=la*(hpY[(size_t)c]-lpY[(size_t)c]);b.setSample(c,i,lpY[(size_t)c]);}
        const float nr=juce::jlimit(0.0f,1.0f,params.noiseReduction.load()); for(int i=0;i<n;++i)for(int c=0;c<ch;++c){float x=b.getSample(c,i);const float floor=0.004f*nr;b.setSample(c,i,std::abs(x)<floor?x*(1.0f-nr):x);}
    }
    if(params.gateEnabled.load()){const float threshold=juce::Decibels::decibelsToGain(params.gateThreshold.load());const float a=std::exp(-1.0f/(0.001f*juce::jmax(0.1f,params.gateAttack.load())*static_cast<float>(sampleRate)));const float r=std::exp(-1.0f/(0.001f*juce::jmax(1.0f,params.gateRelease.load())*static_cast<float>(sampleRate)));for(int i=0;i<n;++i){float level=0;for(int c=0;c<ch;++c)level=juce::jmax(level,std::abs(b.getSample(c,i)));const float target=level>=threshold?1.0f:0.0f;gateEnvelope=(target>gateEnvelope?a:r)*gateEnvelope+(1.0f-(target>gateEnvelope?a:r))*target;for(int c=0;c<ch;++c)b.setSample(c,i,b.getSample(c,i)*gateEnvelope);}}
    if(params.compressorEnabled.load()){compressor.setThreshold(params.compressorThreshold.load());compressor.setRatio(juce::jmax(1.0f,params.compressorRatio.load()));compressor.setAttack(params.compressorAttack.load());compressor.setRelease(params.compressorRelease.load());juce::dsp::AudioBlock<float> block(b);juce::dsp::ProcessContextReplacing<float> ctx(block);compressor.process(ctx);}
    pitch.setSemitones(params.pitchSemitones.load()+params.fineCents.load()/100.0f); pitch.process(b);
    const float dist=params.distortion.load(); if(dist>0)for(int c=0;c<ch;++c)for(int i=0;i<n;++i)b.setSample(c,i,std::tanh(b.getSample(c,i)*(1+dist*20))/std::tanh(1+dist*20));
    const float ring=params.ringMod.load(); if(ring>0)for(int i=0;i<n;++i){const float mod=std::sin(ringPhase);ringPhase=std::fmod(ringPhase+juce::MathConstants<float>::twoPi*(30+ring*470)/static_cast<float>(sampleRate),juce::MathConstants<float>::twoPi);for(int c=0;c<ch;++c)b.setSample(c,i,b.getSample(c,i)*((1-ring)+ring*mod));}
    const float crush=params.bitCrush.load();if(crush>0){const float steps=std::pow(2.0f,16.0f-crush*12.0f);for(int c=0;c<ch;++c)for(int i=0;i<n;++i)b.setSample(c,i,std::round(b.getSample(c,i)*steps)/steps);}
    const float cho=params.chorus.load();chorus.setMix(cho);chorus.setRate(0.4f);chorus.setDepth(0.35f);chorus.setCentreDelay(12);chorus.setFeedback(0.1f);{juce::dsp::AudioBlock<float> block(b);juce::dsp::ProcessContextReplacing<float> ctx(block);chorus.process(ctx);}
    const float dMix=params.delay.load();for(int i=0;i<n;++i)for(int c=0;c<ch;++c){const float delayed=delay.popSample(c,static_cast<float>(sampleRate*0.22));const float x=b.getSample(c,i);delay.pushSample(c,x+delayed*0.3f);b.setSample(c,i,x*(1-dMix)+delayed*dMix);}
    juce::dsp::Reverb::Parameters rp;rp.wetLevel=params.reverb.load()*0.4f;rp.dryLevel=1-rp.wetLevel;rp.roomSize=0.45f;reverb.setParameters(rp);{juce::dsp::AudioBlock<float> block(b);juce::dsp::ProcessContextReplacing<float> ctx(block);reverb.process(ctx);}
    for(int i=0;i<n;++i){const float m=mix.getNextValue(),g=outputGain.getNextValue();for(int c=0;c<ch;++c)b.setSample(c,i,(dry.getSample(c,i)*(1-m)+b.getSample(c,i)*m)*g);}
    if(params.limiterEnabled.load()){limiter.setThreshold(-0.3f);limiter.setRelease(50);juce::dsp::AudioBlock<float> block(b);juce::dsp::ProcessContextReplacing<float> ctx(block);limiter.process(ctx);} outPeak.store(b.getMagnitude(0,n),std::memory_order_relaxed);
}
}
