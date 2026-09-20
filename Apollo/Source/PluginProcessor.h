#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "EarthDSPCore.h"

// Thin JUCE adapter for the shared EarthDSPCore.
//
// It only reads the APVTS, translates the values into earth::EarthParameters
// and calls the core. No sonic rule lives here. All APVTS ids, ranges and
// defaults are preserved. The pre-Stage-G implementation is kept at
// PluginProcessor_legacy.cpp/.h.disabled for rollback until the plugin build is
// validated.
//
class ApolloAudioProcessor : public juce::AudioProcessor
{
public:
    ApolloAudioProcessor();
    ~ApolloAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    earth::EarthDSPCore core_;
    earth::EarthParameters params_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessor)
};
