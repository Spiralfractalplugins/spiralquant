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

file(GLOB SQ_PRESET_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/presets/*.sp303preset")
juce_add_binary_data(SpiralQuantPresets SOURCES ${SQ_PRESET_SOURCES})

set(SQ_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons")
if(APPLE AND EXISTS "${SQ_ICON_DIR}/SpiralQuant.icns")
    set(SQ_ICON_ARGS ICON_icns "${SQ_ICON_DIR}/SpiralQuant.icns")
elseif(WIN32 AND EXISTS "${SQ_ICON_DIR}/SpiralQuant.ico")
    set(SQ_ICON_ARGS ICON_win_ico "${SQ_ICON_DIR}/SpiralQuant.ico" ICON_win_embed TRUE)
else()
    set(SQ_ICON_ARGS "")
endif()

set(SPIRALQUANT_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png")

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
    ${SQ_ICON_ARGS}
)

if (APPLE)
    set_property(TARGET SpiralQuant PROPERTY JUCE_COPY_PLUGIN_AFTER_BUILD TRUE)
    juce_enable_copy_plugin_step(SpiralQuant)
endif()

target_sources(SpiralQuant PRIVATE
    Source/PluginProcessor.cpp
    Source/CreateEditor.cpp
    Source/PluginEditor.cpp
    Source/gui/QuantumCanvas.cpp
    Source/gui/AdsrCanvas.cpp
    Source/dsp/QuantumOsc.cpp
    Source/dsp/QuantumVoice.cpp
    Source/dsp/QuantumEngine.cpp
    Source/dsp/MasterFx.cpp
)

target_link_libraries(SpiralQuant PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    SpiralQuantPresets
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
    Source/gui/QuantumCanvas.cpp
    Source/gui/AdsrCanvas.cpp
    Source/dsp/QuantumOsc.cpp
    Source/dsp/QuantumVoice.cpp
    Source/dsp/QuantumEngine.cpp
    Source/dsp/MasterFx.cpp
)
target_include_directories(SpiralQuantTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(SpiralQuantTests PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    SpiralQuantPresets
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

# Deterministic, allocation-free DSP wants aggressive but safe optimization.
# Deliberately no -ffast-math by default: it breaks the isfinite NaN guards.
option(SPIRALQUANT_OFAST "Use -Ofast for DSP code (faster math; NaN guards stay, denormal behaviour changes)" OFF)
set(SPIRALQUANT_PGO "OFF" CACHE STRING "PGO mode: OFF, generate, or use")
set(SPIRALQUANT_PGO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pgo" CACHE PATH "PGO profile directory")

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    if(SPIRALQUANT_OFAST)
        set(SQ_OPT_FLAGS -Ofast)
    else()
        set(SQ_OPT_FLAGS -O3 -fno-math-errno -ffp-contract=fast)
    endif()
    set(SQ_PGO_FLAGS "")
    if(SPIRALQUANT_PGO STREQUAL "generate")
        set(SQ_PGO_FLAGS -fprofile-generate=${SPIRALQUANT_PGO_DIR})
    elseif(SPIRALQUANT_PGO STREQUAL "use")
        set(SQ_PGO_FLAGS -fprofile-use=${SPIRALQUANT_PGO_DIR})
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND SQ_PGO_FLAGS -Wno-missing-profile)
        endif()
    endif()
    target_compile_options(SpiralQuant PRIVATE ${SQ_OPT_FLAGS} ${SQ_PGO_FLAGS})
    target_link_libraries(SpiralQuant PRIVATE ${SQ_PGO_FLAGS})
    target_compile_options(SpiralQuantTests PRIVATE ${SQ_OPT_FLAGS} ${SQ_PGO_FLAGS})
    target_link_libraries(SpiralQuantTests PRIVATE ${SQ_PGO_FLAGS})
elseif(MSVC)
    target_compile_options(SpiralQuant PRIVATE /O2 /fp:fast)
    target_compile_options(SpiralQuantTests PRIVATE /O2 /fp:fast)
endif()