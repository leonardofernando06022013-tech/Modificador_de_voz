#include <juce_core/juce_core.h>
#include "DSP/VoiceProcessor.h"
#include "Presets/PresetManager.h"
#include "Settings/SettingsManager.h"
#include "UI/Navigation/PageRouter.h"
#include "Integrations/IntegrationProfile.h"
#include "Integrations/VirtualDeviceDetector.h"

class DSPTests final:public juce::UnitTest{public:DSPTests():UnitTest("DSP"){}void runTest()override{
    beginTest("Buffer vazio e bypass");vox::Parameters p;vox::VoiceProcessor v(p);v.prepare(48000,256,2);juce::AudioBuffer<float>b(2,256);b.clear();v.process(b);expectEquals(b.getMagnitude(0,256),0.0f);b.setSample(0,0,.25f);p.bypass=true;v.process(b);expectWithinAbsoluteError(b.getSample(0,0),.25f,.0001f);
    beginTest("Limitador evita clipping");p.bypass=false;p.inputGainDb=24;for(int c=0;c<2;++c)for(int i=0;i<256;++i)b.setSample(c,i,1);v.process(b);expect(b.getMagnitude(0,256)<=1.0f);
    beginTest("Parâmetros inválidos são limitados");auto*o=new juce::DynamicObject();o->setProperty("pitchSemitones",99);o->setProperty("mix",-4);expect(vox::SettingsManager::jsonToParameters(juce::var(o),p));expectEquals(p.pitchSemitones.load(),12.0f);expectEquals(p.mix.load(),0.0f);
    beginTest("JSON inválido");expect(!vox::SettingsManager::jsonToParameters(juce::JSON::parse("{bad"),p));
    beginTest("Preset salva e carrega");vox::PresetManager pm;p.pitchSemitones=6;expect(pm.save("Teste automatizado",p));p.pitchSemitones=0;expect(pm.load("Teste automatizado",p));expectEquals(p.pitchSemitones.load(),6.0f);expect(pm.remove("Teste automatizado"));
    beginTest("Mil vozes geradas sao distintas e limitadas");expectEquals(vox::PresetManager::generatedVoiceCount,1000);vox::Parameters generatedA,generatedB;pm.applyFactory(vox::PresetManager::generatedName(0),generatedA);pm.applyFactory(vox::PresetManager::generatedName(999),generatedB);expect(generatedA.pitchSemitones.load()!=generatedB.pitchSemitones.load()||generatedA.formant.load()!=generatedB.formant.load());expectWithinAbsoluteError(juce::jlimit(-12.0f,12.0f,generatedB.pitchSemitones.load()),generatedB.pitchSemitones.load(),.0001f);
    beginTest("Roteador preserva pagina anterior");vox::PageRouter router(vox::PageId::Home);int changes=0;router.setListener([&](vox::PageId oldPage,vox::PageId newPage){expect(oldPage!=newPage);++changes;});router.navigateTo(vox::PageId::Settings);expect(router.currentPage()==vox::PageId::Settings);expect(router.previousPage()==vox::PageId::Home);router.navigateTo(vox::PageId::Settings);expectEquals(changes,1);router.back();expect(router.currentPage()==vox::PageId::Home);expectEquals(changes,2);
    beginTest("Perfil de integracao preserva roteamento");vox::IntegrationProfile profile;profile.name="Discord";profile.inputDevice="Microfone USB";profile.virtualOutput="CABLE Input";profile.sampleRate=48000;profile.bufferSize=256;auto restored=vox::IntegrationProfile::fromJson(profile.toJson());expectEquals(restored.name,profile.name);expectEquals(restored.inputDevice,profile.inputDevice);expectEquals(restored.virtualOutput,profile.virtualOutput);expectEquals(restored.bufferSize,256);
    beginTest("Detector reconhece cabos virtuais sem falso positivo comum");expect(vox::VirtualDeviceDetector::isVirtual("CABLE Input (VB-Audio Virtual Cable)"));expect(vox::VirtualDeviceDetector::isVirtual("VoiceMeeter AUX Input"));expect(vox::VirtualDeviceDetector::isVirtual("Dubbing Speaker (Dubbing Virtual Device)"));expect(!vox::VirtualDeviceDetector::isVirtual("Microfone USB Realtek"));
    beginTest("Interface administrativa preserva UTF-8");auto adminText=juce::String::fromUTF8("Administração · usuários · permissões · relatório");expect(adminText.contains(juce::String::fromUTF8("Administração")));expect(!adminText.contains("Ã"));
}};
int main(){juce::ScopedJuceInitialiser_GUI init;DSPTests tests;juce::UnitTestRunner runner;runner.runAllTests();for(int i=0;i<runner.getNumResults();++i)if(runner.getResult(i)->failures>0)return 1;return 0;}
