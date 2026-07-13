#pragma once
#include "Settings/SettingsManager.h"
#include <juce_core/juce_core.h>
namespace vox {
class LocalizationManager final {
public:
  using Listener = std::function<void()>;
  static LocalizationManager &instance();
  void initialise(SettingsManager &);
  bool setLanguage(const juce::String &);
  juce::String text(const juce::String &) const;
  juce::String format(const juce::String &,
                      const juce::StringPairArray &) const;
  juce::String currentLanguage() const { return currentCode; }
  juce::StringArray languageCodes() const;
  juce::StringArray languageNames() const;
  int addListener(Listener);
  void removeListener(int);
  bool validateAll() const;

private:
  bool load(const juce::String &, juce::NamedValueSet &) const;
  juce::File localeFile(const juce::String &) const;
  juce::String detectSystemLanguage() const;
  SettingsManager *settings = nullptr;
  juce::String currentCode{"pt-BR"};
  juce::NamedValueSet current, fallback;
  std::map<int, Listener> listeners;
  int nextListener = 1;
};
} // namespace vox
