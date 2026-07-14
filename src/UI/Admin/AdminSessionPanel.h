#pragma once
#include "Admin/AdminSessionManager.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Painel lateral direito mostrando sessão administrativa ativa,
/// info do usuário e ações (renovar / bloquear / encerrar).
class AdminSessionPanel final : public juce::Component {
public:
    std::function<void()> onRenew;
    std::function<void()> onLock;
    std::function<void()> onEnd;

    explicit AdminSessionPanel(AdminSessionManager &mgr);

    void refresh();
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    AdminSessionManager &session;

    juce::Label headingLabel;
    // Avatar / identidade
    juce::Label avatarLabel, nameLabel, roleLabel, stateLabel;
    // Tempos
    juce::Label startedLabel, lastLabel, expiresLabel, remainingLabel;
    // Botões
    juce::TextButton renewBtn, lockBtn, endBtn;

    juce::Colour stateColour() const noexcept;
};

} // namespace vox
