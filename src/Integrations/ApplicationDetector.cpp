#include "ApplicationDetector.h"

#if JUCE_WINDOWS
#include <Windows.h>
#include <TlHelp32.h>
#endif

namespace vox {
juce::StringArray ApplicationDetector::executablesFor(
    const juce::String &target) {
  if (target == "Discord") return {"Discord.exe"};
  if (target == "FiveM") return {"FiveM.exe", "FiveM_b2802_GTAProcess.exe"};
  if (target == "OBS") return {"obs64.exe", "obs32.exe"};
  if (target == "Jogos")
    return {"steam.exe", "RobloxPlayerBeta.exe", "FiveM.exe"};
  if (target == "Chamadas")
    return {"Teams.exe", "ms-teams.exe", "Zoom.exe", "Skype.exe"};
  return {};
}

bool ApplicationDetector::isRunning(const juce::String &executable) {
#if JUCE_WINDOWS
  const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return false;
  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  bool found = false;
  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (juce::String(entry.szExeFile).equalsIgnoreCase(executable)) {
        found = true;
        break;
      }
    } while (Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return found;
#else
  juce::ignoreUnused(executable);
  return false;
#endif
}

bool ApplicationDetector::isTargetRunning(const juce::String &target) {
  for (const auto &executable : executablesFor(target))
    if (isRunning(executable)) return true;
  return false;
}

juce::String ApplicationDetector::statusFor(const juce::String &target) {
  if (target == "Manual")
    return "Configuração manual · verificação de processo não aplicável";
  const auto executables = executablesFor(target);
  if (executables.isEmpty())
    return target + " · nenhum processo específico configurado";
  return isTargetRunning(target)
             ? target + " aberto · roteamento ainda precisa ser testado"
             : target + " não detectado · abra o aplicativo para testar";
}
} // namespace vox
