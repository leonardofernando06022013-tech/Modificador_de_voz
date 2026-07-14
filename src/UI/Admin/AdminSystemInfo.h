#pragma once
#include "Audio/AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Cartão organizado de informações do sistema — versão, SO, áudio, uptime.
class AdminSystemInfo final : public juce::Component {
public:
    explicit AdminSystemInfo(AudioEngine &engine);

    void refresh();
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    AudioEngine &eng;

    juce::Label headingLabel;
    juce::Label versionLabel, modeLabel, osLabel, archLabel;
    juce::Label audioLabel, uptimeLabel, backupLabel;

    juce::TextButton openDataBtn, copyDataBtn;

    juce::Time startTime;

    juce::String dataFolderName() const;

    static juce::String buildMode();
};

/// Seção de armazenamento com barras de progresso.
class AdminStoragePanel final : public juce::Component,
                                private juce::Thread {
public:
    AdminStoragePanel();
    ~AdminStoragePanel() override;

    void startCalculation();

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    void run() override;
    void applyResults();

    struct Entry {
        juce::String label;
        juce::String sizeText;
        float        fraction = 0.0f;
        juce::Colour colour;
    };

    juce::Array<Entry>   entries;
    juce::String         totalText;
    juce::CriticalSection lock;
    std::atomic<bool>    ready { false };
};

/// Saúde do sistema com itens e estados.
class AdminHealthPanel final : public juce::Component {
public:
    AdminHealthPanel();

    void runCheck();
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    enum class Health { Good, Warning, Error, Unavailable };

    struct Item {
        juce::String label;
        Health       health = Health::Unavailable;
        juce::String detail;
    };

    juce::Label headingLabel;
    juce::TextButton checkButton;
    juce::Array<Item> items;

    juce::Colour colourOf(Health h) const noexcept;
    juce::String iconOf(Health h)   const noexcept;
};

} // namespace vox
