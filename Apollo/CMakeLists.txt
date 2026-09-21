cmake_minimum_required(VERSION 3.15)

# The macOS 15 SDK marks CGWindowListCreateImage (still used by JUCE 8.0.0's
# juceaide) as obsoleted, which is a hard error when the deployment target is
# >= 15.0. A lower target turns it into the deprecation warning that JUCE
# already suppresses, so juceaide builds on the macOS 15 runners. Set before
# project()/FetchContent so it applies to juceaide too.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

project(Apollo VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(Apollo
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE GuML # GuitarML
    PLUGIN_CODE Apol # Apollo
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Apollo"
)

# Enable C++20 for std::span in Multirate.h
target_compile_features(Apollo PUBLIC cxx_std_20)

# Shared EarthDSPCore sources (outside the Apollo tree).
set(EARTH_SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared")
set(EARTH_CORE_SOURCES
    "${EARTH_SHARED_DIR}/EarthDSPCore.cpp"
    "${EARTH_SHARED_DIR}/Dattorro/Dattorro.cpp"
    "${EARTH_SHARED_DIR}/Dattorro/dsp/delays/InterpDelay.cpp"
    "${EARTH_SHARED_DIR}/Dattorro/dsp/filters/OnePoleFilters.cpp"
    "${EARTH_SHARED_DIR}/Effects/Overdrive.cpp"
)

target_sources(Apollo
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ApolloLookAndFeel.cpp
        Source/ApolloTheme.cpp
        ${EARTH_CORE_SOURCES}
)

target_compile_definitions(Apollo
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_DIRECT2D=0
)

target_link_libraries(Apollo
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Add DSP path to includes
# The shared core must be found before the legacy Source/DSP tree so that
# "Dattorro/Dattorro.hpp" resolves to shared/Dattorro (rate-context aware).
target_include_directories(Apollo
    PRIVATE
        Source
        "${EARTH_SHARED_DIR}"
        Source/DSP
)

juce_add_console_app(ApolloTest)
target_sources(ApolloTest PRIVATE Source/test_dsp.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/ApolloLookAndFeel.cpp Source/ApolloTheme.cpp ${EARTH_CORE_SOURCES})
target_include_directories(ApolloTest PRIVATE Source "${EARTH_SHARED_DIR}" Source/DSP)
target_link_libraries(ApolloTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
target_compile_features(ApolloTest PUBLIC cxx_std_20)

target_compile_definitions(ApolloTest PUBLIC JUCE_USE_DIRECT2D=0)

