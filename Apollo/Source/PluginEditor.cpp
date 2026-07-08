#include "PluginProcessor.h"
#include "PluginEditor.h"

// Helper function to setup knobs
static void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
}

// Helper function to setup toggles
static void setupToggle(juce::ToggleButton& btn, const juce::String& text)
{
    btn.setButtonText(text);
    btn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff6a00));
    btn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffffffff));
}

ApolloAudioProcessorEditor::ApolloAudioProcessorEditor (ApolloAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (700, 420);
    setLookAndFeel (&customLookAndFeel);

    // --- Title ---
    addAndMakeVisible(titleLabel);
    titleLabel.setText("APOLLO VST", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(24.0f).withStyle(juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff6a00));

    // --- Mix Group ---
    addAndMakeVisible(groupMix);
    groupMix.setText("Mix");
    groupMix.setTextLabelPosition(juce::Justification::centredTop);
    groupMix.setColour(juce::GroupComponent::textColourId, juce::Colour(0xffffffff));
    groupMix.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff555555));

    groupMix.addAndMakeVisible(faderMix);
    faderMix.setSliderStyle(juce::Slider::LinearVertical);
    faderMix.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    faderMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "mix", faderMix);

    groupMix.addAndMakeVisible(lblMix);
    lblMix.setText("Wet/Dry", juce::dontSendNotification);
    lblMix.setJustificationType(juce::Justification::centred);
    lblMix.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));

    // --- Knobs ---
    addAndMakeVisible(knobPredelay); setupKnob(knobPredelay, lblPredelay, "Pre-Delay");
    attachPredelay = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "predelay", knobPredelay);
    addAndMakeVisible(lblPredelay);

    addAndMakeVisible(knobDecay); setupKnob(knobDecay, lblDecay, "Decay");
    attachDecay = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "decay", knobDecay);
    addAndMakeVisible(lblDecay);

    addAndMakeVisible(knobDamp); setupKnob(knobDamp, lblDamp, "Damp");
    attachDamp = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "damp", knobDamp);
    addAndMakeVisible(lblDamp);

    addAndMakeVisible(knobModSpeed); setupKnob(knobModSpeed, lblModSpeed, "Mod Speed");
    attachModSpeed = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "modspeed", knobModSpeed);
    addAndMakeVisible(lblModSpeed);

    addAndMakeVisible(knobModDepth); setupKnob(knobModDepth, lblModDepth, "Mod Depth");
    attachModDepth = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "moddepth", knobModDepth);
    addAndMakeVisible(lblModDepth);

    addAndMakeVisible(knobEq1); setupKnob(knobEq1, lblEq1, "Octave High");
    attachEq1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "eq1_gain", knobEq1);
    addAndMakeVisible(lblEq1);

    addAndMakeVisible(knobEq2); setupKnob(knobEq2, lblEq2, "Octave Low");
    attachEq2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "eq2_gain", knobEq2);
    addAndMakeVisible(lblEq2);

    // --- ComboBoxes ---
    addAndMakeVisible(lblTimeScale);
    lblTimeScale.setText("Time Scale", juce::dontSendNotification);
    lblTimeScale.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
    addAndMakeVisible(comboTimeScale);
    comboTimeScale.addItemList({"Small", "Medium", "Large"}, 1);
    attachTimeScale = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "time_scale", comboTimeScale);

    addAndMakeVisible(lblEffectMode);
    lblEffectMode.setText("Effect Mode", juce::dontSendNotification);
    lblEffectMode.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
    addAndMakeVisible(comboEffectMode);
    comboEffectMode.addItemList({"None", "Up Octave", "Down Octave", "Both Octaves"}, 1);
    attachEffectMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "effect_mode", comboEffectMode);

    addAndMakeVisible(lblFootswitchMode);
    lblFootswitchMode.setText("Switch Mode", juce::dontSendNotification);
    lblFootswitchMode.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
    addAndMakeVisible(comboFootswitchMode);
    comboFootswitchMode.addItemList({"Freeze", "Overdrive", "Effect"}, 1);
    attachFootswitchMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "footswitch_mode", comboFootswitchMode);

    // --- Toggle Buttons ---
    addAndMakeVisible(btnInputDiffusion); setupToggle(btnInputDiffusion, "Input Diffusion");
    attachInputDiffusion = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "input_diffusion", btnInputDiffusion);

    addAndMakeVisible(btnOctaveDryMix); setupToggle(btnOctaveDryMix, "Octave Dry Mix");
    attachOctaveDryMix = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "octave_dry_mix", btnOctaveDryMix);

    addAndMakeVisible(btnMomentaryEffect); setupToggle(btnMomentaryEffect, "Momentary Switch");
    attachMomentaryEffect = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "momentary_effect", btnMomentaryEffect);

    addAndMakeVisible(btnBypass);
    btnBypass.setButtonText("BYPASS");
    btnBypass.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff0000));
    btnBypass.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff0000));
    attachBypass = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", btnBypass);
    dropShadowBypass.setShadowProperties({juce::Colours::black.withAlpha(0.7f), 6, juce::Point<int>(0, 3)});
    btnBypass.setComponentEffect(&dropShadowBypass);
}

ApolloAudioProcessorEditor::~ApolloAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark modern sci-fi background
    g.fillAll (juce::Colour (0xff111111));
    
    // Add a dark radial gradient for depth
    juce::Graphics::ScopedSaveState state (g);
    juce::ColourGradient gradient (juce::Colour(0xff222222), getWidth() * 0.5f, getHeight() * 0.5f,
                                   juce::Colour(0xff0a0a0a), 0, 0, true);
    g.setGradientFill (gradient);
    g.fillAll();
    
    // Subtle texture/grid
    g.setColour (juce::Colour(0xff181818));
    for (int i = 0; i < getWidth(); i += 20)
        g.drawVerticalLine(i, 0, (float)getHeight());
    for (int i = 0; i < getHeight(); i += 20)
        g.drawHorizontalLine(i, 0, (float)getWidth());
}

void ApolloAudioProcessorEditor::resized()
{
    // Title
    titleLabel.setBounds(0, 10, getWidth(), 30);

    // Left Panel - Mix
    groupMix.setBounds(20, 50, 90, 330);
    faderMix.setBounds(15, 30, 60, 260);
    lblMix.setBounds(5, 300, 80, 20);

    // Center Panel - Knobs
    int startX = 130;
    int startY = 80;
    int knobSize = 75;
    int spacingX = 85;
    int spacingY = 110;

    // Row 1
    knobPredelay.setBounds(startX, startY, knobSize, knobSize);
    lblPredelay.setBounds(startX, startY - 20, knobSize, 20);

    knobDecay.setBounds(startX + spacingX, startY, knobSize, knobSize);
    lblDecay.setBounds(startX + spacingX, startY - 20, knobSize, 20);

    knobDamp.setBounds(startX + 2 * spacingX, startY, knobSize, knobSize);
    lblDamp.setBounds(startX + 2 * spacingX, startY - 20, knobSize, 20);

    knobEq1.setBounds(startX + 3 * spacingX, startY, knobSize, knobSize);
    lblEq1.setBounds(startX + 3 * spacingX, startY - 20, knobSize, 20);

    // Row 2
    knobModSpeed.setBounds(startX, startY + spacingY, knobSize, knobSize);
    lblModSpeed.setBounds(startX, startY + spacingY - 20, knobSize, 20);

    knobModDepth.setBounds(startX + spacingX, startY + spacingY, knobSize, knobSize);
    lblModDepth.setBounds(startX + spacingX, startY + spacingY - 20, knobSize, 20);

    knobEq2.setBounds(startX + 2 * spacingX, startY + spacingY, knobSize, knobSize);
    lblEq2.setBounds(startX + 2 * spacingX, startY + spacingY - 20, knobSize, 20);

    // Bypass button next to row 2
    btnBypass.setBounds(startX + 3 * spacingX, startY + spacingY + 15, knobSize + 10, 40);

    // Right Panel - Combos and Toggles
    int rightX = 520;
    int comboY = 80;
    int comboWidth = 140;
    int comboHeight = 24;
    int comboSpacing = 60;

    lblTimeScale.setBounds(rightX, comboY - 20, comboWidth, 20);
    comboTimeScale.setBounds(rightX, comboY, comboWidth, comboHeight);

    lblEffectMode.setBounds(rightX, comboY + comboSpacing - 20, comboWidth, 20);
    comboEffectMode.setBounds(rightX, comboY + comboSpacing, comboWidth, comboHeight);

    lblFootswitchMode.setBounds(rightX, comboY + 2 * comboSpacing - 20, comboWidth, 20);
    comboFootswitchMode.setBounds(rightX, comboY + 2 * comboSpacing, comboWidth, comboHeight);

    // Toggles below combos
    int toggleY = comboY + 3 * comboSpacing - 10;
    int toggleHeight = 24;
    
    btnInputDiffusion.setBounds(rightX, toggleY, comboWidth + 20, toggleHeight);
    btnOctaveDryMix.setBounds(rightX, toggleY + 30, comboWidth + 20, toggleHeight);
    btnMomentaryEffect.setBounds(rightX, toggleY + 60, comboWidth + 20, toggleHeight);
}
