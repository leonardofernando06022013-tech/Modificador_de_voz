#pragma once
#include "AdminAccessController.h"
#include "AuditLogManager.h"
#include "UserManager.h"
#include <functional>

namespace vox {

enum class AdminSessionState {
    Inactive,      // Nenhuma sessão iniciada
    Active,        // Sessão válida e ativa
    ExpiringSoon,  // Menos de 3 minutos restantes
    Expired,       // Tempo expirado
    Locked         // Bloqueada manualmente
};

struct AdminSessionInfo {
    AdminSessionState state = AdminSessionState::Inactive;
    UserAccount       user;
    juce::Time        startedAt;
    juce::Time        expiresAt;
    juce::Time        lastActivity;
    juce::String      expiredReason;
    int               sessionDurationMinutes = 30;
    int               warnMinutes            = 3;

    bool   isActive()       const noexcept { return state == AdminSessionState::Active || state == AdminSessionState::ExpiringSoon; }
    double minutesRemaining() const noexcept {
        if (!isActive()) return 0.0;
        return (expiresAt - juce::Time::getCurrentTime()).inMinutes();
    }
    juce::String remainingText() const;
};

class AdminSessionManager {
public:
    using StateCallback = std::function<void(AdminSessionState)>;

    AdminSessionManager();

    // Inicia uma sessão para o usuário atual do Windows
    bool  begin (int durationMinutes = 30);
    // Renova a sessão a partir de agora
    bool  renew ();
    // Bloqueia a sessão (exige renovação)
    void  lock  (const juce::String &reason = {});
    // Encerra a sessão permanentemente
    void  end   ();

    // Verifica o estado e notifica se mudou — chame do Timer a cada segundo
    void  tick  ();

    AdminSessionState    state()       const noexcept { return info.state; }
    const AdminSessionInfo& session() const noexcept { return info; }
    bool  isActive()                  const noexcept { return info.isActive(); }
    bool  can (Permission p)          const noexcept { return controller.can(p); }
    const UserAccount& user()         const noexcept { return info.user; }

    void setStateCallback(StateCallback cb) { onStateChanged = std::move(cb); }

private:
    void setState(AdminSessionState newState, const juce::String &reason = {});

    AdminSessionInfo       info;
    AdminAccessController  controller;
    UserManager            users;
    AuditLogManager        audit;
    StateCallback          onStateChanged;
};

} // namespace vox
