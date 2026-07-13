#include "AuditLogManager.h"

#include "App/AppPaths.h"

namespace vox {

juce::File AuditLogManager::file() const {
    return AppPaths::logs().getChildFile("audit.log");
}

void AuditLogManager::record(const UserAccount &user,
                             const juce::String &action,
                             const juce::String &target,
                             const juce::String &result,
                             const juce::String &severity) const {
    const auto safe = [](juce::String value) {
        return value.replace("|", "/").replaceCharacters("\r\n", "  ");
    };

    file().appendText(juce::Time::getCurrentTime().toISO8601(true) + " | " +
                      severity + " | " + safe(user.id) + " | " +
                      safe(roleName(user.role)) + " | " + safe(action) + " | " +
                      safe(target) + " | " + safe(result) + "\n");
}

juce::StringArray AuditLogManager::recent(int limit) const {
    auto lines = juce::StringArray::fromLines(file().loadFileAsString());
    while (lines.size() > limit)
        lines.remove(0);
    return lines;
}

juce::Array<AuditEvent> AuditLogManager::recentEvents(int limit) const {
    juce::Array<AuditEvent> events;
    for (auto line : recent(limit)) {
        auto fields = juce::StringArray::fromTokens(line, "|", "");
        for (auto &field : fields)
            field = field.trim();
        if (fields.size() < 7)
            continue;

        AuditEvent event;
        event.time = juce::Time::fromISO8601(fields[0]);
        event.severity = fields[1];
        event.userId = fields[2];
        event.role = fields[3];
        event.action = fields[4];
        event.target = fields[5];
        event.result = fields[6];
        events.insert(0, event);
    }
    return events;
}

} // namespace vox
