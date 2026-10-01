#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ApolloLookAndFeel.h"
#include "ApolloTheme.h"
#include "PluginProcessor.h"

// Keeps the existing automatable boolean parameter, but makes a mouse/keyboard
// gesture behave like a gate instead of a latching switch.
class MomentaryGateButton : public juce::ToggleButton
{
public:
    MomentaryGateButton();
    ~MomentaryGateButton() override;
    void releaseLocalGate();
    void visibilityChanged() override;
    void enablementChanged() override;
    void parentHierarchyChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool keyIsDown) override;
    void focusLost (FocusChangeType cause) override;

private:
    bool localKeyGestureActive = false;
    bool localMouseGestureActive = false;
};

// A boolean bank whose two faces explicitly select the legacy polarity.
class ReverbFeedButton : public juce::ToggleButton
{
public:
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isEnabled() && e.mods.isLeftButtonDown())
        {
            mouseGesture = true;
            grabKeyboardFocus();
        }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (mouseGesture && isEnabled() && getLocalBounds().contains (e.getPosition()))
            setToggleState (e.x >= getWidth() / 2, juce::sendNotificationSync);
        mouseGesture = false;
    }
private:
    bool mouseGesture = false;
};

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
    void applyFontScale (float scale);

    float getDesignScale() const;
    juce::Rectangle<float> scaled (float x, float y, float w, float h) const;

    ApolloAudioProcessor& audioProcessor;
    ApolloLookAndFeel customLookAndFeel;

    // Non-interactive section captions on one shared instrument surface.
    ApolloRackPanel reverbPanel      { "SPACE" };
    ApolloRackPanel outputPanel      { "OUTPUT" };
    ApolloRackPanel octavePanel      { "OCTAVE" };
    ApolloRackPanel performancePanel { "PERFORM" };

    juce::TooltipWindow tooltipWindow { this, 650 };

    juce::Slider faderMix;
    juce::Slider knobDecay, knobPredelay, knobDamp, knobModSpeed, knobModDepth, knobEq1, knobEq2;
    juce::Label lblDecay, lblPredelay, lblDamp, lblModSpeed, lblModDepth, lblEq1, lblEq2;
    juce::Label valueDecay, valuePredelay, valueDamp, valueModSpeed, valueModDepth, valueEq1, valueEq2;
    juce::Label lblToneHigh, lblToneFlat, lblToneLow, lblDry, lblWet;

    ApolloSelector comboTimeScale, comboEffectMode, comboFootswitchMode;
    juce::Label lblTimeScale, lblEffectMode, lblFootswitchMode;
    juce::Label lblInputDiffusion, lblOctaveDryMix;
    juce::ToggleButton btnInputDiffusion, btnBypass;
    ReverbFeedButton btnOctaveDryMix;
    MomentaryGateButton btnMomentaryEffect;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachDecay, attachPredelay, attachDamp, attachModSpeed, attachModDepth, attachEq1, attachEq2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachTimeScale, attachEffectMode, attachFootswitchMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachInputDiffusion, attachOctaveDryMix, attachBypass, attachMomentaryEffect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessorEditor)
};
