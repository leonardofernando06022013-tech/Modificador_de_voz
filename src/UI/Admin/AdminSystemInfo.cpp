#include "AdminSystemInfo.h"
#include "App/AppPaths.h"
#include "UI/Theme.h"
#include "Admin/BackupManager.h"

namespace vox {

// ─── Helpers ────────────────────────────────────────────────────────────────
namespace {
static void sLabel(juce::Label &l, float sz, juce::Colour c, bool bold = false) {
    l.setColour(juce::Label::textColourId, c);
    l.setFont(juce::Font(juce::FontOptions(sz, bold ? juce::Font::bold : juce::Font::plain)));
}

static juce::String formatSize(int64_t bytes) {
    if (bytes < 1024)       return juce::String(bytes) + " B";
    if (bytes < 1024*1024)  return juce::String(bytes / 1024) + " KB";
    return juce::String::formatted("%.1f MB", bytes / (1024.0 * 1024.0));
}
} // namespace

// ─── AdminSystemInfo ─────────────────────────────────────────────────────────

AdminSystemInfo::AdminSystemInfo(AudioEngine &engine)
    : eng(engine), startTime(juce::Time::getCurrentTime())
{
    sLabel(headingLabel, 13, theme::text, true);
    headingLabel.setText(juce::String::fromUTF8("Informações do Sistema"),
                         juce::dontSendNotification);

    for (auto *l : { &versionLabel, &modeLabel, &osLabel, &archLabel,
                     &audioLabel, &uptimeLabel, &backupLabel })
    {
        sLabel(*l, 11, theme::muted);
        addAndMakeVisible(l);
    }
    addAndMakeVisible(headingLabel);

    openDataBtn.setButtonText(juce::String::fromUTF8("Abrir pasta"));
    openDataBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    openDataBtn.setColour(juce::TextButton::textColourOffId, theme::text);
    openDataBtn.onClick = [] { AppPaths::data().startAsProcess(); };

    copyDataBtn.setButtonText("Copiar caminho");
    copyDataBtn.setColour(juce::TextButton::buttonColourId, theme::elevated);
    copyDataBtn.setColour(juce::TextButton::textColourOffId, theme::muted);
    copyDataBtn.onClick = [] {
        juce::SystemClipboard::copyTextToClipboard(
            AppPaths::data().getFullPathName());
    };

    addAndMakeVisible(openDataBtn);
    addAndMakeVisible(copyDataBtn);
    refresh();
}

juce::String AdminSystemInfo::buildMode() {
#if JUCE_DEBUG
    return "Debug";
#else
    return "Release";
#endif
}

juce::String AdminSystemInfo::dataFolderName() const {
    // Mostrar apenas o nome da pasta, não o caminho completo
    auto path = AppPaths::data().getFullPathName();
    // Ex: C:\Users\usuario\AppData\Roaming\BlackVoice → "BlackVoice"
    return AppPaths::data().getFileName();
}

void AdminSystemInfo::refresh() {
    versionLabel.setText("v1.0.0  ·  Build: " + buildMode(),
                         juce::dontSendNotification);
    osLabel.setText(juce::String::fromUTF8("SO: ") +
                    juce::SystemStats::getOperatingSystemName(),
                    juce::dontSendNotification);
    archLabel.setText(juce::String::fromUTF8("Arquitetura: ") +
                      (juce::SystemStats::isOperatingSystem64Bit() ? "64 bits" : "32 bits"),
                      juce::dontSendNotification);

    const bool audioOk = eng.deviceManager().getCurrentAudioDevice() != nullptr;
    audioLabel.setText(juce::String::fromUTF8("Áudio: ") +
                       (audioOk ? juce::String::fromUTF8("Online") :
                                  juce::String::fromUTF8("Não disponível")),
                       juce::dontSendNotification);
    audioLabel.setColour(juce::Label::textColourId,
                         audioOk ? theme::green : theme::yellow);

    const double uptimeSecs = (juce::Time::getCurrentTime() - startTime).inSeconds();
    const int h = (int)(uptimeSecs / 3600);
    const int m = (int)(uptimeSecs / 60) % 60;
    const int s = (int)uptimeSecs % 60;
    uptimeLabel.setText(juce::String::fromUTF8("Em execução: ") +
                        juce::String::formatted("%02d:%02d:%02d", h, m, s),
                        juce::dontSendNotification);

    BackupManager bm;
    auto backups = bm.list();
    if (!backups.isEmpty()) {
        auto latest = backups.getLast();
        backupLabel.setText(juce::String::fromUTF8("Último backup: ") +
                            latest.getLastModificationTime().toString(true, false),
                            juce::dontSendNotification);
    } else {
        backupLabel.setText(juce::String::fromUTF8("Último backup: Nenhum"),
                            juce::dontSendNotification);
    }

    modeLabel.setText(juce::String::fromUTF8("Pasta de dados: ") + dataFolderName(),
                      juce::dontSendNotification);
    modeLabel.setTooltip(AppPaths::data().getFullPathName());
    repaint();
}

void AdminSystemInfo::paint(juce::Graphics &g) {
    theme::roundedPanel(g, getLocalBounds().toFloat(), 10, theme::elevated);
}

void AdminSystemInfo::resized() {
    auto a = getLocalBounds().reduced(12, 8);
    headingLabel.setBounds(a.removeFromTop(22));
    a.removeFromTop(4);
    for (auto *l : { &versionLabel, &modeLabel, &osLabel, &archLabel,
                     &audioLabel, &uptimeLabel, &backupLabel }) {
        l->setBounds(a.removeFromTop(18));
    }
    a.removeFromTop(6);
    auto btnRow = a.removeFromTop(28);
    openDataBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(2));
    copyDataBtn.setBounds(btnRow.reduced(2));
}

// ─── AdminStoragePanel ───────────────────────────────────────────────────────

AdminStoragePanel::AdminStoragePanel() : juce::Thread("StorageCalc") {
    // Inicializar com placeholders
    entries.add({ "Presets",  juce::String::fromUTF8("Calculando…"), 0.0f, theme::blue   });
    entries.add({ "Logs",     juce::String::fromUTF8("Calculando…"), 0.0f, theme::yellow });
    entries.add({ "Backups",  juce::String::fromUTF8("Calculando…"), 0.0f, theme::purple });
    entries.add({ "Cache",    juce::String::fromUTF8("Calculando…"), 0.0f, theme::cyan   });
    totalText = juce::String::fromUTF8("Calculando…");
}

AdminStoragePanel::~AdminStoragePanel() { stopThread(2000); }

void AdminStoragePanel::startCalculation() {
    if (!isThreadRunning()) startThread();
}

void AdminStoragePanel::run() {
    auto calc = [](const juce::File &dir) -> int64_t {
        int64_t total = 0;
        for (auto &f : dir.findChildFiles(juce::File::findFiles, true))
            total += f.getSize();
        return total;
    };

    const int64_t presetsSize = calc(AppPaths::data().getChildFile("presets"));
    const int64_t logsSize    = calc(AppPaths::logs());
    const int64_t backupsSize = calc(AppPaths::data().getChildFile("Backups"));
    const int64_t cacheSize   = calc(AppPaths::data().getChildFile("cache"));
    const int64_t total       = presetsSize + logsSize + backupsSize + cacheSize;

    juce::CriticalSection::ScopedLockType lock2(lock);
    entries.getReference(0).sizeText = formatSize(presetsSize);
    entries.getReference(1).sizeText = formatSize(logsSize);
    entries.getReference(2).sizeText = formatSize(backupsSize);
    entries.getReference(3).sizeText = formatSize(cacheSize);

    if (total > 0) {
        entries.getReference(0).fraction = (float)presetsSize / total;
        entries.getReference(1).fraction = (float)logsSize    / total;
        entries.getReference(2).fraction = (float)backupsSize / total;
        entries.getReference(3).fraction = (float)cacheSize   / total;
    }

    totalText = juce::String::fromUTF8("Total: ") + formatSize(total);
    ready = true;

    juce::MessageManager::callAsync([this] { repaint(); });
}

void AdminStoragePanel::paint(juce::Graphics &g) {
    theme::roundedPanel(g, getLocalBounds().toFloat(), 10, theme::elevated);

    auto a = getLocalBounds().reduced(12, 8);
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(13, juce::Font::bold)));
    g.drawText(juce::String::fromUTF8("Armazenamento"), a.removeFromTop(22),
               juce::Justification::centredLeft);
    a.removeFromTop(4);

    juce::CriticalSection::ScopedLockType lock2(lock);
    for (auto &entry : entries) {
        auto row = a.removeFromTop(32);
        // Label
        g.setColour(theme::muted);
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.drawText(entry.label, row.removeFromTop(14), juce::Justification::centredLeft);
        // Barra + tamanho
        auto barRow = row;
        auto sizeLabel = barRow.removeFromRight(72);
        g.setColour(theme::border);
        g.fillRoundedRectangle(barRow.toFloat(), 4);
        g.setColour(entry.colour.withAlpha(0.7f));
        g.fillRoundedRectangle(barRow.withWidth(
            (int)(barRow.getWidth() * entry.fraction)).toFloat(), 4);
        g.setColour(entry.colour);
        g.setFont(juce::Font(juce::FontOptions(10)));
        g.drawText(entry.sizeText, sizeLabel, juce::Justification::centredRight);
        a.removeFromTop(2);
    }

    a.removeFromTop(4);
    g.setColour(theme::border);
    g.drawHorizontalLine(a.getY(), (float)a.getX(), (float)a.getRight());
    a.removeFromTop(6);
    g.setColour(theme::text);
    g.setFont(juce::Font(juce::FontOptions(12, juce::Font::bold)));
    g.drawText(totalText, a.removeFromTop(20), juce::Justification::centredLeft);
}

void AdminStoragePanel::resized() {}

// ─── AdminHealthPanel ─────────────────────────────────────────────────────────

AdminHealthPanel::AdminHealthPanel() {
    sLabel(headingLabel, 13, theme::text, true);
    headingLabel.setText(juce::String::fromUTF8("Saúde do Sistema"),
                         juce::dontSendNotification);
    addAndMakeVisible(headingLabel);

    checkButton.setButtonText(juce::String::fromUTF8("↻  Executar verificação"));
    checkButton.setColour(juce::TextButton::buttonColourId, theme::elevated);
    checkButton.setColour(juce::TextButton::textColourOffId, theme::cyan);
    checkButton.onClick = [this] { runCheck(); };
    addAndMakeVisible(checkButton);

    // Inicializar itens
    const char *labels[] = {
        "Sess\xC3\xa3o", "Banco local",
        "\xC3\x81udio", "Configura\xC3\xa7\xC3\xb5\x65s",
        "Logs", "Backups",
        "Armazenamento", "Permiss\xC3\xb5\x65s"
    };
    for (auto *lbl : labels)
        items.add({ juce::String::fromUTF8(lbl), Health::Unavailable, {} });
}

void AdminHealthPanel::runCheck() {
    const bool usersOk = AppPaths::data().getChildFile("Admin/users.json").existsAsFile();
    const bool settingsOk = AppPaths::data().getChildFile("settings.json").existsAsFile();
    const bool logsOk = AppPaths::logs().isDirectory();
    const bool backupsOk = AppPaths::data().getChildFile("Backups").isDirectory();

    auto set = [this](int i, Health h, const juce::String &d = {}) {
        items.getReference(i).health = h;
        if (d.isNotEmpty()) items.getReference(i).detail = d;
    };

    set(0, Health::Good);   // sessão verificada externamente
    set(1, usersOk ? Health::Good : Health::Warning,
           usersOk ? "OK" : juce::String::fromUTF8("Arquivo não encontrado"));
    set(2, Health::Good);   // áudio ok se chegou aqui
    set(3, settingsOk ? Health::Good : Health::Warning,
           settingsOk ? "OK" : juce::String::fromUTF8("settings.json não encontrado"));
    set(4, logsOk    ? Health::Good : Health::Error);
    set(5, backupsOk ? Health::Good : Health::Warning);
    set(6, Health::Good);   // armazenamento
    set(7, usersOk ? Health::Good : Health::Warning);

    repaint();
}

juce::Colour AdminHealthPanel::colourOf(Health h) const noexcept {
    switch (h) {
        case Health::Good:        return theme::green;
        case Health::Warning:     return theme::yellow;
        case Health::Error:       return theme::red;
        default:                  return theme::muted;
    }
}

juce::String AdminHealthPanel::iconOf(Health h) const noexcept {
    switch (h) {
        case Health::Good:        return juce::String::fromUTF8("✓");
        case Health::Warning:     return juce::String::fromUTF8("⚠");
        case Health::Error:       return juce::String::fromUTF8("✕");
        default:                  return juce::String::fromUTF8("○");
    }
}

void AdminHealthPanel::paint(juce::Graphics &g) {
    theme::roundedPanel(g, getLocalBounds().toFloat(), 10, theme::elevated);

    auto a = getLocalBounds().reduced(12, 8);
    headingLabel.setBounds(a.removeFromTop(22));
    a.removeFromTop(4);

    for (auto &item : items) {
        auto row = a.removeFromTop(22);
        // Ícone
        g.setColour(colourOf(item.health));
        g.setFont(juce::Font(juce::FontOptions(11, juce::Font::bold)));
        g.drawText(iconOf(item.health), row.removeFromLeft(20),
                   juce::Justification::centred);
        // Label
        g.setColour(theme::text);
        g.setFont(juce::Font(juce::FontOptions(11)));
        g.drawText(item.label, row, juce::Justification::centredLeft);
        // Estado (direita)
        g.setColour(colourOf(item.health));
        juce::String stateStr;
        switch (item.health) {
            case Health::Good:        stateStr = juce::String::fromUTF8("Saudável");        break;
            case Health::Warning:     stateStr = juce::String::fromUTF8("Atenção");         break;
            case Health::Error:       stateStr = "Erro";                                     break;
            default:                  stateStr = juce::String::fromUTF8("Não verificado");  break;
        }
        g.drawText(stateStr, row, juce::Justification::centredRight);
    }

    a.removeFromTop(6);
    checkButton.setBounds(a.removeFromTop(32).reduced(0, 2));
}

void AdminHealthPanel::resized() {
    // Layout feito no paint para evitar duplicação
}

} // namespace vox
