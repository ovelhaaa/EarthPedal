#include "PluginEditor.h"

namespace
{
const auto amber = juce::Colour (0xffffa13b);
const auto text = juce::Colour (0xfff2eee8);
const auto muted = juce::Colour (0xffaaa49d);

void MomentaryGateButton::mouseDown (const juce::MouseEvent&)
{
    setToggleState (true, juce::sendNotification);
}

void MomentaryGateButton::mouseUp (const juce::MouseEvent&)
{
    setToggleState (false, juce::sendNotification);
}

bool MomentaryGateButton::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        gateKeyIsDown = true;
        setToggleState (true, juce::sendNotification);
        return true;
    }

    return juce::ToggleButton::keyPressed (key);
}

bool MomentaryGateButton::keyStateChanged (bool isKeyDown)
{
    if (gateKeyIsDown && ! isKeyDown)
    {
        gateKeyIsDown = false;
        setToggleState (false, juce::sendNotification);
        return true;
    }

    return juce::ToggleButton::keyStateChanged (isKeyDown);
}

void MomentaryGateButton::focusLost (FocusChangeType cause)
{
    juce::ignoreUnused (cause);
    gateKeyIsDown = false;
    setToggleState (false, juce::sendNotification);
    juce::ToggleButton::focusLost (cause);
}

void setupLabel (juce::Label& label, const juce::String& caption, float size = 12.0f)
{
    label.setText (caption, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (size));
    label.setColour (juce::Label::textColourId, text);
}

void setupKnob (juce::Slider& slider, juce::Label& label, juce::Label& value, const juce::String& caption)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setWantsKeyboardFocus (true);
    setupLabel (label, caption);
    setupLabel (value, "", 11.0f);
    value.setColour (juce::Label::textColourId, amber);
}

void setupToggle (juce::ToggleButton& button, const juce::String& caption, const juce::String& tooltip)
{
    button.setButtonText (caption);
    button.setTooltip (tooltip);
    button.setWantsKeyboardFocus (true);
}
}

ApolloAudioProcessorEditor::ApolloAudioProcessorEditor (ApolloAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (900, 620);
    setWantsKeyboardFocus (true);
    setLookAndFeel (&customLookAndFeel);

    addAndMakeVisible (titleLabel); setupLabel (titleLabel, "APOLLO", 26.0f);
    titleLabel.setFont (titleLabel.getFont().boldened());
    titleLabel.setColour (juce::Label::textColourId, amber);
    addAndMakeVisible (globalStateLabel); setupLabel (globalStateLabel, "ACTIVE", 12.0f);
    addAndMakeVisible (helpLabel); setupLabel (helpLabel, "?  Contextual help", 11.0f);
    helpLabel.setColour (juce::Label::textColourId, muted);

    for (auto* group : { &reverbGroup, &octaveGroup, &performanceGroup, &outputGroup })
    {
        addAndMakeVisible (*group);
        group->setColour (juce::GroupComponent::textColourId, text);
        group->setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff5a554e));
    }
    reverbGroup.setText ("REVERB"); octaveGroup.setText ("OCTAVE");
    performanceGroup.setText ("PERFORMANCE"); outputGroup.setText ("OUTPUT");
    addAndMakeVisible (octaveStateLabel); setupLabel (octaveStateLabel, "OCTAVE OFF", 11.0f);
    addAndMakeVisible (performanceStateLabel); setupLabel (performanceStateLabel, "PERFORM READY", 11.0f);

    addAndMakeVisible (faderMix);
    faderMix.setSliderStyle (juce::Slider::LinearVertical);
    faderMix.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    faderMix.setWantsKeyboardFocus (true);
    faderMix.setDoubleClickReturnValue (true, 0.5);
    faderMix.setTooltip ("Equilibra sinal direto e reverb.");
    faderMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "mix", faderMix);
    addAndMakeVisible (lblMix); setupLabel (lblMix, "MIX", 14.0f);
    addAndMakeVisible (valueMix); setupLabel (valueMix, "", 12.0f); valueMix.setColour (juce::Label::textColourId, amber);
    addAndMakeVisible (dryLabel); setupLabel (dryLabel, "DRY", 10.0f);
    addAndMakeVisible (wetLabel); setupLabel (wetLabel, "WET", 10.0f);

    auto addKnob = [this] (juce::Slider& knob, juce::Label& label, juce::Label& value, const char* id, const juce::String& caption,
                            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment)
    {
        addAndMakeVisible (knob); addAndMakeVisible (label); addAndMakeVisible (value);
        setupKnob (knob, label, value, caption);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id, knob);
        auto* parameter = audioProcessor.apvts.getParameter (id);
        knob.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    };
    addKnob (knobPredelay, lblPredelay, valuePredelay, "predelay", "Pre-delay", attachPredelay);
    addKnob (knobDecay, lblDecay, valueDecay, "decay", "Decay", attachDecay);
    addKnob (knobDamp, lblDamp, valueDamp, "damp", "Tone", attachDamp);
    addKnob (knobModSpeed, lblModSpeed, valueModSpeed, "modspeed", "Mod Rate", attachModSpeed);
    addKnob (knobModDepth, lblModDepth, valueModDepth, "moddepth", "Mod Depth", attachModDepth);
    addKnob (knobEq1, lblEq1, valueEq1, "eq1_gain", "Octave High Shelf", attachEq1);
    addKnob (knobEq2, lblEq2, valueEq2, "eq2_gain", "Octave Low Shelf", attachEq2);
    knobPredelay.setTooltip ("Atrasa a entrada do reverb.");
    knobDecay.setTooltip ("Define quanto tempo o reverb sustenta.");
    knobDamp.setTooltip ("High Cut à esquerda; Low Cut à direita.");
    knobModSpeed.setTooltip ("Define a velocidade do movimento.");
    knobModDepth.setTooltip ("Define a intensidade do movimento.");
    knobEq1.setTooltip ("Ajusta o shelf alto da ramificação de oitava.");
    knobEq2.setTooltip ("Ajusta o shelf baixo da ramificação de oitava.");

    auto addChoice = [this] (juce::ComboBox& combo, juce::Label& label, const char* id, const juce::String& caption,
                              const juce::StringArray& choices, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attachment)
    {
        addAndMakeVisible (label); setupLabel (label, caption); addAndMakeVisible (combo);
        combo.addItemList (choices, 1); combo.setWantsKeyboardFocus (true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, id, combo);
    };
    addChoice (comboTimeScale, lblTimeScale, "time_scale", "Size", { "Small", "Medium", "Large" }, attachTimeScale);
    addChoice (comboEffectMode, lblEffectMode, "effect_mode", "Octave Mode", { "Off", "Up", "Down", "Up + Down" }, attachEffectMode);
    addChoice (comboFootswitchMode, lblFootswitchMode, "footswitch_mode", "Perform Action", { "Freeze", "Overdrive", "Octave Perform" }, attachFootswitchMode);
    comboTimeScale.setTooltip ("Escolhe o tamanho do espaço.");
    comboEffectMode.setTooltip ("Escolhe None, Up, Down ou Both para a ramificação de oitava.");
    comboFootswitchMode.setTooltip ("Escolhe a ação que Perform / Gate executa enquanto é sustentado.");

    addAndMakeVisible (btnInputDiffusion); setupToggle (btnInputDiffusion, "Input Diffusion", "Espalha o sinal antes do plate.");
    attachInputDiffusion = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "input_diffusion", btnInputDiffusion);
    addAndMakeVisible (btnOctaveDryMix); setupToggle (btnOctaveDryMix, "Octave Dry Routing (pending)", "Roteamento dry da ramificação de oitava — validação pendente.");
    attachOctaveDryMix = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "octave_dry_mix", btnOctaveDryMix);
    addAndMakeVisible (btnMomentaryEffect); setupToggle (btnMomentaryEffect, "PERFORM / GATE", "Sustente para executar a ação selecionada.");
    attachMomentaryEffect = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "momentary_effect", btnMomentaryEffect);
    addAndMakeVisible (btnBypass); setupToggle (btnBypass, "BYPASS", "Bypass interno: passa o sinal direto; não é o bypass do host.");
    attachBypass = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "bypass", btnBypass);

    updateStatePresentation(); updateValueLabels();
    startTimerHz (12);
}

ApolloAudioProcessorEditor::~ApolloAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ApolloAudioProcessorEditor::timerCallback()
{
    updateStatePresentation();
    updateValueLabels();
}

void ApolloAudioProcessorEditor::updateStatePresentation()
{
    const auto mode = (int) std::round (audioProcessor.apvts.getRawParameterValue ("effect_mode")->load());
    const auto action = (int) std::round (audioProcessor.apvts.getRawParameterValue ("footswitch_mode")->load());
    const bool perform = audioProcessor.apvts.getRawParameterValue ("momentary_effect")->load() > 0.5f;
    const bool bypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    const bool octaveOff = mode == 0;
    const float octaveAlpha = octaveOff ? 0.42f : 1.0f;
    for (juce::Component* component : { static_cast<juce::Component*> (&knobEq1), static_cast<juce::Component*> (&knobEq2),
                                        static_cast<juce::Component*> (&lblEq1), static_cast<juce::Component*> (&lblEq2),
                                        static_cast<juce::Component*> (&valueEq1), static_cast<juce::Component*> (&valueEq2),
                                        static_cast<juce::Component*> (&btnOctaveDryMix) })
        component->setAlpha (octaveAlpha);
    octaveStateLabel.setText (octaveOff ? "OCTAVE OFF — controls remain automatable" : "OCTAVE ACTIVE", juce::dontSendNotification);
    octaveStateLabel.setColour (juce::Label::textColourId, octaveOff ? muted : amber);
    globalStateLabel.setText (bypassed ? "Bypassed — internal dry path" : "Active — processing", juce::dontSendNotification);
    globalStateLabel.setColour (juce::Label::textColourId, bypassed ? amber : text);
    juce::String state = "Perform Ready";
    if (perform && action == 0) state = "Freeze Active";
    else if (perform && action == 1) state = "Drive Active";
    else if (perform && action == 2) state = octaveOff ? "No Octave Mode Selected" : "Octave Perform Active";
    performanceStateLabel.setText (state, juce::dontSendNotification);
    performanceStateLabel.setColour (juce::Label::textColourId, perform ? amber : muted);
    btnInputDiffusion.setButtonText (juce::String ("Input Diffusion — ") + (audioProcessor.apvts.getRawParameterValue ("input_diffusion")->load() > 0.5f ? "On" : "Off"));
    btnOctaveDryMix.setButtonText (juce::String ("Octave Dry Routing (pending) — ") + (audioProcessor.apvts.getRawParameterValue ("octave_dry_mix")->load() > 0.5f ? "On" : "Off"));
    btnBypass.setButtonText (juce::String ("BYPASS — ") + (bypassed ? "On" : "Off"));
    btnMomentaryEffect.setButtonText ("PERFORM / GATE — " + (action == 0 ? "Freeze" : action == 1 ? "Overdrive" : "Effect"));
}

void ApolloAudioProcessorEditor::updateValueLabels()
{
    auto percent = [] (juce::Slider& s) { return juce::String (juce::roundToInt (s.getValue() * 100.0)) + "%"; };
    valuePredelay.setText (juce::String (juce::roundToInt (knobPredelay.getValue() * 1000.0)) + " ms", juce::dontSendNotification);
    valueDecay.setText (percent (knobDecay), juce::dontSendNotification); valueDamp.setText (percent (knobDamp), juce::dontSendNotification);
    valueModSpeed.setText (percent (knobModSpeed), juce::dontSendNotification); valueModDepth.setText (percent (knobModDepth), juce::dontSendNotification);
    valueEq1.setText (juce::String (knobEq1.getValue(), 1) + " dB", juce::dontSendNotification); valueEq2.setText (juce::String (knobEq2.getValue(), 1) + " dB", juce::dontSendNotification);
    valueMix.setText (percent (faderMix), juce::dontSendNotification);
}

void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff131211));
    g.setColour (juce::Colour (0xff201e1b));
    for (int x = 0; x < getWidth(); x += 24) g.drawVerticalLine (x, 0.0f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    g.setColour (amber.withAlpha (0.65f));
    g.fillRect (20, 54, getWidth() - 40, 1);
}

void ApolloAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    auto header = area.removeFromTop (46); titleLabel.setBounds (header.removeFromLeft (170)); globalStateLabel.setBounds (header.removeFromLeft (260)); helpLabel.setBounds (header.removeFromRight (150));
    area.removeFromTop (10);
    auto top = area.removeFromTop (330); auto bottom = area.removeFromTop (190);
    auto reverb = top.removeFromLeft (500); top.removeFromLeft (10); auto output = top;
    reverbGroup.setBounds (reverb); outputGroup.setBounds (output);
    auto octave = bottom.removeFromLeft (500); bottom.removeFromLeft (10); auto performance = bottom;
    octaveGroup.setBounds (octave); performanceGroup.setBounds (performance);

    auto placeKnob = [] (juce::Rectangle<int> cell, juce::Slider& knob, juce::Label& label, juce::Label& value) { label.setBounds (cell.removeFromTop (18)); value.setBounds (cell.removeFromBottom (18)); knob.setBounds (cell.reduced (8, 0)); };
    auto reverbControls = reverb.reduced (15, 35); auto row1 = reverbControls.removeFromTop (135); auto row2 = reverbControls.removeFromTop (135);
    const int cell = 82;
    placeKnob (row1.removeFromLeft (cell), knobPredelay, lblPredelay, valuePredelay); placeKnob (row1.removeFromLeft (cell), knobDecay, lblDecay, valueDecay); placeKnob (row1.removeFromLeft (cell), knobDamp, lblDamp, valueDamp);
    lblTimeScale.setBounds (row1.removeFromLeft (90).removeFromTop (20)); comboTimeScale.setBounds (reverb.getX() + 280, reverb.getY() + 73, 110, 26);
    btnInputDiffusion.setBounds (reverb.getX() + 390, reverb.getY() + 73, 95, 26);
    placeKnob (row2.removeFromLeft (cell), knobModSpeed, lblModSpeed, valueModSpeed); placeKnob (row2.removeFromLeft (cell), knobModDepth, lblModDepth, valueModDepth);

    auto octaveControls = octave.reduced (15, 32); lblEffectMode.setBounds (octaveControls.removeFromTop (18).removeFromLeft (150)); comboEffectMode.setBounds (octave.getX() + 15, octave.getY() + 51, 155, 25); octaveStateLabel.setBounds (octave.getX() + 185, octave.getY() + 52, 295, 24);
    auto octaveKnobs = octaveControls.withTrimmedTop (32); placeKnob (octaveKnobs.removeFromLeft (125), knobEq1, lblEq1, valueEq1); placeKnob (octaveKnobs.removeFromLeft (125), knobEq2, lblEq2, valueEq2); btnOctaveDryMix.setBounds (octave.getX() + 270, octave.getY() + 95, 200, 28);

    lblFootswitchMode.setBounds (performance.getX() + 15, performance.getY() + 33, 150, 18); comboFootswitchMode.setBounds (performance.getX() + 15, performance.getY() + 53, 160, 25); btnMomentaryEffect.setBounds (performance.getX() + 195, performance.getY() + 52, 185, 32); performanceStateLabel.setBounds (performance.getX() + 15, performance.getY() + 105, performance.getWidth() - 30, 26);
    auto outputControls = output.reduced (25, 38); lblMix.setBounds (outputControls.getX(), outputControls.getY(), outputControls.getWidth(), 20); valueMix.setBounds (outputControls.getX(), outputControls.getBottom() - 22, outputControls.getWidth(), 20); faderMix.setBounds (output.getX() + 65, output.getY() + 75, 70, 190); dryLabel.setBounds (output.getX() + 20, output.getBottom() - 48, 48, 18); wetLabel.setBounds (output.getRight() - 68, output.getBottom() - 48, 48, 18); btnBypass.setBounds (output.getX() + 170, output.getY() + 130, output.getWidth() - 195, 42);
}
