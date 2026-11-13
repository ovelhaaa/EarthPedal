#include "PluginEditor.h"

#include <cmath>
#include <locale>
#include <sstream>

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

void styleSlider (juce::Slider& slider, const juce::String& name)
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setWantsKeyboardFocus (true);
    slider.setTitle (name);
    slider.setDescription (name + " control. Use arrow keys to adjust, double click the control to reset,"
                           " or click the value to type a new setting.");
}

void useDisplayUnits (juce::Slider& slider, double scale, double offset,
                      int decimals, const juce::String& suffix)
{
    slider.textFromValueFunction = [=] (double value)
    {
        return juce::String (value * scale + offset, decimals) + suffix;
    };
    slider.valueFromTextFunction = [&slider, scale, offset, suffix] (const juce::String& text)
    {
        auto number = text.trim();
        if (number.endsWithIgnoreCase (suffix.trimStart()))
            number = number.dropLastCharacters (suffix.trimStart().length()).trimEnd();
        number = number.replaceCharacter (',', '.');
        std::istringstream input (number.toStdString());
        input.imbue (std::locale::classic());
        double value = 0.0;
        if (! (input >> value) || ! input.eof() || ! std::isfinite (value))
            return slider.getValue();
        return (value - offset) / scale;
    };
    slider.updateText();
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
    : AudioProcessorEditor (&p), audioProcessor (p), spaceField (p.apvts)
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
    setDescription ("Apollo plate reverb editor. Keyboard focus follows the header, Space, Modulation, Octave and Performance.");
    setLookAndFeel (&customLookAndFeel);

    addAndMakeVisible (spaceField);
    for (auto* panel : { &reverbPanel, &modulationPanel, &octavePanel, &performancePanel })
        addAndMakeVisible (*panel);

    auto addKnob = [this] (juce::Slider& knob, juce::Label& caption, const char* id,
                           const juce::String& name,
                           std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment)
    {
        addAndMakeVisible (knob);
        addAndMakeVisible (caption);
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setMouseDragSensitivity (260);
        styleSlider (knob, name);
        styleCaption (caption, name, ApolloTheme::Metrics::controlLabel);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id, knob);
        auto* parameter = audioProcessor.apvts.getParameter (id);
        knob.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    };

    addKnob (knobPredelay, lblPredelay, "predelay", "Pre-delay", attachPredelay);
    addKnob (knobDecay, lblDecay, "decay", "Decay", attachDecay);
    addKnob (knobDamp, lblDamp, "damp", "Tone", attachDamp);
    addKnob (knobModSpeed, lblModSpeed, "modspeed", "Mod Rate", attachModSpeed);
    addKnob (knobModDepth, lblModDepth, "moddepth", "Mod Depth", attachModDepth);
    addKnob (knobEq1, lblEq1, "eq1_gain", "Presence", attachEq1);
    addKnob (knobEq2, lblEq2, "eq2_gain", "Body", attachEq2);
    knobPredelay.setTooltip ("Atrasa a entrada do reverb.");
    knobDecay.setTooltip ("Define quanto tempo o reverb sustenta.");
    knobDamp.setTooltip ("High Cut a esquerda; Low Cut a direita.");
    knobModSpeed.setTooltip ("Modulation speed multiplier, 0.30x to 15.30x, applied to the four tank LFO rates.");
    knobModDepth.setTooltip ("Define a intensidade do movimento.");
    knobEq1.setTooltip ("Ajusta o shelf alto da ramificacao de oitava.");
    knobEq2.setTooltip ("Ajusta o shelf baixo da ramificacao de oitava.");

    knobDamp.getProperties().set (ApolloTheme::bipolarProperty, true);
    useDisplayUnits (knobPredelay, 1000.0, 0.0, 0, " ms");
    useDisplayUnits (knobDecay, 100.0, 0.0, 0, "%");
    useDisplayUnits (knobModSpeed, 15.0, 0.3, 2, "x");
    useDisplayUnits (knobModDepth, 100.0, 0.0, 0, "%");
    useDisplayUnits (knobEq1, 1.0, 0.0, 2, " dB");
    useDisplayUnits (knobEq2, 1.0, 0.0, 2, " dB");
    // Signed percentage keeps the neutral point editable and unambiguous.
    useDisplayUnits (knobDamp, 200.0, -100.0, 0, "%");
    knobDamp.setTooltip ("Negative: high cut. Zero: flat. Positive: low cut.");

    auto addChoice = [this] (ApolloSelector& combo, juce::Label& caption, const char* id, const juce::String& name,
                             const juce::StringArray& choices,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attachment)
    {
        styleCaption (caption, name, ApolloTheme::Metrics::controlLabel);
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
    comboTimeScale.setTooltip ("Escolhe o tamanho do espaco.");
    comboEffectMode.setTooltip ("Escolhe Off, Up, Down ou Up + Down para a ramificacao de oitava.");
    comboFootswitchMode.setTooltip ("Select Freeze, Drive or Octave; hold Perform or automate the momentary gate.");

    addAndMakeVisible (lblInputDiffusion);
    styleCaption (lblInputDiffusion, "Input Diffusion", ApolloTheme::Metrics::controlLabel);
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
    faderMix.setSliderStyle (juce::Slider::LinearHorizontal);
    styleSlider (faderMix, "Mix");
    faderMix.setTooltip ("Equilibra sinal direto e reverb.");
    faderMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "mix", faderMix);
    useDisplayUnits (faderMix, 100.0, 0.0, 0, "%");
    addAndMakeVisible (lblMix);
    styleCaption (lblMix, "Mix", ApolloTheme::Metrics::controlLabel);
    auto* mixParameter = audioProcessor.apvts.getParameter ("mix");
    faderMix.setDoubleClickReturnValue (true, mixParameter->convertFrom0to1 (mixParameter->getDefaultValue()));

    addAndMakeVisible (lblDry);
    styleCaption (lblDry, "DRY", 10.0f);
    addAndMakeVisible (lblWet);
    styleCaption (lblWet, "WET", 10.0f);
    int focusOrder = 1;
    for (juce::Component* component : std::initializer_list<juce::Component*> {
             &btnBypass, &knobPredelay, &knobDecay, &knobDamp, &comboTimeScale, &btnInputDiffusion,
             &knobModSpeed, &knobModDepth, &comboEffectMode, &btnOctaveDryMix, &knobEq1, &knobEq2,
             &comboFootswitchMode, &btnMomentaryEffect, &faderMix })
        component->setExplicitFocusOrder (focusOrder++);

    resized(); // layout/font scale after all controls and attachments exist
    updateStatePresentation();
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
    const auto caption = [s] (juce::Label& l) { l.setFont (ApolloTheme::labelFont (ApolloTheme::Metrics::controlLabel * s)); };

    for (auto* label : { &lblPredelay, &lblDecay, &lblDamp, &lblModSpeed, &lblModDepth, &lblEq1, &lblEq2,
                         &lblTimeScale, &lblEffectMode, &lblFootswitchMode, &lblInputDiffusion, &lblOctaveDryMix,
                         &lblMix, &lblDry, &lblWet })
        caption (*label);
}

void ApolloAudioProcessorEditor::visibilityChanged()
{
    spaceField.updateTimer();
    if (! isVisible())
        btnMomentaryEffect.releaseLocalGate();
}

void ApolloAudioProcessorEditor::parentHierarchyChanged()
{
    spaceField.updateTimer();
}

void ApolloAudioProcessorEditor::timerCallback()
{
    if (isVisible())
        updateStatePresentation();
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
    modulationPanel.setDimmed (bypassed);
    octavePanel.setDimmed (octaveOff || bypassed);
    for (juce::Component* component : std::initializer_list<juce::Component*> { &knobEq1, &knobEq2, &btnOctaveDryMix })
    {
        if (component->getProperties().set (ApolloTheme::dimProperty, octaveOff || bypassed))
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

void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ApolloTheme;
    g.fillAll (chassis);
    const float s = getDesignScale();

    const auto symbol = scaled (24, 22, 44, 44);
    g.setColour (chassisMid);
    g.fillRoundedRectangle (symbol, 4.0f * s);
    const auto centre = scaled (39, 44, 0, 0).getPosition();
    g.setColour (orange);
    g.fillEllipse (centre.x - 2.0f * s, centre.y - 2.0f * s, 4.0f * s, 4.0f * s);
    g.setColour (textOnChassis);
    for (float radius : { 8.0f, 15.0f, 22.0f })
    {
        juce::Path wave;
        wave.addCentredArc (centre.x, centre.y, radius * s, radius * s,
                            0.0f, 0.95f, 2.19f, true);
        g.strokePath (wave, juce::PathStrokeType (1.2f * s));
    }

    g.setFont (font (37.0f * s, FontWeight::Bold, 0.10f));
    g.drawText ("APOLLO", scaled (80, 14, 320, 39), juce::Justification::centredLeft);
    g.setColour (textOnChassisSoft);
    g.setFont (headingFont (11.0f * s));
    g.drawText ("STEREO SPACE PROCESSOR", scaled (82, 54, 380, 15), juce::Justification::centredLeft);
    g.setColour (chassisEdge);
    g.fillRect (scaled (24, 80, 852, 1));
    g.setColour (orange);
    g.fillRect (scaled (24, 79, 44, 3));
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
    for (auto* panel : { &reverbPanel, &modulationPanel, &octavePanel, &performancePanel })
        panel->setScale (s);

    const auto R = [this] (float x, float y, float w, float h) { return scaled (x, y, w, h).toNearestInt(); };
    btnBypass.setBounds (R (726, 25, 150, 36));
    spaceField.setBounds (R (24, 93, 852, 183));
    reverbPanel.setBounds (R (24, 288, 548, 164));
    modulationPanel.setBounds (R (584, 288, 292, 164));
    octavePanel.setBounds (R (24, 464, 480, 132));
    performancePanel.setBounds (R (516, 464, 360, 132));

    const auto knob = [&] (juce::Slider& control, juce::Label& caption, float cx, float y, float diameter)
    {
        caption.setBounds (R (cx - 45, y - 19, 90, 17));
        control.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::roundToInt (74 * s),
                                 juce::roundToInt (20 * s));
        control.setBounds (R (cx - 44, y, 88, diameter + 20));
    };

    knob (knobPredelay, lblPredelay, 84, 345, 73);
    knob (knobDecay, lblDecay, 179, 342, 79);
    knob (knobDamp, lblDamp, 274, 345, 73);
    lblTimeScale.setBounds (R (333, 324, 223, 17));
    comboTimeScale.setBounds (R (333, 344, 223, 29));
    lblInputDiffusion.setBounds (R (333, 387, 150, 29));
    btnInputDiffusion.setBounds (R (487, 387, 69, 29));

    knob (knobModSpeed, lblModSpeed, 657, 345, 73);
    knob (knobModDepth, lblModDepth, 803, 345, 73);

    lblEffectMode.setBounds (R (38, 498, 222, 16));
    comboEffectMode.setBounds (R (38, 515, 222, 25));
    lblOctaveDryMix.setBounds (R (38, 543, 222, 16));
    btnOctaveDryMix.setBounds (R (38, 560, 222, 25));
    knob (knobEq1, lblEq1, 324, 520, 51);
    knob (knobEq2, lblEq2, 435, 520, 51);

    lblFootswitchMode.setBounds (R (530, 498, 168, 16));
    comboFootswitchMode.setBounds (R (530, 515, 168, 26));
    btnMomentaryEffect.setBounds (R (530, 548, 168, 37));
    lblMix.setBounds (R (710, 498, 150, 16));
    lblDry.setBounds (R (710, 515, 42, 16));
    lblWet.setBounds (R (818, 515, 42, 16));
    faderMix.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::roundToInt (68 * s),
                              juce::roundToInt (20 * s));
    faderMix.setBounds (R (720, 533, 130, 49));
}
