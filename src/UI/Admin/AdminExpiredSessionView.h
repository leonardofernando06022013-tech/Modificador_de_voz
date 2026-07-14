#pragma once
#include "Admin/AdminSessionManager.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace vox {

/// Tela exibida quando a sessão administrativa está expirada, bloqueada ou inativa.
/// Cobre completamente o conteúdo administrativo protegido.
class AdminExpiredSessionView final : public juce::Component {
public:
    std::function<void()> onRenew;     // botão "Renovar sessão"
    std::function<void()> onBack;      // botão "Voltar ao aplicativo"
    std::function<void()> onEndAdmin;  // botão "Encerrar conta administrativa"

    explicit AdminExpiredSessionView(const AdminSessionManager &mgr);

    void refresh();     // chame sempre que o estado mudar
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    const AdminSessionManager &session;

    juce::Label  iconLabel, titleLabel, subtitleLabel;
    juce::Label  userLabel, roleLabel, timeLabel, reasonLabel;
    juce::TextButton renewButton, backButton, endButton;

    juce::Colour stateColour() const noexcept;
    juce::String stateTitle()  const;
    juce::String stateSubtitle() const;
};

} // namespace vox
