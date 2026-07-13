#include "PresetManager.h"
#include "App/AppPaths.h"
namespace vox {
PresetManager::PresetManager(){dir=AppPaths::presets();}
juce::StringArray PresetManager::names()const{juce::StringArray r{"Voz normal limpa","Masculina grave","Feminina suave","Robô","Rádio policial","Telefone","Demônio","Alienígena","Monstro","Personagem infantil","Narrador","Megafone"};for(auto&f:dir.findChildFiles(juce::File::findFiles,false,"*.json"))r.addIfNotAlreadyThere(f.getFileNameWithoutExtension());return r;}
bool PresetManager::save(const juce::String&n,const Parameters&p)const{auto safe=juce::File::createLegalFileName(n.trim());return safe.isNotEmpty()&&dir.getChildFile(safe+".json").replaceWithText(juce::JSON::toString(SettingsManager::parametersToJson(p),true));}
bool PresetManager::load(const juce::String&n,Parameters&p)const{auto f=dir.getChildFile(juce::File::createLegalFileName(n)+".json");if(f.existsAsFile())return SettingsManager::jsonToParameters(juce::JSON::parse(f),p);applyFactory(n,p);return names().contains(n);}
bool PresetManager::remove(const juce::String&n)const{auto f=dir.getChildFile(juce::File::createLegalFileName(n)+".json");return f.existsAsFile()&&f.deleteFile();}
void PresetManager::applyFactory(const juce::String&n,Parameters&p)const{p.pitchSemitones=0;p.distortion=0;p.chorus=0;p.delay=0;p.reverb=0;p.ringMod=0;p.bitCrush=0;p.hpFreq=70;p.lpFreq=18000;
if(n=="Masculina grave"){p.pitchSemitones=-4;p.formant=-0.3f;}else if(n=="Feminina suave"){p.pitchSemitones=3;p.formant=0.25f;p.chorus=.12f;p.reverb=.12f;}else if(n=="Robô"){p.ringMod=.8f;p.bitCrush=.2f;}else if(n=="Rádio policial"){p.hpFreq=400;p.lpFreq=3500;p.distortion=.18f;}else if(n=="Telefone"){p.hpFreq=500;p.lpFreq=3000;p.bitCrush=.12f;}else if(n=="Demônio"){p.pitchSemitones=-8;p.distortion=.35f;p.reverb=.25f;}else if(n=="Alienígena"){p.pitchSemitones=5;p.ringMod=.45f;p.delay=.15f;}else if(n=="Monstro"){p.pitchSemitones=-10;p.distortion=.25f;p.chorus=.2f;}else if(n=="Personagem infantil"){p.pitchSemitones=7;p.formant=.5f;}else if(n=="Narrador"){p.pitchSemitones=-2;p.compressorRatio=4;p.reverb=.08f;}else if(n=="Megafone"){p.hpFreq=350;p.lpFreq=4500;p.distortion=.3f;p.delay=.08f;}}
}
