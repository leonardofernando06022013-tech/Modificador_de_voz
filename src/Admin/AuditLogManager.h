#pragma once

#include "UserAccount.h"

namespace vox {

struct AuditEvent {
    juce::Time time;
    juce::String severity;
    juce::String userId;
    juce::String role;
    juce::String action;
    juce::String target;
    juce::String result;
};

class AuditLogManager {
public:
    void record(const UserAccount &, const juce::String &action,
                const juce::String &target, const juce::String &result,
                const juce::String &severity = "INFO") const;
    juce::StringArray recent(int limit = 100) const;
    juce::Array<AuditEvent> recentEvents(int limit = 100) const;
    juce::File file() const;
};

} // namespace vox
