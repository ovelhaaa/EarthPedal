#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int defaultEditorWidth = 900;
constexpr int defaultEditorHeight = 620;
constexpr int minEditorWidth = defaultEditorWidth;
constexpr int minEditorHeight = defaultEditorHeight;
constexpr int maxEditorWidth = 1400;
constexpr int maxEditorHeight = 980;

void styleCaption (juce::Label& label, const juce::String& caption, float size)
{
    label.setText (caption.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (ApolloTheme::labelFont (size));
    label.setColour (juce::Label::textColourId, ApolloTheme::textOnPanel);
    label.getProperties().set (ApolloTheme::captionProperty, true);
}

void styleValue (juce::Label& label, float size)
{
    label.setText ({}, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (ApolloTheme::valueFont (size));
    label.setColour (juce::Label::textColourId, ApolloTheme::orange);
    label.getProperties().set (ApolloTheme::displayProperty, true);
}

void styleKnob (juce::Slider& slider, juce::Label& caption, juce::Label& value,
                const juce::String& name, float captionSize = 10.0f, float valueSize = 10.0f)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setWantsKeyboardFocus (true);
    slider.setTitle (name);
    slider.setDescription (name + " control. Use arrow keys for small changes; double click resets to the parameter default.");
    styleCaption (caption, name, captionSize);
    styleValue (value, valueSize);
}

void styleRockToggle (juce::ToggleButton& button, const juce::String& name, const juce::String& tooltip,
                      const juce::String& description)
{
    button.setButtonText ({});
    button.setTooltip (tooltip);
    button.setTitle (name);
    button.setDescription (description);
    button.setWantsKeyboardFocus (true);
    button.getProperties().set (ApolloTheme::styleProperty, (int) ApolloTheme::ButtonStyle::Rocker);
}
}

// MomentaryGateButton is declared in the global namespace. Keep these
// out-of-class definitions at global scope too; MSVC rejects them with C2888
// if they are placed inside the anonymous namespace used for file helpers.
MomentaryGateButton::MomentaryGateButton()
{
    setClickingTogglesState (false);
}

MomentaryGateButton::~MomentaryGateButton() { releaseLocalGate(); }

void MomentaryGateButton::releaseLocalGate()
{
    // Do not cancel an automated ON when this editor owns no gesture.
    if (! localMouseGestureActive && ! localKeyGestureActive) return;
    localMouseGestureActive = localKeyGestureActive = false;
    setState (juce::Button::buttonNormal);
    // Notify even if automation already changed the visual state to OFF.
    // This guarantees the local release reaches APVTS in that race.
    if (getToggleState()) setToggleState (false, juce::sendNotificationSync);
    else juce::ToggleButton::internalClickCallback (juce::ModifierKeys());
}
void MomentaryGateButton::mouseDown (const juce::MouseEvent& event)
{
    if (! isEnabled() || ! event.mods.isLeftButtonDown()) return;
    juce::ToggleButton::mouseDown (event);
    localMouseGestureActive = true;
    setToggleState (true, juce::sendNotificationSync);
}
void MomentaryGateButton::mouseUp (const juce::MouseEvent& event)
{
    releaseLocalGate();
    juce::ToggleButton::mouseUp (event);
}
bool MomentaryGateButton::keyPressed (const juce::KeyPress& key)
{
    if (isEnabled() && (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey))
    {
        localKeyGestureActive = true;
        setState (juce::Button::buttonDown);
        setToggleState (true, juce::sendNotificationSync);
        return true;
    }
    return false;
}
bool MomentaryGateButton::keyStateChanged (bool keyIsDown)
{
    juce::ignoreUnused (keyIsDown);
    if (localKeyGestureActive)
    {
        if (! juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::spaceKey)
            && ! juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::returnKey))
            releaseLocalGate();
        return true;
    }
    return false;
}
void MomentaryGateButton::focusLost (FocusChangeType cause)
{
    releaseLocalGate();
    juce::ToggleButton::focusLost (cause);
}
void MomentaryGateButton::visibilityChanged()
{
    juce::ToggleButton::visibilityChanged();
    if (! isShowing()) releaseLocalGate();
}
void MomentaryGateButton::enablementChanged()
{
    juce::ToggleButton::enablementChanged();
    if (! isEnabled()) releaseLocalGate();
}
void MomentaryGateButton::parentHierarchyChanged()
{
    juce::ToggleButton::parentHierarchyChanged();
    if (! isShowing()) releaseLocalGate();
}

//==============================================================================
ApolloAudioProcessorEditor::ApolloAudioProcessorEditor (ApolloAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (defaultEditorWidth, defaultEditorHeight);
    setResizable (true, true);
    setResizeLimits (minEditorWidth, minEditorHeight, maxEditorWidth, maxEditorHeight);
    if (auto* constrainer = getConstrainer())
    {
        constrainer->setFixedAspectRatio ((double) defaultEditorWidth / (double) defaultEditorHeight);
        constrainer->checkComponentBounds (this);
    }
    setWantsKeyboardFocus (true);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setTitle ("Apollo plugin editor");
    setDescription ("Apollo plate reverb editor. Keyboard focus follows Space, Output, Octave and Perform.");
    setLookAndFeel (&customLookAndFeel);

    // Decorative back plates go in first so they always sit behind the controls.
    for (auto* panel : { &reverbPanel, &outputPanel, &octavePanel, &performancePanel })
        addAndMakeVisible (*panel);


    auto addKnob = [this] (juce::Slider& knob, juce::Label& caption, juce::Label& value, const char* id,
                           const juce::String& name,
                           std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment)
    {
        addAndMakeVisible (knob);
        addAndMakeVisible (caption);
        addAndMakeVisible (value);
        styleKnob (knob, caption, value, name);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id, knob);
        auto* parameter = audioProcessor.apvts.getParameter (id);
        knob.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    };

    addKnob (knobPredelay, lblPredelay, valuePredelay, "predelay", "Pre-delay", attachPredelay);
    addKnob (knobDecay, lblDecay, valueDecay, "decay", "Decay", attachDecay);
    addKnob (knobDamp, lblDamp, valueDamp, "damp", "Tone", attachDamp);
    addKnob (knobModSpeed, lblModSpeed, valueModSpeed, "modspeed", "Mod Rate", attachModSpeed);
    addKnob (knobModDepth, lblModDepth, valueModDepth, "moddepth", "Mod Depth", attachModDepth);
    addKnob (knobEq1, lblEq1, valueEq1, "eq1_gain", "Presence", attachEq1);
    addKnob (knobEq2, lblEq2, valueEq2, "eq2_gain", "Body", attachEq2);
    knobPredelay.setTooltip ("Atrasa a entrada do reverb.");
    knobDecay.setTooltip ("Define quanto tempo o reverb sustenta.");
    knobDamp.setTooltip ("High Cut a esquerda; Low Cut a direita.");
    knobModSpeed.setTooltip ("Modulation speed multiplier, 0.30x to 15.30x, applied to the four tank LFO rates.");
    knobModDepth.setTooltip ("Define a intensidade do movimento.");
    knobEq1.setTooltip ("Ajusta o shelf alto da ramificacao de oitava.");
    knobEq2.setTooltip ("Ajusta o shelf baixo da ramificacao de oitava.");

    knobDamp.getProperties().set (ApolloTheme::bipolarProperty, true);
    addAndMakeVisible (lblToneHigh);
    addAndMakeVisible (lblToneLow);
    addAndMakeVisible (lblToneFlat);
    styleCaption (lblToneHigh, "HIGH CUT", 7.5f);
    styleCaption (lblToneFlat, "FLAT", 7.5f);
    styleCaption (lblToneLow, "LOW CUT", 7.5f);
    lblToneHigh.setColour (juce::Label::textColourId, ApolloTheme::textOnPanelDim);
    lblToneLow.setColour (juce::Label::textColourId, ApolloTheme::textOnPanelDim);

    auto addChoice = [this] (ApolloSelector& combo, juce::Label& caption, const char* id, const juce::String& name,
                             const juce::StringArray& choices,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attachment)
    {
        styleCaption (caption, name, 10.0f);
        addAndMakeVisible (caption);
        addAndMakeVisible (combo);
        combo.addItemList (choices, 1);
        auto* parameter = audioProcessor.apvts.getParameter (id);
        combo.setDefaultIndex (juce::roundToInt (parameter->convertFrom0to1 (parameter->getDefaultValue())));
        combo.setWantsKeyboardFocus (true);
        combo.setTitle (name);
        combo.setDescription (name + " selector. Use arrow keys to change the selected choice.");
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, id, combo);
    };

    addChoice (comboTimeScale, lblTimeScale, "time_scale", "Size", { "SMALL", "MEDIUM", "LARGE" }, attachTimeScale);
    addChoice (comboEffectMode, lblEffectMode, "effect_mode", "Mode", { "OFF", "UP", "DOWN", "UP+DOWN" }, attachEffectMode);
    addChoice (comboFootswitchMode, lblFootswitchMode, "footswitch_mode", "Action", { "FREEZE", "DRIVE", "OCTAVE" }, attachFootswitchMode);
    for (auto* combo : { &comboTimeScale, &comboEffectMode, &comboFootswitchMode })
        combo->getProperties().set (ApolloTheme::pushBankProperty, true);
    comboTimeScale.setTooltip ("Escolhe o tamanho do espaco.");
    comboEffectMode.setTooltip ("Escolhe Off, Up, Down ou Up + Down para a ramificacao de oitava.");
    comboFootswitchMode.setTooltip ("Select Freeze, Drive or Octave; hold Perform or automate the momentary gate.");

    addAndMakeVisible (lblInputDiffusion);
    styleCaption (lblInputDiffusion, "Diffusion", 9.0f);
    addAndMakeVisible (btnInputDiffusion);
    styleRockToggle (btnInputDiffusion, "Input Diffusion", "Espalha o sinal antes do plate.",
                     "Toggles input diffusion on or off.");
    attachInputDiffusion = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "input_diffusion", btnInputDiffusion);

    addAndMakeVisible (lblOctaveDryMix);
    styleCaption (lblOctaveDryMix, "Reverb Feed", 9.0f);
    addAndMakeVisible (btnOctaveDryMix);
    styleRockToggle (btnOctaveDryMix, "Reverb Feed", "OCT: octave only. OCT + DRY: adds 0.5 times dry to the octave branch before reverb.",
                     "Off selects OCT. On selects OCT + DRY, including 0.5 times dry before reverb.");
    btnOctaveDryMix.getProperties().set (ApolloTheme::feedProperty, true);
    attachOctaveDryMix = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "octave_dry_mix", btnOctaveDryMix);

    addAndMakeVisible (btnMomentaryEffect);
    btnMomentaryEffect.setButtonText ("PERFORM");
    btnMomentaryEffect.setTitle ("Perform");
    btnMomentaryEffect.setWantsKeyboardFocus (true);
    btnMomentaryEffect.setTooltip ("Hold mouse, Space or Enter to perform the selected action. Release to end.");
    btnMomentaryEffect.getProperties().set (ApolloTheme::styleProperty, (int) ApolloTheme::ButtonStyle::Momentary);
    attachMomentaryEffect = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "momentary_effect", btnMomentaryEffect);

    addAndMakeVisible (btnBypass);
    btnBypass.setButtonText ({});
    btnBypass.setTooltip ("Bypass interno: passa o sinal direto; nao e o bypass do host.");
    btnBypass.setTitle ("Bypass");
    btnBypass.setDescription ("Internal bypass is off. Apollo is processing.");
    btnBypass.setWantsKeyboardFocus (true);
    btnBypass.getProperties().set (ApolloTheme::styleProperty, (int) ApolloTheme::ButtonStyle::Bypass);
    attachBypass = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "bypass", btnBypass);

    addAndMakeVisible (faderMix);
    faderMix.setSliderStyle (juce::Slider::LinearVertical);
    faderMix.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    faderMix.setWantsKeyboardFocus (true);
    faderMix.setTooltip ("Equilibra sinal direto e reverb.");
    faderMix.setTitle ("Mix");
    faderMix.setDescription ("Dry wet mix control. Use arrow keys for small changes; double click resets to the parameter default.");
    faderMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "mix", faderMix);
    auto* mixParameter = audioProcessor.apvts.getParameter ("mix");
    faderMix.setDoubleClickReturnValue (true, mixParameter->convertFrom0to1 (mixParameter->getDefaultValue()));

    addAndMakeVisible (lblDry);
    styleCaption (lblDry, "DRY", 10.0f);
    addAndMakeVisible (lblWet);
    styleCaption (lblWet, "WET", 10.0f);
    int focusOrder = 1;
    for (auto* component : { static_cast<juce::Component*> (&knobPredelay), static_cast<juce::Component*> (&knobDecay),
                             static_cast<juce::Component*> (&knobDamp), static_cast<juce::Component*> (&knobModSpeed),
                             static_cast<juce::Component*> (&knobModDepth), static_cast<juce::Component*> (&comboTimeScale),
                             static_cast<juce::Component*> (&btnInputDiffusion), static_cast<juce::Component*> (&faderMix),
                             static_cast<juce::Component*> (&comboEffectMode), static_cast<juce::Component*> (&btnOctaveDryMix),
                             static_cast<juce::Component*> (&knobEq1), static_cast<juce::Component*> (&knobEq2),
                             static_cast<juce::Component*> (&comboFootswitchMode), static_cast<juce::Component*> (&btnMomentaryEffect),
                             static_cast<juce::Component*> (&btnBypass) })
        component->setExplicitFocusOrder (focusOrder++);

    resized(); // layout/font scale after all controls and attachments exist
    updateStatePresentation();
    updateValueLabels();
    startTimerHz (12);
}

ApolloAudioProcessorEditor::~ApolloAudioProcessorEditor()
{
    stopTimer();
    btnMomentaryEffect.releaseLocalGate(); // before its attachment is destroyed
    setLookAndFeel (nullptr);
}

//==============================================================================
float ApolloAudioProcessorEditor::getDesignScale() const
{
    return juce::jmin ((float) getWidth() / (float) ApolloTheme::designWidth,
                       (float) getHeight() / (float) ApolloTheme::designHeight);
}

juce::Rectangle<float> ApolloAudioProcessorEditor::scaled (float x, float y, float w, float h) const
{
    const float s = getDesignScale();
    const float ox = ((float) getWidth() - (float) ApolloTheme::designWidth * s) * 0.5f;
    const float oy = ((float) getHeight() - (float) ApolloTheme::designHeight * s) * 0.5f;
    return { ox + x * s, oy + y * s, w * s, h * s };
}

void ApolloAudioProcessorEditor::applyFontScale (float s)
{
    const auto caption = [s] (juce::Label& l) { l.setFont (ApolloTheme::labelFont (10.0f * s)); };
    const auto value   = [s] (juce::Label& l) { l.setFont (ApolloTheme::valueFont (10.0f * s)); };

    caption (lblPredelay); caption (lblDecay); caption (lblDamp);
    caption (lblModSpeed); caption (lblModDepth); caption (lblEq1); caption (lblEq2);
    caption (lblTimeScale); caption (lblEffectMode); caption (lblFootswitchMode);
    caption (lblInputDiffusion); caption (lblOctaveDryMix);
    caption (lblDry); caption (lblWet);

    value (valuePredelay); value (valueDecay); value (valueDamp);
    value (valueModSpeed); value (valueModDepth); value (valueEq1); value (valueEq2);

    lblToneHigh.setFont (ApolloTheme::labelFont (7.5f * s));
    lblToneLow.setFont (ApolloTheme::labelFont (7.5f * s));
    lblToneFlat.setFont (ApolloTheme::labelFont (7.5f * s));
}

void ApolloAudioProcessorEditor::timerCallback()
{
    updateStatePresentation();
    updateValueLabels();
}

//==============================================================================
void ApolloAudioProcessorEditor::updateStatePresentation()
{
    const auto mode = (int) std::round (audioProcessor.apvts.getRawParameterValue ("effect_mode")->load());
    const auto action = (int) std::round (audioProcessor.apvts.getRawParameterValue ("footswitch_mode")->load());
    const bool perform = audioProcessor.apvts.getRawParameterValue ("momentary_effect")->load() > 0.5f;
    const bool bypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    const bool octaveOff = mode == 0 || (action == 2 && ! perform);
    reverbPanel.setDimmed (bypassed);
    octavePanel.setDimmed (octaveOff || bypassed);
    for (auto* component : { static_cast<juce::Component*> (&knobEq1), static_cast<juce::Component*> (&knobEq2),
                             static_cast<juce::Component*> (&valueEq1), static_cast<juce::Component*> (&valueEq2),
                             static_cast<juce::Component*> (&btnOctaveDryMix) })
    {
        component->getProperties().set (ApolloTheme::dimProperty, octaveOff || bypassed);
        component->repaint();
    }
    btnBypass.setDescription (bypassed ? "Bypassed. Smoothed dry path; controls remain editable." : "Active. Apollo is processing.");
    btnMomentaryEffect.setDescription (juce::String ("Momentary gate for ")
        + (action == 0 ? "Freeze" : action == 1 ? "Drive" : "Octave")
        + (perform ? ". Gate ON. " : ". Gate OFF. ")
        + (bypassed ? "Audio is bypassed. " : "")
        + (action == 2 && mode == 0 ? "Select an octave mode to hear this action. " : "")
        + "Hold mouse, Space or Enter; release or leave focus to end. Host automation is supported.");
    btnOctaveDryMix.setDescription (juce::String (octaveOff ? "Octave branch inactive; control remains editable. " : "")
        + (btnOctaveDryMix.getToggleState() ? "OCT + DRY: adds 0.5 times dry before reverb." : "OCT: octave only before reverb."));
    knobEq1.setDescription (juce::String (octaveOff ? "Octave branch inactive. " : "") + "Presence: octave high shelf at 140 Hz, "
                           + juce::String (knobEq1.getValue(), 1) + " dB.");
    knobEq2.setDescription (juce::String (octaveOff ? "Octave branch inactive. " : "") + "Body: octave low shelf at 160 Hz, "
                           + juce::String (knobEq2.getValue(), 1) + " dB.");
    knobEq1.setTooltip (knobEq1.getDescription());
    knobEq2.setTooltip (knobEq2.getDescription());
}

void ApolloAudioProcessorEditor::updateValueLabels()
{
    auto percent = [] (juce::Slider& s) { return juce::String (juce::roundToInt (s.getValue() * 100.0)) + "%"; };

    valuePredelay.setText (juce::String (juce::roundToInt (knobPredelay.getValue() * 1000.0)) + " ms", juce::dontSendNotification);
    valueDecay.setText (percent (knobDecay), juce::dontSendNotification);
    const double tone = knobDamp.getValue();
    const auto cut = juce::String (juce::roundToInt (std::abs (tone - 0.5) * 200.0)) + "%";
    const auto toneText = tone == 0.5 ? juce::String ("FLAT") : juce::String (tone < 0.5 ? "HI " : "LO ") + cut;
    valueDamp.setText (toneText, juce::dontSendNotification);
    knobDamp.setDescription ("Tone: " + toneText + ". Left: high cut; centre: neutral; right: low cut. Double click resets to Flat.");
    knobDamp.setTooltip (knobDamp.getDescription());
    valueModSpeed.setText (juce::String (0.3 + 15.0 * knobModSpeed.getValue(), 2) + "x", juce::dontSendNotification);
    valueModDepth.setText (percent (knobModDepth), juce::dontSendNotification);
    valueEq1.setText (juce::String (knobEq1.getValue(), 1) + " dB", juce::dontSendNotification);
    valueEq2.setText (juce::String (knobEq2.getValue(), 1) + " dB", juce::dontSendNotification);
}

//==============================================================================
void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ApolloTheme::chassisMid);
    g.setColour (ApolloTheme::chassisEdge);
    g.drawRect (getLocalBounds(), 1);
    const float s = getDesignScale();
    g.setColour (ApolloTheme::textOnChassis);
    g.setFont (ApolloTheme::headingFont (38.0f * s));
    g.drawText ("APOLLO", scaled (36, 20, 400, 46), juce::Justification::centredLeft, false);
    g.setColour (ApolloTheme::textOnChassisSoft);
    g.setFont (ApolloTheme::labelFont (10.0f * s));
    g.drawText ("STEREO SPACE PROCESSOR", scaled (38, 68, 400, 18), juce::Justification::centredLeft, false);

    g.setColour (ApolloTheme::graphite);
    g.fillRoundedRectangle (scaled (24, 112, 852, 484), 3.0f * s);
    g.setColour (ApolloTheme::graphiteEdge.withAlpha (0.65f));
    g.drawLine (scaled (24, 392, 0, 0).getX(), scaled (24, 392, 0, 0).getY(),
                scaled (876, 392, 0, 0).getX(), scaled (876, 392, 0, 0).getY(), s);
    g.drawLine (scaled (636, 112, 0, 0).getX(), scaled (636, 112, 0, 0).getY(),
                scaled (636, 596, 0, 0).getX(), scaled (636, 596, 0, 0).getY(), s);
}

void ApolloAudioProcessorEditor::resized()
{
    if (auto* constrainer = getConstrainer())
    {
        const auto before = getBounds();
        constrainer->checkComponentBounds (this);
        if (getBounds() != before) return;
    }
    const float s = getDesignScale();
    customLookAndFeel.setUiScale (s);
    applyFontScale (s);
    const auto R = [this] (float x, float y, float w, float h) { return scaled (x, y, w, h).toNearestInt(); };
    reverbPanel.setBounds (R (24, 112, 612, 280));
    outputPanel.setBounds (R (636, 112, 240, 280));
    octavePanel.setBounds (R (24, 392, 612, 204));
    performancePanel.setBounds (R (636, 392, 240, 204));
    btnBypass.setBounds (R (710, 40, 166, 38));

    const auto knob = [&R] (juce::Slider& control, juce::Label& caption, juce::Label& value,
                            float cx, float y, float diameter)
    {
        caption.setBounds (R (cx - 58, y - 22, 116, 18));
        control.setBounds (R (cx - diameter / 2, y, diameter, diameter));
        value.setBounds (R (cx - 58, y + diameter + 2, 116, 20));
    };
    knob (knobPredelay, lblPredelay, valuePredelay, 112, 182, 92);
    knob (knobDecay, lblDecay, valueDecay, 258, 170, 116);
    knob (knobDamp, lblDamp, valueDamp, 404, 182, 92);
    lblToneHigh.setBounds (R (324, 307, 64, 12));
    lblToneFlat.setBounds (R (388, 307, 32, 12));
    lblToneLow.setBounds (R (420, 307, 64, 12));
    // Smaller secondary modulation controls share a disciplined right column.
    knob (knobModSpeed, lblModSpeed, valueModSpeed, 560, 162, 68);
    knob (knobModDepth, lblModDepth, valueModDepth, 560, 286, 68);

    lblTimeScale.setBounds (R (64, 340, 40, 28));
    comboTimeScale.setBounds (R (112, 340, 248, 28));
    lblInputDiffusion.setBounds (R (374, 340, 86, 28));
    btnInputDiffusion.setBounds (R (464, 340, 38, 28));

    lblWet.setBounds (R (698, 153, 116, 16));
    faderMix.setBounds (R (698, 179, 116, 154));
    lblDry.setBounds (R (698, 352, 116, 16));

    lblEffectMode.setBounds (R (48, 442, 264, 16));
    comboEffectMode.setBounds (R (48, 466, 264, 34));
    lblOctaveDryMix.setBounds (R (48, 517, 264, 16));
    btnOctaveDryMix.setBounds (R (48, 541, 264, 32));
    knob (knobEq1, lblEq1, valueEq1, 416, 464, 86);
    knob (knobEq2, lblEq2, valueEq2, 556, 464, 86);

    // The section title already names the action; no duplicate caption.
    lblFootswitchMode.setVisible (false);
    comboFootswitchMode.setBounds (R (660, 444, 192, 32));
    btnMomentaryEffect.setBounds (R (660, 497, 192, 76));
}
