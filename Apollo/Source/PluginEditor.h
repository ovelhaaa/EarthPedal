#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ApolloLookAndFeel.h"
#include "PluginProcessor.h"

class ApolloAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ApolloAudioProcessorEditor (ApolloAudioProcessor&);
    ~ApolloAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ApolloAudioProcessor& audioProcessor;

    // --- GUI Components ---
    juce::GroupComponent groupMix;
    juce::Slider faderMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderMixAttachment;
    juce::Label lblMix;

    // Knobs
    juce::Slider knobDecay, knobPredelay, knobDamp, knobModSpeed, knobModDepth, knobEq1, knobEq2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachDecay, attachPredelay, attachDamp, attachModSpeed, attachModDepth, attachEq1, attachEq2;
    juce::Label lblDecay, lblPredelay, lblDamp, lblModSpeed, lblModDepth, lblEq1, lblEq2;

    // ComboBoxes
    juce::ComboBox comboTimeScale, comboEffectMode, comboFootswitchMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachTimeScale, attachEffectMode, attachFootswitchMode;
    juce::Label lblTimeScale, lblEffectMode, lblFootswitchMode;

    // Toggle Buttons
    juce::ToggleButton btnInputDiffusion, btnOctaveDryMix, btnBypass, btnMomentaryEffect;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachInputDiffusion, attachOctaveDryMix, attachBypass, attachMomentaryEffect;

    juce::DropShadowEffect dropShadowBypass;
    juce::Label titleLabel;

    ApolloLookAndFeel customLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessorEditor)
};
