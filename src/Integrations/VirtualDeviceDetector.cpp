#include "VirtualDeviceDetector.h"
namespace vox {
bool VirtualDeviceDetector::isVirtual(const juce::String &name) {
  auto n = name.toLowerCase().removeCharacters(" -_().");
  const juce::StringArray terms{"vbaudio",      "cableinput",  "cableoutput",
                                "virtualcable", "voicemeeter", "vaio",
                                "virtualaudio", "auxinput",    "auxoutput",
                                "virtualdevice", "dubbing",    "voicewave"};
  for (auto term : terms)
    if (n.contains(term))
      return true;
  return false;
}
juce::Array<DetectedAudioDevice>
VirtualDeviceDetector::scan(juce::AudioDeviceManager &m) {
  juce::Array<DetectedAudioDevice> result;
  auto setup = m.getAudioDeviceSetup();
  for (auto *type : m.getAvailableDeviceTypes()) {
    type->scanForDevices();
    for (bool input : {true, false})
      for (auto name : type->getDeviceNames(input)) {
        DetectedAudioDevice d{name, type->getTypeName(), input, isVirtual(name),
                              input ? name == setup.inputDeviceName
                                    : name == setup.outputDeviceName};
        result.add(d);
      }
  }
  return result;
}
juce::StringArray
VirtualDeviceDetector::inputs(const juce::Array<DetectedAudioDevice> &items) {
  juce::StringArray out;
  for (auto &i : items)
    if (i.input && !i.virtualDevice)
      out.addIfNotAlreadyThere(i.name);
  return out;
}
juce::StringArray VirtualDeviceDetector::virtualOutputs(
    const juce::Array<DetectedAudioDevice> &items) {
  juce::StringArray out;
  for (auto &i : items)
    if (!i.input && i.virtualDevice)
      out.addIfNotAlreadyThere(i.name);
  return out;
}
} // namespace vox
