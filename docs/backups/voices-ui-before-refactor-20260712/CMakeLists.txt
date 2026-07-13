cmake_minimum_required(VERSION 3.25)
project(ModificadorDeVoz VERSION 1.0.0 LANGUAGES CXX)

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

juce_add_gui_app(ModificadorDeVoz PRODUCT_NAME "Modificador de Voz" COMPANY_NAME "Modificador de Voz" VERSION ${PROJECT_VERSION}
  BUNDLE_ID "com.modificadordevoz.app"
  ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ModificadorDeVoz.png"
  ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ModificadorDeVoz.png")
set_target_properties(ModificadorDeVoz PROPERTIES OUTPUT_NAME "ModificadorDeVoz")
target_sources(ModificadorDeVoz PRIVATE
  src/App/Main.cpp src/App/MainWindow.h
  src/Audio/AudioEngine.h src/Audio/AudioEngine.cpp
  src/DSP/VoiceProcessor.h src/DSP/VoiceProcessor.cpp
  src/Presets/PresetManager.h src/Presets/PresetManager.cpp
  src/Settings/SettingsManager.h src/Settings/SettingsManager.cpp
  src/Diagnostics/DiagnosticsManager.h src/Diagnostics/DiagnosticsManager.cpp
  src/UI/MainComponent.h src/UI/MainComponent.cpp
  src/UI/Theme.h src/UI/AppLookAndFeel.h src/UI/AppLookAndFeel.cpp
  src/UI/ModernComponents.h src/UI/ModernComponents.cpp
  src/UI/Navigation/PageRouter.h src/UI/Navigation/PageRouter.cpp
  src/UI/Pages/SettingsPage.h src/UI/Pages/SettingsPage.cpp
  src/UI/Pages/ModulePage.h src/UI/Pages/ModulePage.cpp
  src/App/AppPaths.h)
target_include_directories(ModificadorDeVoz PRIVATE src)
target_compile_definitions(ModificadorDeVoz PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_APPLICATION_NAME_STRING="Modificador de Voz" JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")
target_compile_options(ModificadorDeVoz PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_libraries(ModificadorDeVoz PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

enable_testing()
juce_add_console_app(ModificadorDeVozTests PRODUCT_NAME "Modificador de Voz Tests")
target_sources(ModificadorDeVozTests PRIVATE tests/Tests.cpp src/DSP/VoiceProcessor.cpp src/Presets/PresetManager.cpp src/Settings/SettingsManager.cpp src/UI/Navigation/PageRouter.cpp)
target_include_directories(ModificadorDeVozTests PRIVATE src)
target_compile_definitions(ModificadorDeVozTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_compile_options(ModificadorDeVozTests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_libraries(ModificadorDeVozTests PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_data_structures PUBLIC juce::juce_recommended_config_flags)
add_test(NAME ModificadorDeVozTests COMMAND ModificadorDeVozTests)

install(TARGETS ModificadorDeVoz RUNTIME DESTINATION . RESOURCE DESTINATION Resources)
