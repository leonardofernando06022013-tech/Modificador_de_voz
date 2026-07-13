#include "IntegrationProfileManager.h"
#include "App/AppPaths.h"
namespace vox {
IntegrationProfileManager::IntegrationProfileManager() {
  auto dir = AppPaths::data().getChildFile("Integrations");
  dir.createDirectory();
  profilesFile = dir.getChildFile("integration-profiles.json");
}
juce::Array<IntegrationProfile> IntegrationProfileManager::load() const {
  juce::Array<IntegrationProfile> result;
  if (!profilesFile.existsAsFile())
    return result;
  auto value = juce::JSON::parse(profilesFile);
  if (auto *a = value.getArray())
    for (auto &item : *a)
      result.add(IntegrationProfile::fromJson(item));
  return result;
}
bool IntegrationProfileManager::save(
    const juce::Array<IntegrationProfile> &items) const {
  juce::Array<juce::var> values;
  for (auto &i : items)
    values.add(i.toJson());
  juce::TemporaryFile temporary(profilesFile);
  return temporary.getFile().replaceWithText(
             juce::JSON::toString(juce::var(values), true)) &&
         temporary.overwriteTargetFileWithTemporary();
}
bool IntegrationProfileManager::upsert(const IntegrationProfile &profile) {
  auto items = load();
  bool found = false;
  for (auto &i : items)
    if (i.name == profile.name) {
      i = profile;
      found = true;
      break;
    }
  if (!found)
    items.add(profile);
  return save(items);
}
bool IntegrationProfileManager::remove(const juce::String &name) {
  auto items = load();
  for (int i = items.size(); --i >= 0;)
    if (items.getReference(i).name == name)
      items.remove(i);
  return save(items);
}
bool IntegrationProfileManager::exportTo(const juce::File&target)const{return profilesFile.existsAsFile()?profilesFile.copyFileTo(target):target.replaceWithText("[]");}
bool IntegrationProfileManager::importFrom(const juce::File&source){if(!source.existsAsFile())return false;auto parsed=juce::JSON::parse(source);if(!parsed.isArray())return false;juce::Array<IntegrationProfile>items;for(auto&item:*parsed.getArray())items.add(IntegrationProfile::fromJson(item));return save(items);}
} // namespace vox
