#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ApolloLookAndFeel.h"
#include "PluginProcessor.h"

class ApolloAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit ApolloAudioProcessorEditor (ApolloAudioProcessor&);
    ~ApolloAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateStatePresentation();
    void updateValueLabels();

    ApolloAudioProcessor& audioProcessor;
    ApolloLookAndFeel customLookAndFeel;

    juce::Label titleLabel, globalStateLabel, helpLabel;
    juce::GroupComponent reverbGroup, octaveGroup, performanceGroup, outputGroup;
    juce::Label octaveStateLabel, performanceStateLabel;

    juce::Slider faderMix;
    juce::Slider knobDecay, knobPredelay, knobDamp, knobModSpeed, knobModDepth, knobEq1, knobEq2;
    juce::Label lblMix, lblDecay, lblPredelay, lblDamp, lblModSpeed, lblModDepth, lblEq1, lblEq2;
    juce::Label valueMix, valueDecay, valuePredelay, valueDamp, valueModSpeed, valueModDepth, valueEq1, valueEq2;

    juce::ComboBox comboTimeScale, comboEffectMode, comboFootswitchMode;
    juce::Label lblTimeScale, lblEffectMode, lblFootswitchMode;
    juce::ToggleButton btnInputDiffusion, btnOctaveDryMix, btnBypass, btnMomentaryEffect;
    juce::Label dryLabel, wetLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachDecay, attachPredelay, attachDamp, attachModSpeed, attachModDepth, attachEq1, attachEq2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachTimeScale, attachEffectMode, attachFootswitchMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachInputDiffusion, attachOctaveDryMix, attachBypass, attachMomentaryEffect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessorEditor)
};
