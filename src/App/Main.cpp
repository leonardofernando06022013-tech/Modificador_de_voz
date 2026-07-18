#include <juce_gui_extra/juce_gui_extra.h>
#include "MainWindow.h"
#include "AppPaths.h"
#include "Localization/LocalizationManager.h"

namespace vox {
class Application final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "BlackVoice"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void anotherInstanceStarted(const juce::String&) override { if (window) window->bringToFront(); }

    void initialise(const juce::String& commandLine) override {
        const auto stamp = juce::Time::getCurrentTime().toString(true, true, true, true);
        const auto utf8 = [] (const char* text) { return juce::String::fromUTF8(text); };
        const auto preferences = settings.preferences();
        const auto preference = [&preferences](const char *key,
                                               const char *fallback) {
            return preferences.getValue(key, fallback);
        };
        const bool saveLogs = preference("saveLogs", "1") == "1";
        const auto log = [&] (const juce::String& name, const juce::String& line) {
            if (!saveLogs) return;
            AppPaths::logs().getChildFile(name).appendText(stamp + " | " + line + "\r\n");
        };

        log("startup.log", "Version 1.0.0 | " + juce::SystemStats::getOperatingSystemName()
            + " | " + juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName());
        juce::LookAndFeel::setDefaultLookAndFeel(&look);
        if (preference("restorePreset", "1") == "1" &&
            !settings.load(engine.parameters()))
            log("errors.log", "Arquivo de configuracao corrompido movido para backup");
        LocalizationManager::instance().initialise(settings);

        const auto error = engine.initialise();
        if (error.isEmpty() && preference("autoProcess", "0") == "1")
            engine.start();
        if (error.isEmpty() && preference("monitorInput", "0") == "1") {
            engine.start();
            const auto monitorError = engine.setMonitoring(true);
            if (monitorError.isNotEmpty()) {
                log("errors.log", "Falha ao restaurar monitoramento: " + monitorError);
                if (preference("autoProcess", "0") != "1") engine.stop();
            }
        }
        if (auto* device = engine.deviceManager().getCurrentAudioDevice())
            log("audio.log", device->getName() + " | " + juce::String(device->getCurrentSampleRate())
                + " Hz | " + juce::String(device->getCurrentBufferSizeSamples()) + " samples");

        window = std::make_unique<MainWindow>(engine, settings);
        if (error.isNotEmpty()) {
            log("errors.log", "Falha de inicializacao de audio: " + error);
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                utf8("\xC3\x81udio indispon\xC3\xADvel"),
                error + utf8("\nVerifique a permiss\xC3\xA3o do microfone e o dispositivo selecionado."));
        }

        const auto noticeFlag = AppPaths::data().getChildFile("responsible-use-shown.flag");
        if (!noticeFlag.existsAsFile()) {
            noticeFlag.replaceWithText("1");
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                utf8("Uso respons\xC3\xA1vel"),
                utf8("Use modifica\xC3\xA7\xC3\xA3o de voz com consentimento. O \xC3\xA1udio \xC3\xA9 processado localmente e n\xC3\xA3o \xC3\xA9 gravado nem enviado."));
        }
        if (commandLine.contains("--ui-smoke-test"))
            juce::MessageManager::callAsync([this] {
                auto* main = window ? dynamic_cast<MainComponent*>(window->getContentComponent()) : nullptr;
                setApplicationReturnValue(main && main->runNavigationSmokeTest() ? 0 : 1);
                systemRequestedQuit();
            });
    }

    void shutdown() override {
        engine.stop();
        settings.save(engine.parameters());
        window.reset();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

private:
    juce::LookAndFeel_V4 look;
    AudioEngine engine;
    SettingsManager settings;
    std::unique_ptr<MainWindow> window;
};
}

START_JUCE_APPLICATION(vox::Application)
