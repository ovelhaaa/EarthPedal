cmake_minimum_required(VERSION 3.15)

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

target_sources(Apollo
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Dattorro/Dattorro.cpp
        Source/DSP/Dattorro/dsp/delays/InterpDelay.cpp
        Source/DSP/Dattorro/dsp/filters/OnePoleFilters.cpp
        Source/DSP/DaisySP/overdrive.cpp
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
target_include_directories(Apollo
    PRIVATE
        Source
        Source/DSP
)

juce_add_console_app(ApolloTest)
target_sources(ApolloTest PRIVATE Source/test_dsp.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/DSP/Dattorro/Dattorro.cpp Source/DSP/Dattorro/dsp/delays/InterpDelay.cpp Source/DSP/Dattorro/dsp/filters/OnePoleFilters.cpp Source/DSP/DaisySP/overdrive.cpp)
target_include_directories(ApolloTest PRIVATE Source Source/DSP)
target_link_libraries(ApolloTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors)
target_compile_features(ApolloTest PUBLIC cxx_std_20)

target_compile_definitions(ApolloTest PUBLIC JUCE_USE_DIRECT2D=0)

