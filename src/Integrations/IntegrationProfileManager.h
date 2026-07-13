#pragma once
#include "IntegrationProfile.h"
namespace vox {
class IntegrationProfileManager {
public:
  IntegrationProfileManager();
  juce::Array<IntegrationProfile> load() const;
  bool save(const juce::Array<IntegrationProfile> &) const;
  bool upsert(const IntegrationProfile &);
  bool remove(const juce::String &);
  bool exportTo(const juce::File &) const;
  bool importFrom(const juce::File &);
  juce::File file() const { return profilesFile; }

private:
  juce::File profilesFile;
};
} // namespace vox
