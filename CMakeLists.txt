cmake_minimum_required(VERSION 3.25)
project(BlackVoice VERSION 1.0.0 LANGUAGES CXX)

add_custom_target(validate_locales ALL
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate-locales.ps1"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  VERBATIM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
option(VOXFORGE_FETCH_JUCE "Download JUCE automatically" ON)

if(VOXFORGE_FETCH_JUCE)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.13 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
else()
  find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_gui_app(BlackVoice PRODUCT_NAME "BlackVoice" COMPANY_NAME "BlackVoice" VERSION ${PROJECT_VERSION}
  BUNDLE_ID "com.blackvoice.app"
  ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/BlackVoice.png"
  ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/BlackVoice.png")
set_target_properties(BlackVoice PROPERTIES OUTPUT_NAME "BlackVoice")
target_sources(BlackVoice PRIVATE
  src/App/Main.cpp src/App/MainWindow.h
  src/Admin/UserAccount.h src/Admin/UserAccount.cpp
  src/Admin/UserManager.h src/Admin/UserManager.cpp
  src/Admin/PermissionManager.h src/Admin/PermissionManager.cpp
  src/Admin/AdminAccessController.h src/Admin/AdminAccessController.cpp
  src/Admin/AuditLogManager.h src/Admin/AuditLogManager.cpp
  src/Admin/BackupManager.h src/Admin/BackupManager.cpp
  src/Admin/AdminSessionManager.h src/Admin/AdminSessionManager.cpp
  src/Audio/AudioEngine.h src/Audio/AudioEngine.cpp
  src/Audio/AudioRoutingManager.h src/Audio/AudioRoutingManager.cpp
  src/DSP/VoiceProcessor.h src/DSP/VoiceProcessor.cpp
  src/Presets/PresetManager.h src/Presets/PresetManager.cpp
  src/Settings/SettingsManager.h src/Settings/SettingsManager.cpp
  src/Localization/LocalizationManager.h src/Localization/LocalizationManager.cpp
  src/Diagnostics/DiagnosticsManager.h src/Diagnostics/DiagnosticsManager.cpp
  src/UI/MainComponent.h src/UI/MainComponent.cpp
  src/UI/Theme.h src/UI/AppLookAndFeel.h src/UI/AppLookAndFeel.cpp
  src/UI/ModernComponents.h src/UI/ModernComponents.cpp
  src/UI/Navigation/PageRouter.h src/UI/Navigation/PageRouter.cpp
  src/UI/Pages/SettingsPage.h src/UI/Pages/SettingsPage.cpp
  src/UI/Pages/ModulePage.h src/UI/Pages/ModulePage.cpp
  src/UI/Pages/VoicesPage.h src/UI/Pages/VoicesPage.cpp
  src/UI/Pages/AdminPage.h src/UI/Pages/AdminPage.cpp
  src/UI/Admin/AdminActivityFeed.h src/UI/Admin/AdminActivityFeed.cpp
  src/UI/Admin/AdminExpiredSessionView.h src/UI/Admin/AdminExpiredSessionView.cpp
  src/UI/Admin/AdminSessionPanel.h src/UI/Admin/AdminSessionPanel.cpp
  src/UI/Admin/AdminStatsCard.h src/UI/Admin/AdminStatsCard.cpp
  src/UI/Admin/AdminSystemInfo.h src/UI/Admin/AdminSystemInfo.cpp
  src/UI/Admin/AdminOverviewTab.h src/UI/Admin/AdminOverviewTab.cpp
  src/UI/Admin/AdminUsersTab.h src/UI/Admin/AdminUsersTab.cpp
  src/UI/Admin/AdminTabs.h src/UI/Admin/AdminTabs.cpp
  src/UI/Pages/IntegrationsPage.h src/UI/Pages/IntegrationsPage.cpp
  src/Integrations/IntegrationProfile.h src/Integrations/IntegrationProfile.cpp
  src/Integrations/IntegrationProfileManager.h src/Integrations/IntegrationProfileManager.cpp
  src/Integrations/VirtualDeviceDetector.h src/Integrations/VirtualDeviceDetector.cpp
  src/Integrations/RoutingDiagnostics.h src/Integrations/RoutingDiagnostics.cpp
  src/Integrations/ApplicationDetector.h src/Integrations/ApplicationDetector.cpp
  src/Platform/WindowsAudioSettingsLauncher.h
  src/App/AppPaths.h)
target_include_directories(BlackVoice PRIVATE src)
target_compile_definitions(BlackVoice PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_APPLICATION_NAME_STRING="BlackVoice" JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")
target_compile_options(BlackVoice PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_libraries(BlackVoice PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

enable_testing()
juce_add_console_app(BlackVoiceTests PRODUCT_NAME "BlackVoice Tests")
target_sources(BlackVoiceTests PRIVATE
  tests/Tests.cpp
  src/DSP/VoiceProcessor.cpp
  src/Presets/PresetManager.cpp
  src/Settings/SettingsManager.cpp
  src/UI/Navigation/PageRouter.cpp
  src/Integrations/IntegrationProfile.cpp
  src/Integrations/VirtualDeviceDetector.cpp
  src/Integrations/ApplicationDetector.cpp
  src/Admin/UserAccount.cpp
  src/Admin/PermissionManager.cpp
  src/Admin/AdminAccessController.cpp)
target_include_directories(BlackVoiceTests PRIVATE src)
target_compile_definitions(BlackVoiceTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_compile_options(BlackVoiceTests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_libraries(BlackVoiceTests PRIVATE juce::juce_audio_formats juce::juce_audio_devices juce::juce_dsp juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags)
add_test(NAME BlackVoiceTests COMMAND BlackVoiceTests)

install(TARGETS BlackVoice RUNTIME DESTINATION . RESOURCE DESTINATION Resources)
