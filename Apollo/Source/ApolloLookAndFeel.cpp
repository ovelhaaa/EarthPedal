#include "ApolloLookAndFeel.h"
#include <cmath>

using juce::Graphics;

using juce::Point;
using juce::Justification;

ApolloLookAndFeel::ApolloLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, ApolloTheme::orange);
    setColour (juce::Slider::thumbColourId, ApolloTheme::orange);
    setColour (juce::Slider::textBoxTextColourId, ApolloTheme::textOnPanel);
    setColour (juce::Slider::textBoxBackgroundColourId, ApolloTheme::graphiteDeep);
    setColour (juce::Slider::textBoxOutlineColourId, ApolloTheme::graphiteEdge);
    setColour (juce::Slider::textBoxHighlightColourId, ApolloTheme::orange.withAlpha (0.4f));
    setColour (juce::TextEditor::focusedOutlineColourId, ApolloTheme::orange);
    setColour (juce::CaretComponent::caretColourId, ApolloTheme::orangeBright);
    setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, ApolloTheme::graphiteDeep);
    setColour (juce::Label::textColourId, ApolloTheme::textOnPanel);
    setColour (juce::PopupMenu::backgroundColourId, ApolloTheme::graphite);
    setColour (juce::PopupMenu::textColourId, ApolloTheme::textOnPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ApolloTheme::orange);
    setColour (juce::PopupMenu::highlightedTextColourId, ApolloTheme::graphiteDeep);
    setColour (juce::TooltipWindow::backgroundColourId, ApolloTheme::chassisMid);
    setColour (juce::TooltipWindow::textColourId, ApolloTheme::textOnChassis);
    setColour (juce::TooltipWindow::outlineColourId, ApolloTheme::chassisEdge);
}

juce::Font ApolloLookAndFeel::getPopupMenuFont() { return ApolloTheme::font (sc (11.0f), ApolloTheme::FontWeight::Medium); }
juce::TextLayout ApolloLookAndFeel::tooltipLayout (const juce::String& text)
{
    juce::AttributedString attributed;
    attributed.setJustification (juce::Justification::centred);
    attributed.append (text, ApolloTheme::font (sc (11.0f), ApolloTheme::FontWeight::Regular),
                       findColour (juce::TooltipWindow::textColourId));
    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths (attributed, sc (400.0f));
    return layout;
}

juce::Rectangle<int> ApolloLookAndFeel::getTooltipBounds (const juce::String& text,
                                                        juce::Point<int> position,
                                                        juce::Rectangle<int> parentArea)
{
    const auto layout = tooltipLayout (text);
    const int width = juce::roundToInt (std::ceil (layout.getWidth() + sc (14.0f)));
    const int height = juce::roundToInt (std::ceil (layout.getHeight() + sc (6.0f)));
    return juce::Rectangle<int> (position.x > parentArea.getCentreX() ? position.x - width - 12 : position.x + 24,
                                 position.y > parentArea.getCentreY() ? position.y - height - 6 : position.y + 6,
                                 width, height).constrainedWithin (parentArea);
}

void ApolloLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    g.fillAll (findColour (juce::TooltipWindow::backgroundColourId));
    g.setColour (findColour (juce::TooltipWindow::outlineColourId));
    g.drawRect (0, 0, width, height);
    tooltipLayout (text).draw (g, juce::Rectangle<float> ((float) width, (float) height));
}

void ApolloLookAndFeel::drawKnobScale (Graphics& g, Point<float> centre, float outerRadius,
                                      float innerRadius, float startAngle, float endAngle,
                                      bool bipolar, bool dim)
{
    constexpr int ticks = 16;
    for (int i = 0; i <= ticks; ++i)
    {
        const float angle = startAngle + (float) i / ticks * (endAngle - startAngle);
        const bool middle = bipolar && i == ticks / 2;
        const bool major = i % 4 == 0;
        const float inner = innerRadius - (major ? sc (2.0f) : 0.0f);
        g.setColour (middle ? ApolloTheme::orange : ApolloTheme::textOnPanelDim.withAlpha (dim ? 0.5f : 0.8f));
        g.drawLine (centre.x + std::sin (angle) * inner, centre.y - std::cos (angle) * inner,
                    centre.x + std::sin (angle) * outerRadius, centre.y - std::cos (angle) * outerRadius,
                    sc (major ? 1.15f : 0.65f));
    }
}

void ApolloLookAndFeel::drawInstrumentKnob (Graphics& g, Point<float> centre, float radius,
                                            float angle, bool dimmed, bool focused, bool active)
{
    const auto body = juce::Rectangle<float> (centre.x - radius, centre.y - radius, radius * 2, radius * 2);
    g.setColour (active ? ApolloTheme::graphiteLight : ApolloTheme::graphiteDeep);
    g.fillEllipse (body);
    g.setColour (ApolloTheme::graphiteEdge);
    g.drawEllipse (body.reduced (sc (0.5f)), sc (0.8f));
    g.setColour (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::textOnPanel);
    g.drawLine (centre.x + std::sin (angle) * radius * 0.32f,
                centre.y - std::cos (angle) * radius * 0.32f,
                centre.x + std::sin (angle) * radius * 0.86f,
                centre.y - std::cos (angle) * radius * 0.86f, sc (2.2f));
    if (focused)
    {
        g.setColour (ApolloTheme::orange.withAlpha (0.65f));
        g.drawEllipse (body.expanded (sc (2.0f)), sc (0.9f));
    }
}

void ApolloLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                         float pos, float start, float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const bool dim = isDimmed (slider);
    const bool bipolar = (bool) slider.getProperties()[ApolloTheme::bipolarProperty];
    const bool hover = slider.isMouseOverOrDragging();
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float origin = bipolar ? (start + end) * 0.5f : start;
    const float arcRadius = radius * 0.75f;
    const juce::PathStrokeType stroke (sc (2.5f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, start, end, true);
    g.setColour (ApolloTheme::graphiteEdge);
    g.strokePath (arc, stroke);
    if (std::abs (angle - origin) > 0.001f)
    {
        arc.clear();
        arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                           juce::jmin (origin, angle), juce::jmax (origin, angle), true);
        g.setColour ((hover ? ApolloTheme::orangeBright : ApolloTheme::orange).withAlpha (dim ? 0.54f : 1.0f));
        g.strokePath (arc, stroke);
    }
    drawKnobScale (g, centre, radius - sc (2), radius * 0.89f, start, end, bipolar, dim);
    drawInstrumentKnob (g, centre, radius * 0.61f, angle, dim, slider.hasKeyboardFocus (true), hover);
}

void ApolloLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                         float pos, float minPos, float maxPos,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, pos, minPos, maxPos, style, slider);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    ApolloTheme::drawInsetWell (g, { (float) x, cy - sc (3), (float) width, sc (6) }, sc (3));
    g.setColour (ApolloTheme::orange);
    g.fillRoundedRectangle ({ (float) x, cy - sc (2), juce::jmax (0.0f, pos - (float) x), sc (4) }, sc (2));
    const juce::Rectangle<float> thumb (pos - sc (4), cy - sc (8), sc (8), sc (16));
    g.setColour (slider.isMouseOverOrDragging() ? ApolloTheme::orangeBright : ApolloTheme::textOnPanel);
    g.fillRoundedRectangle (thumb, sc (2));
    if (slider.hasKeyboardFocus (true))
        drawFocusHalo (g, thumb.expanded (sc (3)), sc (3));
}

juce::Label* ApolloLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (ApolloTheme::valueFont (sc (ApolloTheme::Metrics::controlValue)));
    return label;
}

void ApolloLookAndFeel::drawFocusHalo (Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    g.setColour (ApolloTheme::orange.withAlpha (0.65f));
    g.drawRoundedRectangle (bounds.reduced (sc (0.5f)), corner, sc (0.9f));
}

void ApolloLookAndFeel::drawRocker (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (down);
    const bool on = button.getToggleState(), dim = isDimmed (button);
    const auto b = button.getLocalBounds().toFloat().reduced (sc (2));
    const bool feed = (bool) button.getProperties()[ApolloTheme::feedProperty];
    const int count = feed ? 2 : 1;
    for (int i = 0; i < count; ++i)
    {
        auto face = juce::Rectangle<float> (b.getX() + (float) i * b.getWidth() / (float) count, b.getY(),
                                      b.getWidth() / (float) count, b.getHeight()).reduced (sc (1));
        const bool selected = feed ? (i == (on ? 1 : 0)) : on;
        const bool hovered = highlighted && face.contains (button.getMouseXYRelative().toFloat());
        g.setColour (selected ? (hovered ? ApolloTheme::orangeBright : ApolloTheme::orange).withAlpha (dim ? 0.38f : 1.0f)
                              : hovered ? ApolloTheme::graphiteEdge : ApolloTheme::graphiteLight);
        g.fillRoundedRectangle (face, sc (2));
        g.setColour (ApolloTheme::graphiteEdge.withAlpha (highlighted ? 1.0f : 0.5f));
        g.drawRoundedRectangle (face, sc (2), sc (0.8f));
        g.setColour (selected && ! dim ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
        g.setFont (ApolloTheme::font (sc (10.5f), ApolloTheme::FontWeight::Medium, 0.015f));
        g.drawText (feed ? (i == 0 ? "OCT" : "OCT + DRY") : (on ? "ON" : "OFF"),
                    face, Justification::centred, false);
    }
}

void ApolloLookAndFeel::drawMomentary (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (down);
    // Host automation may release the gate while the pointer remains down.
    // The light always follows the attached parameter state.
    const bool active = button.getToggleState();
    auto face = button.getLocalBounds().toFloat().reduced (sc (3));
    g.setColour (active ? (highlighted ? ApolloTheme::orangeBright : ApolloTheme::orange)
                        : ApolloTheme::graphiteLight);
    g.fillRoundedRectangle (face, sc (3));
    g.setColour (ApolloTheme::graphiteEdge.withAlpha (highlighted ? 1.0f : 0.6f));
    g.drawRoundedRectangle (face, sc (3), sc (1));
    g.setColour (active ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
    g.setFont (ApolloTheme::font (sc (15.0f), ApolloTheme::FontWeight::SemiBold, 0.08f));
    g.drawText (active ? "PERFORM  /  HELD" : "PERFORM", face, Justification::centred, false);
}

void ApolloLookAndFeel::drawBypass (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (down);
    const bool bypassed = button.getToggleState();
    auto face = button.getLocalBounds().toFloat().reduced (sc (2));
    g.setColour (highlighted ? ApolloTheme::graphiteLight : ApolloTheme::graphite);
    g.fillRoundedRectangle (face, sc (3));
    g.setColour (bypassed ? ApolloTheme::textOnPanelDim : ApolloTheme::orangeBright);
    const juce::Rectangle<float> indicator (face.getX() + sc (12), face.getCentreY() - sc (3), sc (6), sc (6));
    if (bypassed)
        g.drawEllipse (indicator, sc (1));
    else
        g.fillEllipse (indicator);
    g.setColour (ApolloTheme::textOnPanel);
    g.setFont (ApolloTheme::labelFont (sc (11.0f)));
    g.drawText (bypassed ? "BYPASS  /  ON" : "BYPASS  /  OFF",
                face.withTrimmedLeft (sc (22)), Justification::centred);

}

void ApolloLookAndFeel::drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hover, bool down)
{
    switch ((ApolloTheme::ButtonStyle) (int) b.getProperties()[ApolloTheme::styleProperty])
    {
        case ApolloTheme::ButtonStyle::Rocker: drawRocker (g, b, hover, down); break;
        case ApolloTheme::ButtonStyle::Momentary: drawMomentary (g, b, hover, down); break;
        case ApolloTheme::ButtonStyle::Bypass: drawBypass (g, b, hover, down); break;
        case ApolloTheme::ButtonStyle::Plain: LookAndFeel_V4::drawToggleButton (g, b, hover, down); break;
    }
    if (b.hasKeyboardFocus (true))
        drawFocusHalo (g, b.getLocalBounds().toFloat().reduced (sc (0.5f)), sc (3));
}

void ApolloLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox& box)
{
    juce::ignoreUnused (down, bx, by, bw, bh);
    drawPushBank (g, { 0, 0, (float) w, (float) h }, box, isDimmed (box));
    if (box.hasKeyboardFocus (true))
        drawFocusHalo (g, box.getLocalBounds().toFloat(), sc (3));
}

void ApolloLookAndFeel::drawPushBank (Graphics& g, juce::Rectangle<float> b, juce::ComboBox& box, bool dim)
{
    const int n = box.getNumItems();
    if (n == 0) return;
    const float width = b.getWidth() / (float) n;
    const auto mouse = box.getMouseXYRelative();
    for (int i = 0; i < n; ++i)
    {
        const auto slot = juce::Rectangle<float> (b.getX() + (float) i * width, b.getY(), width, b.getHeight());
        const auto face = slot.reduced (sc (1.5f));
        const bool on = box.getSelectedItemIndex() == i;
        const bool hovered = slot.contains (mouse.toFloat());
        g.setColour (on ? (hovered ? ApolloTheme::orangeBright : ApolloTheme::orange).withAlpha (dim ? 0.38f : 1.0f)
                        : hovered ? ApolloTheme::graphiteEdge : ApolloTheme::graphiteLight);
        g.fillRoundedRectangle (face, sc (2));
        g.setColour (ApolloTheme::graphiteEdge.withAlpha (0.5f));
        g.drawRoundedRectangle (face, sc (2), sc (0.8f));
        g.setColour (on && ! dim ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
        g.setFont (ApolloTheme::font (sc (10.5f), ApolloTheme::FontWeight::Medium, 0.02f));
        g.drawText (box.getItemText (i), face.reduced (sc (2)), Justification::centred, false);
    }
}

void ApolloLookAndFeel::drawLabel (Graphics& g, juce::Label& l)
{
    if (! (bool) l.getProperties()[ApolloTheme::captionProperty])
    {
        LookAndFeel_V4::drawLabel (g, l);
        return;
    }
    g.setColour (l.findColour (juce::Label::textColourId));
    g.setFont (l.getFont());
    g.drawText (l.getText(), l.getLocalBounds().toFloat(), l.getJustificationType(), false);
}
void ApolloLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    g.fillAll (ApolloTheme::graphite);
    g.setColour (ApolloTheme::graphiteEdge);
    g.drawRect (0, 0, width, height, 1);
}
void ApolloLookAndFeel::drawPopupMenuItem (Graphics& g, const juce::Rectangle<int>& area,
    bool separator, bool active, bool highlighted, bool ticked, bool submenu,
    const juce::String& text, const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* colour)
{
    LookAndFeel_V4::drawPopupMenuItem (g, area, separator, active, highlighted, ticked, submenu, text, shortcut, icon, colour);
}
