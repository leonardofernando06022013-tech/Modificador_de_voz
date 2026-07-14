#include "AdminSessionManager.h"

namespace vox {

// ─────────────────────────────────────────────────────────────────────────────
juce::String AdminSessionInfo::remainingText() const {
    if (!isActive()) return {};
    const double mins = minutesRemaining();
    if (mins < 1.0) {
        const int secs = juce::jmax(0, (int)((expiresAt - juce::Time::getCurrentTime()).inSeconds()));
        return juce::String(secs) + " segundos";
    }
    return juce::String((int)mins) + " minuto" + (mins >= 2.0 ? "s" : "");
}

// ─────────────────────────────────────────────────────────────────────────────
AdminSessionManager::AdminSessionManager() = default;

bool AdminSessionManager::begin(int durationMinutes) {
    info.user = users.currentUser();

    // Usuário vazio = sem conta registrada → promover automaticamente como SuperAdmin local
    if (info.user.id.isEmpty()) {
        // Primeira execução: criar conta owner com o login do Windows
        UserAccount owner;
        owner.id              = juce::Uuid().toString();
        owner.name            = juce::SystemStats::getLogonName();
        owner.windowsIdentity = owner.name;
        owner.role            = Role::SuperAdministrator;
        owner.status          = UserStatus::Active;
        owner.created         = owner.lastAccess = juce::Time::getCurrentTime();
        users.upsert(owner);
        info.user = owner;
    }

    if (info.user.id.isEmpty() ||
        info.user.status == UserStatus::Blocked ||
        info.user.role == Role::User) {
        setState(AdminSessionState::Expired, "Sem permissao administrativa");
        return false;
    }

    if (!controller.begin(info.user, durationMinutes)) {
        setState(AdminSessionState::Expired, "Falha ao iniciar sessao");
        return false;
    }

    info.startedAt             = juce::Time::getCurrentTime();
    info.lastActivity          = info.startedAt;
    info.expiresAt             = info.startedAt + juce::RelativeTime::minutes(durationMinutes);
    info.sessionDurationMinutes = durationMinutes;
    info.expiredReason.clear();
    setState(AdminSessionState::Active);

    audit.record(info.user, "SESSION_START", "Admin", "SUCCESS");
    return true;
}

bool AdminSessionManager::renew() {
    if (info.user.id.isEmpty()) return begin();
    if (info.user.status == UserStatus::Blocked) {
        setState(AdminSessionState::Locked, juce::String::fromUTF8("Conta bloqueada"));
        return false;
    }

    controller.begin(info.user, info.sessionDurationMinutes);
    info.expiresAt   = juce::Time::getCurrentTime() +
                       juce::RelativeTime::minutes(info.sessionDurationMinutes);
    info.lastActivity = juce::Time::getCurrentTime();
    setState(AdminSessionState::Active);

    audit.record(info.user, "SESSION_RENEW", "Admin", "SUCCESS");
    return true;
}

void AdminSessionManager::lock(const juce::String &reason) {
    controller.lock();
    setState(AdminSessionState::Locked,
             reason.isEmpty() ? juce::String::fromUTF8("Bloqueada manualmente") : reason);
    audit.record(info.user, "SESSION_LOCK", "Admin", "INFO");
}

void AdminSessionManager::end() {
    controller.lock();
    audit.record(info.user, "SESSION_END", "Admin", "INFO");
    setState(AdminSessionState::Inactive);
    info = AdminSessionInfo{};
}

void AdminSessionManager::tick() {
    if (info.state == AdminSessionState::Inactive ||
        info.state == AdminSessionState::Expired  ||
        info.state == AdminSessionState::Locked)
        return;

    const double minsLeft = info.minutesRemaining();

    if (minsLeft <= 0.0) {
        setState(AdminSessionState::Expired, juce::String::fromUTF8("Tempo esgotado"));
        controller.lock();
        audit.record(info.user, "SESSION_EXPIRE", "Admin", "INFO");
    } else if (minsLeft <= (double)info.warnMinutes &&
               info.state != AdminSessionState::ExpiringSoon) {
        setState(AdminSessionState::ExpiringSoon);
    } else if (minsLeft > (double)info.warnMinutes &&
               info.state == AdminSessionState::ExpiringSoon) {
        setState(AdminSessionState::Active);
    }
}

void AdminSessionManager::setState(AdminSessionState newState, const juce::String &reason) {
    if (!reason.isEmpty()) info.expiredReason = reason;
    if (info.state == newState) return;
    info.state = newState;
    if (onStateChanged) onStateChanged(newState);
}

} // namespace vox
