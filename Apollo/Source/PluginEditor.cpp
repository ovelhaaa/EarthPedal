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

void MomentaryGateButton::mouseDown (const juce::MouseEvent& event)
{
    juce::ToggleButton::mouseDown (event);
    setToggleState (true, juce::sendNotification);
}

void MomentaryGateButton::mouseUp (const juce::MouseEvent& event)
{
    setToggleState (false, juce::sendNotification);
    juce::ToggleButton::mouseUp (event);
}

bool MomentaryGateButton::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        localKeyGestureActive = true;
        setState (juce::Button::buttonDown);
        setToggleState (true, juce::sendNotification);
        return true;
    }

    return juce::ToggleButton::keyPressed (key);
}

bool MomentaryGateButton::keyStateChanged (bool keyIsDown)
{
    if (localKeyGestureActive && ! keyIsDown)
    {
        const bool triggerKeyIsStillDown = juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::spaceKey)
                                           || juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::returnKey);

        if (! triggerKeyIsStillDown)
        {
            localKeyGestureActive = false;
            setState (juce::Button::buttonNormal);
            setToggleState (false, juce::sendNotification);
        }

        return true;
    }

    return juce::ToggleButton::keyStateChanged (keyIsDown);
}

void MomentaryGateButton::focusLost (FocusChangeType cause)
{
    juce::ignoreUnused (cause);
    if (localKeyGestureActive)
    {
        localKeyGestureActive = false;
        setState (juce::Button::buttonNormal);
        setToggleState (false, juce::sendNotification);
    }
    juce::ToggleButton::focusLost (cause);
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
    setDescription ("Apollo plate reverb editor. Keyboard focus follows Reverb, Octave, Performance and Output.");
    setLookAndFeel (&customLookAndFeel);

    // Decorative back plates go in first so they always sit behind the controls.
    for (auto* panel : { &reverbPanel, &outputPanel, &octavePanel, &performancePanel })
        addAndMakeVisible (*panel);

    addAndMakeVisible (lfoAnnunciator);
    lfoAnnunciator.setText ("MOD LFO");
    lfoAnnunciator.setTitle ("Modulation activity");
    lfoAnnunciator.setDescription ("Pilot lamp reflecting the modulation depth setting.");

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
    addKnob (knobEq1, lblEq1, valueEq1, "eq1_gain", "Oct High Shelf", attachEq1);
    addKnob (knobEq2, lblEq2, valueEq2, "eq2_gain", "Oct Low Shelf", attachEq2);
    knobPredelay.setTooltip ("Atrasa a entrada do reverb.");
    knobDecay.setTooltip ("Define quanto tempo o reverb sustenta.");
    knobDamp.setTooltip ("High Cut a esquerda; Low Cut a direita.");
    knobModSpeed.setTooltip ("Define a velocidade do movimento.");
    knobModDepth.setTooltip ("Define a intensidade do movimento.");
    knobEq1.setTooltip ("Ajusta o shelf alto da ramificacao de oitava.");
    knobEq2.setTooltip ("Ajusta o shelf baixo da ramificacao de oitava.");

    knobDamp.getProperties().set (ApolloTheme::bipolarProperty, true);
    addAndMakeVisible (lblToneHigh);
    addAndMakeVisible (lblToneLow);
    styleCaption (lblToneHigh, "HI CUT", 7.5f);
    styleCaption (lblToneLow, "LO CUT", 7.5f);
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
        combo.setWantsKeyboardFocus (true);
        combo.setTitle (name);
        combo.setDescription (name + " selector. Use arrow keys to change the selected choice.");
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, id, combo);
    };

    addChoice (comboTimeScale, lblTimeScale, "time_scale", "Size", { "SMALL", "MEDIUM", "LARGE" }, attachTimeScale);
    addChoice (comboEffectMode, lblEffectMode, "effect_mode", "Octave Mode", { "OFF", "UP", "DOWN", "UP+DOWN" }, attachEffectMode);
    addChoice (comboFootswitchMode, lblFootswitchMode, "footswitch_mode", "Perform Action", { "FREEZE", "OVERDRIVE", "OCTAVE" }, attachFootswitchMode);
    for (auto* combo : { &comboTimeScale, &comboEffectMode, &comboFootswitchMode })
        combo->getProperties().set (ApolloTheme::pushBankProperty, true);
    comboTimeScale.getProperties().set (ApolloTheme::verticalProperty, true);
    comboTimeScale.setTooltip ("Escolhe o tamanho do espaco.");
    comboEffectMode.setTooltip ("Escolhe Off, Up, Down ou Up + Down para a ramificacao de oitava.");
    comboFootswitchMode.setTooltip ("Escolhe a acao de performance; o disparo momentary vem de MIDI/automacao.");

    addAndMakeVisible (lblInputDiffusion);
    styleCaption (lblInputDiffusion, "Input Diffusion", 9.0f);
    addAndMakeVisible (btnInputDiffusion);
    styleRockToggle (btnInputDiffusion, "Input Diffusion", "Espalha o sinal antes do plate.",
                     "Toggles input diffusion on or off.");
    attachInputDiffusion = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "input_diffusion", btnInputDiffusion);

    addAndMakeVisible (lblOctaveDryMix);
    styleCaption (lblOctaveDryMix, "Dry Routing", 9.0f);
    addAndMakeVisible (btnOctaveDryMix);
    styleRockToggle (btnOctaveDryMix, "Octave Dry Routing", "Roteamento dry da ramificacao de oitava; validacao pendente.",
                     "Pending dry routing control for the octave branch. Semantic validation pending.");
    attachOctaveDryMix = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "octave_dry_mix", btnOctaveDryMix);

    // momentary_effect is triggered by MIDI/automation. The control is kept
    // attached (ButtonAttachment) but is not shown and does not take focus.
    btnMomentaryEffect.setButtonText ("PERFORM");
    btnMomentaryEffect.setTitle ("Perform / Gate");
    btnMomentaryEffect.setDescription ("Momentary performance gate, triggered by MIDI or host automation.");
    btnMomentaryEffect.setVisible (false);
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
    addAndMakeVisible (lblPerformNote);
    lblPerformNote.setText ("TRIGGER: MIDI / AUTOMATION", juce::dontSendNotification);
    lblPerformNote.setJustificationType (juce::Justification::centred);
    lblPerformNote.setFont (ApolloTheme::labelFont (7.5f));
    lblPerformNote.setColour (juce::Label::textColourId, ApolloTheme::textOnPanelDim);

    int focusOrder = 1;
    for (auto* component : { static_cast<juce::Component*> (&comboTimeScale), static_cast<juce::Component*> (&knobPredelay),
                             static_cast<juce::Component*> (&knobDecay), static_cast<juce::Component*> (&knobDamp),
                             static_cast<juce::Component*> (&knobModSpeed), static_cast<juce::Component*> (&knobModDepth),
                             static_cast<juce::Component*> (&btnInputDiffusion), static_cast<juce::Component*> (&comboEffectMode),
                             static_cast<juce::Component*> (&knobEq1), static_cast<juce::Component*> (&knobEq2),
                             static_cast<juce::Component*> (&btnOctaveDryMix), static_cast<juce::Component*> (&comboFootswitchMode),
                             static_cast<juce::Component*> (&faderMix),
                             static_cast<juce::Component*> (&btnBypass) })
        component->setExplicitFocusOrder (focusOrder++);

    updateStatePresentation();
    updateValueLabels();
    startTimerHz (12);
}

ApolloAudioProcessorEditor::~ApolloAudioProcessorEditor()
{
    stopTimer();
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
    lblPerformNote.setFont (ApolloTheme::labelFont (7.5f * s));
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
    const bool octaveOff = mode == 0;

    // Module back-lighting: an off module loses its lamp and backlight instead
    // of being made transparent. Controls stay physically present and editable.
    reverbPanel.setLampActive (! bypassed);
    reverbPanel.setLampColour (ApolloTheme::orange);
    reverbPanel.setDimmed (bypassed);

    outputPanel.setLampActive (! bypassed);
    outputPanel.setLampColour (ApolloTheme::orange);

    octavePanel.setLampActive (! octaveOff && ! bypassed);
    octavePanel.setLampColour (ApolloTheme::orange);
    octavePanel.setDimmed (octaveOff || bypassed);

    performancePanel.setLampColour (ApolloTheme::orange);

    const bool dimOctaveControls = octaveOff || bypassed;
    for (auto* component : { static_cast<juce::Component*> (&knobEq1), static_cast<juce::Component*> (&knobEq2),
                             static_cast<juce::Component*> (&lblEq1), static_cast<juce::Component*> (&lblEq2),
                             static_cast<juce::Component*> (&valueEq1), static_cast<juce::Component*> (&valueEq2),
                             static_cast<juce::Component*> (&btnOctaveDryMix), static_cast<juce::Component*> (&lblOctaveDryMix) })
    {
        component->getProperties().set (ApolloTheme::dimProperty, dimOctaveControls);
        component->repaint();
    }

    // Performance state is shown by the module pilot lamp (no text strip and
    // no large momentary button; momentary_effect is driven by MIDI/automation).
    performancePanel.setLampActive (perform);
    performancePanel.setLampColour ((perform && action == 2 && octaveOff) ? ApolloTheme::red : ApolloTheme::orange);

    const float modDepth = audioProcessor.apvts.getRawParameterValue ("moddepth")->load();
    lfoAnnunciator.setLamp (modDepth > 0.005f && ! bypassed, ApolloTheme::cyan);
    lfoAnnunciator.setDimmed (bypassed);

    btnBypass.setDescription (juce::String ("Internal bypass is ") + (bypassed ? "on. Audio follows the dry path; controls remain editable." : "off. Apollo is processing."));
    btnMomentaryEffect.setDescription (juce::String ("Momentary performance gate for ") + (action == 0 ? "Freeze" : action == 1 ? "Overdrive" : "Octave Perform") + ". Triggered by MIDI or host automation; the UI only reflects the state.");
    btnOctaveDryMix.setDescription (juce::String (octaveOff ? "Inactive in current Octave Off mode, but still editable by keyboard, preset and automation. " : "") + "Pending dry routing control for the octave branch.");
    knobEq1.setDescription (juce::String (octaveOff ? "Inactive in current Octave Off mode, but still editable. " : "") + "Octave high shelf gain in dB.");
    knobEq2.setDescription (juce::String (octaveOff ? "Inactive in current Octave Off mode, but still editable. " : "") + "Octave low shelf gain in dB.");
}

void ApolloAudioProcessorEditor::updateValueLabels()
{
    auto percent = [] (juce::Slider& s) { return juce::String (juce::roundToInt (s.getValue() * 100.0)) + "%"; };

    valuePredelay.setText (juce::String (juce::roundToInt (knobPredelay.getValue() * 1000.0)) + " ms", juce::dontSendNotification);
    valueDecay.setText (percent (knobDecay), juce::dontSendNotification);
    valueDamp.setText (percent (knobDamp), juce::dontSendNotification);
    valueModSpeed.setText (percent (knobModSpeed), juce::dontSendNotification);
    valueModDepth.setText (percent (knobModDepth), juce::dontSendNotification);
    valueEq1.setText (juce::String (knobEq1.getValue(), 1) + " dB", juce::dontSendNotification);
    valueEq2.setText (juce::String (knobEq2.getValue(), 1) + " dB", juce::dontSendNotification);
}

//==============================================================================
void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    ApolloTheme::drawBrushedMetal (g, full, ApolloTheme::chassisMid);

    g.setColour (ApolloTheme::chassisDark);
    g.drawRect (getLocalBounds(), 1);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawLine (0.5f, 0.5f, (float) getWidth() - 0.5f, 0.5f, 1.0f);

    const float s = getDesignScale();

    auto headerPlate = scaled (14.0f, 12.0f, 872.0f, 86.0f);
    ApolloTheme::drawRaisedPlate (g, headerPlate, 5.0f, false);

    auto modelPlate = scaled (470.0f, 28.0f, 190.0f, 56.0f);
    ApolloTheme::drawInsetWell (g, modelPlate, 3.0f, 0.6f);
    {
        auto area = modelPlate.reduced (9.0f, 6.0f);
        auto line1 = area.removeFromTop (area.getHeight() * 0.5f);
        auto line2 = area;

        g.setColour (ApolloTheme::orange);
        g.setFont (ApolloTheme::valueFont (9.5f * s));
        g.drawText ("MOD. APOLLO-RA", line1, juce::Justification::centredLeft, false);

        g.setColour (ApolloTheme::textOnPanelDim);
        g.setFont (ApolloTheme::valueFont (8.0f * s));
        g.drawText ("STEREO SPACE PROCESSOR", line2, juce::Justification::centredLeft, false);
    }

    ApolloTheme::drawEngravedText (g, "APOLLO", scaled (36.0f, 16.0f, 340.0f, 54.0f),
                                   ApolloTheme::headingFont (34.0f * s), juce::Justification::centredLeft,
                                   ApolloTheme::textOnChassis, ApolloTheme::engraveHighlight);

    ApolloTheme::drawEngravedText (g, "STEREO SPACE PROCESSOR", scaled (41.0f, 58.0f, 320.0f, 18.0f),
                                   ApolloTheme::labelFont (11.0f * s), juce::Justification::centredLeft,
                                   ApolloTheme::textOnChassisSoft, ApolloTheme::engraveHighlight);

    ApolloTheme::drawEngravedText (g, "PLATE + OCTAVE SYSTEM", scaled (41.0f, 73.0f, 320.0f, 16.0f),
                                   ApolloTheme::labelFont (9.0f * s), juce::Justification::centredLeft,
                                   ApolloTheme::textOnChassisSoft, ApolloTheme::engraveHighlight);

    const float screwRadius = juce::jmax (3.0f, 4.0f * s);
    ApolloTheme::drawScrew (g, scaled (22.0f, 20.0f, 0.0f, 0.0f).getCentre(), screwRadius);
    ApolloTheme::drawScrew (g, scaled (878.0f, 20.0f, 0.0f, 0.0f).getCentre(), screwRadius);
}

void ApolloAudioProcessorEditor::resized()
{
    if (auto* constrainer = getConstrainer())
    {
        const auto boundsBeforeConstraint = getBounds();
        constrainer->checkComponentBounds (this);

        if (getBounds() != boundsBeforeConstraint)
            return;
    }

    const float s = getDesignScale();
    customLookAndFeel.setUiScale (s);
    applyFontScale (s);

    auto R = [this] (float x, float y, float w, float h) { return scaled (x, y, w, h).toNearestInt(); };

    reverbPanel.setBounds (R (14.0f, 108.0f, 580.0f, 300.0f));
    outputPanel.setBounds (R (604.0f, 108.0f, 282.0f, 360.0f));
    octavePanel.setBounds (R (14.0f, 418.0f, 580.0f, 198.0f));
    performancePanel.setBounds (R (604.0f, 478.0f, 282.0f, 138.0f));

    // Header: the internal bypass lives on the top plate, replacing the old
    // SYSTEM/ACTIVE indicator.
    btnBypass.setBounds (R (672.0f, 28.0f, 198.0f, 58.0f));

    // REVERB
    lblTimeScale.setBounds (R (444.0f, 146.0f, 132.0f, 16.0f));
    comboTimeScale.setBounds (R (444.0f, 164.0f, 132.0f, 110.0f));
    lblInputDiffusion.setBounds (R (444.0f, 282.0f, 132.0f, 16.0f));
    btnInputDiffusion.setBounds (R (444.0f, 300.0f, 132.0f, 36.0f));
    lfoAnnunciator.setBounds (R (300.0f, 314.0f, 130.0f, 56.0f));

    knobPredelay.setBounds (R (52.0f, 162.0f, 90.0f, 90.0f));
    lblPredelay.setBounds (R (30.0f, 144.0f, 133.0f, 16.0f));
    valuePredelay.setBounds (R (30.0f, 254.0f, 133.0f, 16.0f));

    knobDecay.setBounds (R (185.0f, 162.0f, 90.0f, 90.0f));
    lblDecay.setBounds (R (163.0f, 144.0f, 133.0f, 16.0f));
    valueDecay.setBounds (R (163.0f, 254.0f, 133.0f, 16.0f));

    knobDamp.setBounds (R (318.0f, 162.0f, 90.0f, 90.0f));
    lblDamp.setBounds (R (296.0f, 144.0f, 134.0f, 16.0f));
    valueDamp.setBounds (R (296.0f, 254.0f, 134.0f, 16.0f));
    lblToneHigh.setBounds (R (296.0f, 272.0f, 67.0f, 12.0f));
    lblToneLow.setBounds (R (363.0f, 272.0f, 67.0f, 12.0f));

    knobModSpeed.setBounds (R (52.0f, 298.0f, 90.0f, 90.0f));
    lblModSpeed.setBounds (R (30.0f, 282.0f, 133.0f, 16.0f));
    valueModSpeed.setBounds (R (30.0f, 390.0f, 133.0f, 16.0f));

    knobModDepth.setBounds (R (185.0f, 298.0f, 90.0f, 90.0f));
    lblModDepth.setBounds (R (163.0f, 282.0f, 133.0f, 16.0f));
    valueModDepth.setBounds (R (163.0f, 390.0f, 133.0f, 16.0f));

    // OUTPUT (centred fader only; no MIX label, no numeric value, no bypass)
    faderMix.setBounds (R (670.0f, 156.0f, 150.0f, 288.0f));
    lblWet.setBounds (R (636.0f, 154.0f, 54.0f, 16.0f));
    lblDry.setBounds (R (636.0f, 426.0f, 54.0f, 16.0f));

    // OCTAVE (shelf knobs aligned with the mode bank)
    lblEffectMode.setBounds (R (30.0f, 452.0f, 320.0f, 16.0f));
    comboEffectMode.setBounds (R (30.0f, 470.0f, 320.0f, 42.0f));
    lblOctaveDryMix.setBounds (R (30.0f, 522.0f, 320.0f, 16.0f));
    btnOctaveDryMix.setBounds (R (30.0f, 540.0f, 320.0f, 38.0f));

    lblEq1.setBounds (R (400.0f, 452.0f, 84.0f, 16.0f));
    knobEq1.setBounds (R (400.0f, 470.0f, 84.0f, 84.0f));
    valueEq1.setBounds (R (400.0f, 556.0f, 84.0f, 16.0f));

    lblEq2.setBounds (R (490.0f, 452.0f, 84.0f, 16.0f));
    knobEq2.setBounds (R (490.0f, 470.0f, 84.0f, 84.0f));
    valueEq2.setBounds (R (490.0f, 556.0f, 84.0f, 16.0f));

    // PERFORMANCE (three interlocked push buttons; state shown by the panel lamp)
    lblFootswitchMode.setBounds (R (620.0f, 506.0f, 252.0f, 16.0f));
    comboFootswitchMode.setBounds (R (620.0f, 524.0f, 252.0f, 44.0f));
    lblPerformNote.setBounds (R (620.0f, 578.0f, 252.0f, 16.0f));
}
