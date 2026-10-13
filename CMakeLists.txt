cmake_minimum_required(VERSION 3.24)

if (APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures to build" FORCE)
endif()

project(SpiralQuant VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found in vendor/JUCE.")
endif()

add_subdirectory(vendor/JUCE)

juce_add_plugin(SpiralQuant
    COMPANY_NAME "Spiralfractal"
    PLUGIN_MANUFACTURER_CODE Sqnt
    PLUGIN_CODE SpQn
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "SpiralQuant"
    BUNDLE_ID "com.spiralquant.plugin"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

if (APPLE)
    set_property(TARGET SpiralQuant PROPERTY JUCE_COPY_PLUGIN_AFTER_BUILD TRUE)
    juce_enable_copy_plugin_step(SpiralQuant)
endif()

target_sources(SpiralQuant PRIVATE
    Source/PluginProcessor.cpp
    Source/CreateEditor.cpp
    Source/PluginEditor.cpp
    Source/dsp/QuantumOsc.cpp
    Source/dsp/QuantumVoice.cpp
    Source/dsp/QuantumEngine.cpp
)

target_link_libraries(SpiralQuant PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

target_compile_definitions(SpiralQuant PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    SPIRALQUANT_VERSION="${PROJECT_VERSION}"
)

if (MSVC)
    target_compile_options(SpiralQuant PRIVATE /utf-8)
endif()

juce_generate_juce_header(SpiralQuant)

juce_add_console_app(SpiralQuantTests)
target_sources(SpiralQuantTests PRIVATE
    tests/TestRender.cpp
    Source/PluginProcessor.cpp
    Source/CreateEditor.cpp
    Source/PluginEditor.cpp
    Source/dsp/QuantumOsc.cpp
    Source/dsp/QuantumVoice.cpp
    Source/dsp/QuantumEngine.cpp
)
target_include_directories(SpiralQuantTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(SpiralQuantTests PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
target_compile_definitions(SpiralQuantTests PRIVATE
    SPIRALQUANT_TESTS=1
    SPIRALQUANT_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
if (MSVC)
    target_compile_options(SpiralQuantTests PRIVATE /utf-8)
endif()
juce_generate_juce_header(SpiralQuantTests)