#include "LocalizationManager.h"
namespace vox {
LocalizationManager &LocalizationManager::instance() {
  static LocalizationManager manager;
  return manager;
}
juce::StringArray LocalizationManager::languageCodes() const {
  return {"pt-BR", "en-US", "es-ES", "fr-FR", "de-DE",
          "it-IT", "ja-JP", "ko-KR", "zh-CN", "ru-RU"};
}
juce::StringArray LocalizationManager::languageNames() const {
  return {juce::String::fromUTF8("Português (Brasil)"),
          "English",
          juce::String::fromUTF8("Español"),
          juce::String::fromUTF8("Français"),
          "Deutsch",
          "Italiano",
          juce::String::fromUTF8("日本語"),
          juce::String::fromUTF8("한국어"),
          juce::String::fromUTF8("简体中文"),
          juce::String::fromUTF8("Русский")};
}
juce::File LocalizationManager::localeFile(const juce::String &code) const {
  auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                 .getParentDirectory()
                 .getChildFile("Resources")
                 .getChildFile("locales")
                 .getChildFile(code + ".json");
  if (exe.existsAsFile())
    return exe;
  auto cwd=juce::File::getCurrentWorkingDirectory()
      .getChildFile("resources")
      .getChildFile("locales")
      .getChildFile(code + ".json");
  if(cwd.existsAsFile())return cwd;
  return juce::File::getCurrentWorkingDirectory().getParentDirectory().getChildFile("resources").getChildFile("locales").getChildFile(code+".json");
}
bool LocalizationManager::load(const juce::String &code,
                               juce::NamedValueSet &out) const {
  auto file = localeFile(code);
  if (!file.existsAsFile())
    return false;
  auto parsed = juce::JSON::parse(file);
  auto *object = parsed.getDynamicObject();
  if (!object)
    return false;
  out.clear();
  for (const auto &property : object->getProperties())
    out.set(property.name, property.value);
  return !out.isEmpty();
}
juce::String LocalizationManager::detectSystemLanguage() const {
  auto code = juce::SystemStats::getUserLanguage().toLowerCase();
  if (code.startsWith("pt"))
    return "pt-BR";
  if (code.startsWith("en"))
    return "en-US";
  if (code.startsWith("es"))
    return "es-ES";
  if (code.startsWith("fr"))
    return "fr-FR";
  if (code.startsWith("de"))
    return "de-DE";
  if (code.startsWith("it"))
    return "it-IT";
  if (code.startsWith("ja"))
    return "ja-JP";
  if (code.startsWith("ko"))
    return "ko-KR";
  if (code.startsWith("zh"))
    return "zh-CN";
  if (code.startsWith("ru"))
    return "ru-RU";
  return "pt-BR";
}
void LocalizationManager::initialise(SettingsManager &s) {
  settings = &s;
  load("pt-BR", fallback);
  auto code = s.preference("interface.language");
  if (code.isEmpty()) {
    code = detectSystemLanguage();
    s.setPreference("interface.language", code);
  }
  if (!setLanguage(code))
    setLanguage("pt-BR");
}
bool LocalizationManager::setLanguage(const juce::String &code) {
  if (!languageCodes().contains(code))
    return false;
  juce::NamedValueSet loaded;
  if (!load(code, loaded))
    return false;
  current = loaded;
  currentCode = code;
  if (settings && !settings->setPreference("interface.language", code))
    return false;
  for (auto &listener : listeners)
    if (listener.second)
      listener.second();
  return true;
}
juce::String LocalizationManager::text(const juce::String &key) const {
  auto id = juce::Identifier(key);
  if (current.contains(id))
    return current[id].toString();
  if (fallback.contains(id))
    return fallback[id].toString();
  juce::Logger::writeToLog("Missing locale key: " + key);
  return "[" + key + "]";
}
juce::String
LocalizationManager::format(const juce::String &key,
                            const juce::StringPairArray &values) const {
  auto result = text(key);
  for (const auto &name : values.getAllKeys())
    result = result.replace("{" + name + "}", values[name]);
  return result;
}
int LocalizationManager::addListener(Listener listener) {
  const int id = nextListener++;
  listeners[id] = std::move(listener);
  return id;
}
void LocalizationManager::removeListener(int id) { listeners.erase(id); }
bool LocalizationManager::validateAll() const {
  juce::NamedValueSet reference;
  if (!load("pt-BR", reference))
    return false;
  for (const auto &code : languageCodes()) {
    juce::NamedValueSet values;
    if (!load(code, values) || values.size() != reference.size())
      return false;
    for (const auto &item : reference)
      if (!values.contains(item.name))
        return false;
  }
  return true;
}
} // namespace vox
