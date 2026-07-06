#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Dattorro/Dattorro.hpp"
#include "DSP/DaisySP/overdrive.h"
#include "DSP/Util/Multirate.h"
#include "DSP/Util/OctaveGenerator.h"
#include <juce_dsp/juce_dsp.h>

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

    // DSP Components
    Dattorro reverb;
    daisysp::Overdrive overdriveLeft, overdriveRight;
    std::unique_ptr<OctaveGenerator> octave;
    
    // JUCE filters instead of cycfi q
    juce::dsp::IIR::Filter<float> eq1; // Highshelf
    juce::dsp::IIR::Filter<float> eq2; // Lowshelf

    // Resamplers for Octave path
    juce::LagrangeInterpolator octaveResamplerUp;
    juce::LagrangeInterpolator octaveResamplerDown;
    juce::AudioBuffer<float> resampleBuffer48k;
    
    // Anti-aliasing filter before downsampling (8th order = 4 biquads)
    std::array<juce::dsp::IIR::Filter<float>, 4> antiAliasFilters;

    // Sliding window FIFOs for continuous resampling
    juce::AudioBuffer<float> slideUp;
    int slideUpValid = 0;
    double phaseUp = 0.0;
    
    juce::AudioBuffer<float> slideDown;
    int slideDownValid = 0;
    double phaseDown = 0.0;

    // Buffers and variables for Multirate/Octave
    static constexpr int resample_factor = 6;
    Decimator2 decimate;
    Interpolator interpolate;
    std::array<float, resample_factor> buff{};
    std::array<float, resample_factor> buff_out{};
    int bin_counter = 0;

    // Smoothed values
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_predelay;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_moddepth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_modspeed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_freezeDecay;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_ODswell;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_dryMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_wetMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> current_damp;
    
    // Bypass smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassFade;

    // State
    bool odOn = false;
    bool freeze = false;
    float setOD = 0.4f;
    float pmix = -1.0f;
    float pdamp = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessor)
};
