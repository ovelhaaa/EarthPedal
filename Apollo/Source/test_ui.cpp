#include "PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace
{
void require (bool ok, const juce::String& message)
{
    if (! ok) throw std::runtime_error (message.toStdString());
}
void flush()
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil (120);
}
juce::Component* control (juce::Component& editor, const juce::String& title)
{
    for (auto* child : editor.getChildren())
        if (child->getTitle() == title) return child;
    throw std::runtime_error ("Missing control: " + title.toStdString());
}
juce::MouseEvent event (juce::Component& c, bool pressed, float x = 10.0f)
{
    const auto now = juce::Time::getCurrentTime();
    return { juce::Desktop::getInstance().getMainMouseSource(), { x, 10 },
             juce::ModifierKeys (pressed ? juce::ModifierKeys::leftButtonModifier : 0),
             1, 0, 0, 0, 0, &c, &c, now, { x, 10 }, now, 1, false };
}
struct Contract
{
    const char* id;
    const char* title;
    float low, high, defaultValue;
    const char* choices = nullptr;
};
const std::array<Contract, 15> contract {{
    { "predelay", "Pre-delay", 0, 1, 0 },
    { "mix", "Mix", 0, 1, 0.5f },
    { "decay", "Decay", 0, 1, 0.877465f },
    { "moddepth", "Mod Depth", 0, 1, 0.0625f },
    { "modspeed", "Mod Rate", 0, 1, 0.0466667f },
    { "damp", "Tone", 0, 1, 0.5f },
    { "eq1_gain", "Presence", -24, 24, -11 },
    { "eq2_gain", "Body", -24, 24, 5 },
    { "time_scale", "Size", 0, 2, 2, "Small|Medium|Large" },
    { "effect_mode", "Mode", 0, 3, 0, "None|Up Octave|Down Octave|Both Octaves" },
    { "footswitch_mode", "Action", 0, 2, 0, "Freeze|Overdrive|Effect" },
    { "input_diffusion", "Input Diffusion", 0, 1, 1 },
    { "octave_dry_mix", "Reverb Feed", 0, 1, 1 },
    { "bypass", "Bypass", 0, 1, 0 },
    { "momentary_effect", "Perform", 0, 1, 0 }
}};
void set (ApolloAudioProcessor& p, const char* id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}
void checkBounds (juce::Component& editor)
{
    for (auto* c : editor.getChildren())
        if (c->isVisible() && c->getWantsKeyboardFocus())
            require (editor.getLocalBounds().contains (c->getBounds()) && ! c->getBounds().isEmpty(),
                     "Control clipped or empty: " + c->getTitle());
}
void snapshot (juce::Component& editor, const juce::File& directory, const juce::String& name, float scale)
{
    auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, scale);
    require (image.isValid(), "Invalid editor snapshot");
    auto stream = directory.getChildFile (name + ".png").createOutputStream();
    juce::PNGImageFormat png;
    require (stream != nullptr && png.writeImageToStream (image, *stream), "Cannot write snapshot " + name);
}
}

void runInterfaceTests (const juce::File& output)
{
    require (ApolloTheme::fontFamily() == "Montserrat", "Unexpected UI font family");
    require (output.createDirectory().wasOk(), "Cannot create snapshot directory");
    ApolloAudioProcessor processor;
    require (processor.getParameters().size() == 15, "Parameter count changed");
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setVisible (true);
    flush();
    int index = 0;
    for (const auto& c : contract)
    {
        auto* param = processor.apvts.getParameter (c.id);
        require (param != nullptr && processor.getParameters()[index++] == param, "Parameter ID/order changed");
        require (param->getVersionHint() == 1, "Parameter version changed");
        const auto range = param->getNormalisableRange();
        require (range.start == c.low && range.end == c.high, "Range changed: " + juce::String (c.id));
        // JUCE's legacy min/max constructor has a 0.01 interval, so e.g.
        // declared decay 0.877465 is presented by APVTS as 0.88. Preserve it.
        require (std::abs (param->getDefaultValue() - param->convertTo0to1 (c.defaultValue)) < 1.0e-6f,
                 "Default changed: " + juce::String (c.id));
        if (index <= 8) require (range.interval == 0.01f, "Float interval changed");
        if (c.choices != nullptr)
        {
            const auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param);
            require (choice != nullptr && choice->choices.joinIntoString ("|") == c.choices,
                     "Choice contract changed");
        }
        else if (index > 11)
            require (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr, "Boolean type changed");
        else if (index <= 8)
            require (dynamic_cast<juce::AudioParameterFloat*> (param) != nullptr, "Float type changed");

        auto* widget = control (*editor, c.title);
        require (widget->isVisible() && widget->getWantsKeyboardFocus() && widget->getExplicitFocusOrder() > 0,
                 "Control invisible or not focusable: " + juce::String (c.id));

        // Host -> UI and UI -> APVTS for every attachment.
        const float value = c.choices != nullptr ? 1.0f : c.low + 0.75f * (c.high - c.low);
        set (processor, c.id, value);
        flush();
        const float expected = processor.apvts.getRawParameterValue (c.id)->load();
        if (auto* slider = dynamic_cast<juce::Slider*> (widget))
        {
            require (std::abs (slider->getValue() - expected) < 1.0e-5, "Slider attachment");
            require (slider->getMinimum() == c.low && slider->getMaximum() == c.high, "Visual slider range");
            slider->setValue (c.low, juce::sendNotificationSync);
        }
        else if (auto* combo = dynamic_cast<juce::ComboBox*> (widget))
        {
            require (combo->getSelectedItemIndex() == (int) expected, "Choice attachment");
            combo->setSelectedItemIndex (0, juce::sendNotificationSync);
        }
        else if (auto* button = dynamic_cast<juce::ToggleButton*> (widget))
        {
            require (button->getToggleState() == (expected > 0.5f), "Button attachment");
            button->setToggleState (false, juce::sendNotificationSync);
        }
        require (processor.apvts.getRawParameterValue (c.id)->load() == c.low, "UI -> APVTS mismatch");

        if (dynamic_cast<juce::Slider*> (widget) != nullptr || c.choices != nullptr)
        {
            widget->keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
            flush();
            require (processor.apvts.getRawParameterValue (c.id)->load() > c.low, "Arrow key did not change parameter");
            widget->mouseDoubleClick (event (*widget, true));
            flush();
            require (std::abs (processor.apvts.getRawParameterValue (c.id)->load()
                              - param->convertFrom0to1 (param->getDefaultValue())) < 1.0e-6f,
                     "Double-click default reset failed");
        }
    }
    std::puts ("PASS: all 15 parameter contracts and bidirectional attachments");

    for (const auto& c : contract) set (processor, c.id, c.high);
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    ApolloAudioProcessor recalled;
    recalled.setStateInformation (state.getData(), (int) state.getSize());
    for (const auto& c : contract)
        require (recalled.apvts.getRawParameterValue (c.id)->load() == c.high, "XML recall failed");
    processor.setStateInformation (state.getData(), (int) state.getSize());
    flush();
    // Explicit legacy APVTS XML schema, independent of today's serializer.
    juce::XmlElement legacy ("Parameters");
    for (const auto& c : contract)
    {
        auto* child = legacy.createNewChildElement ("PARAM");
        child->setAttribute ("id", c.id);
        child->setAttribute ("value", c.high);
    }
    juce::MemoryBlock legacyState;
    juce::AudioProcessor::copyXmlToBinary (legacy, legacyState);
    recalled.setStateInformation (legacyState.getData(), (int) legacyState.getSize());
    for (const auto& c : contract)
        require (recalled.apvts.getRawParameterValue (c.id)->load() == c.high, "Legacy XML recall failed");
    for (const auto& c : contract) set (processor, c.id, c.defaultValue);
    flush();
    std::puts ("PASS: XML/APVTS recall for all 15 parameters");

    auto* feed = dynamic_cast<ReverbFeedButton*> (control (*editor, "Reverb Feed"));
    feed->mouseDown (event (*feed, true));
    feed->mouseUp (event (*feed, false));
    require (processor.apvts.getRawParameterValue ("octave_dry_mix")->load() == 0, "OCT feed polarity");
    feed->mouseDown (event (*feed, true, (float) feed->getWidth() - 10));
    feed->mouseUp (event (*feed, false, (float) feed->getWidth() - 10));
    require (processor.apvts.getRawParameterValue ("octave_dry_mix")->load() == 1, "OCT + DRY feed polarity");

    auto* gate = dynamic_cast<MomentaryGateButton*> (control (*editor, "Perform"));
    const auto gateValue = [&] { return processor.apvts.getRawParameterValue ("momentary_effect")->load(); };
    auto down = event (*gate, true), up = event (*gate, false);
    gate->mouseDown (down);
    require (gateValue() == 1, "Mouse gate did not open");
    gate->mouseUp (up);
    require (gateValue() == 0, "Mouse gate stuck");
    for (int key : { juce::KeyPress::spaceKey, juce::KeyPress::returnKey })
    {
        gate->keyPressed (juce::KeyPress (key));
        require (gateValue() == 1, "Keyboard gate did not open");
        gate->keyStateChanged (false);
        require (gateValue() == 0, "Keyboard gate stuck");
    }
    gate->mouseDown (down);
    gate->focusLost (juce::Component::focusChangedDirectly);
    require (gateValue() == 0, "Mouse gate stuck on focus loss");
    gate->keyPressed (juce::KeyPress (juce::KeyPress::spaceKey));
    gate->focusLost (juce::Component::focusChangedDirectly);
    require (gateValue() == 0, "Keyboard gate stuck on focus loss");
    gate->mouseDown (down);
    gate->setVisible (false);
    require (gateValue() == 0, "Gate stuck when hidden");
    gate->setVisible (true);
    gate->mouseDown (down);
    gate->setEnabled (false);
    require (gateValue() == 0, "Gate stuck when disabled");
    gate->setEnabled (true);
    set (processor, "momentary_effect", 1);
    flush();
    gate->focusLost (juce::Component::focusChangedDirectly);
    require (gateValue() == 1 && gate->getToggleState(), "Local focus cancelled host automation");
    set (processor, "momentary_effect", 0);
    flush();
    std::puts ("PASS: mouse/Space/Enter gate release, focus, hide, disable and automation ownership");

    for (int width : { 900, 1400 })
    {
        editor->setSize (width, juce::roundToInt ((float) width * 620 / 900));
        checkBounds (*editor);
        snapshot (*editor, output, width == 900 ? "minimum" : "maximum", 1);
    }
    editor->setSize (900, 620);
    for (float scale : { 1.0f, 1.5f, 2.0f })
        snapshot (*editor, output, "default-" + juce::String (juce::roundToInt (scale * 100)), scale);
    for (int size = 0; size < 3; ++size)
    {
        set (processor, "time_scale", (float) size); flush();
        snapshot (*editor, output, "size-" + juce::String (size), 1);
    }
    set (processor, "time_scale", 2);
    for (bool diffusion : { false, true })
    {
        set (processor, "input_diffusion", diffusion ? 1.0f : 0.0f); flush();
        snapshot (*editor, output, diffusion ? "diffusion-on" : "diffusion-off", 1);
    }
    for (int octave = 0; octave < 4; ++octave)
    {
        set (processor, "effect_mode", (float) octave); flush();
        snapshot (*editor, output, "octave-" + juce::String (octave), 1);
    }
    for (bool feed : { false, true })
    {
        set (processor, "octave_dry_mix", feed ? 1.0f : 0.0f); flush();
        snapshot (*editor, output, feed ? "feed-oct-dry" : "feed-oct", 1);
    }
    for (int action = 0; action < 3; ++action)
    {
        set (processor, "footswitch_mode", (float) action);
        set (processor, "momentary_effect", 0); flush();
        const bool dim = (bool) control (*editor, "Presence")->getProperties()[ApolloTheme::dimProperty];
        require (dim == (action == 2), "Octave gate state disagrees with processor");
        gate->keyPressed (juce::KeyPress (juce::KeyPress::spaceKey)); flush();
        require (! (bool) control (*editor, "Presence")->getProperties()[ApolloTheme::dimProperty],
                 "Octave gate ON did not light shelves");
        snapshot (*editor, output, "perform-" + juce::String (action), 1);
        gate->keyStateChanged (false);
    }
    set (processor, "footswitch_mode", 0);
    for (float damp : { 0.2f, 0.5f, 0.8f })
    {
        set (processor, "damp", damp); flush();
        snapshot (*editor, output, "tone-" + juce::String (juce::roundToInt (damp * 100)), 1);
    }
    set (processor, "damp", 0.5f);
    set (processor, "bypass", 1); flush();
    snapshot (*editor, output, "bypass", 1);
    gate->mouseDown (down);
    editor.reset();
    require (gateValue() == 0, "Gate stuck after editor destruction");
    std::puts ("PASS: resize limits, 100/150/200% real editor renders and state matrix");
}
