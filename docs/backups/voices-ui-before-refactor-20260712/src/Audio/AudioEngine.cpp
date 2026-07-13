#include "AudioEngine.h"
#include "App/AppPaths.h"
namespace vox {
AudioEngine::AudioEngine(){ startTimer(1500); }
AudioEngine::~AudioEngine(){ stopTimer(); devices.removeAudioCallback(this);if(auto state=devices.createStateXml())state->writeTo(AppPaths::data().getChildFile("audio-devices.xml"));devices.closeAudioDevice(); }
juce::String AudioEngine::initialise(){
    auto file=AppPaths::data().getChildFile("audio-devices.xml");std::unique_ptr<juce::XmlElement> state;if(file.existsAsFile())state=juce::XmlDocument::parse(file);
    auto result=devices.initialise(1,2,state.get(),true,{},nullptr); if(result.isNotEmpty()){const juce::ScopedLock l(errorLock);error=result;return result;} devices.addAudioCallback(this);return {};
}
void AudioEngine::start(){running.store(true);} void AudioEngine::stop(){running.store(false);}
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* d){const int block=juce::jmax(maximumRealtimeBlockSize,d->getCurrentBufferSizeSamples());const int ch=juce::jmax(1,d->getActiveOutputChannels().countNumberOfSetBits());activeSampleRate.store(d->getCurrentSampleRate());preparedSamples.store(block);work.setSize(ch,block);voice.prepare(d->getCurrentSampleRate(),block,ch);}
void AudioEngine::audioDeviceStopped(){preparedSamples.store(0);voice.reset();}
void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* in,int ni,float* const* out,int no,int n,const juce::AudioIODeviceCallbackContext&){
    const auto begin=juce::Time::getHighResolutionTicks(); for(int c=0;c<no;++c)juce::FloatVectorOperations::clear(out[c],n); if(!running.load()||ni==0||no==0)return;
    const int channels=juce::jmin(no,work.getNumChannels()); if(channels<=0||n>preparedSamples.load()){xruns.fetch_add(1);return;} for(int c=0;c<channels;++c){const float* src=in[juce::jmin(c,ni-1)];if(src)juce::FloatVectorOperations::copy(work.getWritePointer(c),src,n);else work.clear(c,0,n);} juce::AudioBuffer<float> activeBlock(work.getArrayOfWritePointers(),channels,n);voice.process(activeBlock);for(int c=0;c<no;++c)juce::FloatVectorOperations::copy(out[c],activeBlock.getReadPointer(juce::jmin(c,channels-1)),n);
    const double elapsed=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-begin), allowed=n/activeSampleRate.load();cpu.store(allowed>0?elapsed/allowed:0);if(elapsed>allowed)xruns.fetch_add(1);
}
void AudioEngine::timerCallback(){if(devices.getCurrentAudioDevice()==nullptr)devices.restartLastAudioDevice();}
}
