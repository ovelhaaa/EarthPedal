#include "ApolloLookAndFeel.h"
#include <cmath>

using juce::Graphics;

using juce::Point;
using juce::Justification;

ApolloLookAndFeel::ApolloLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, ApolloTheme::orange);
    setColour (juce::Slider::thumbColourId, ApolloTheme::orange);
    setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, ApolloTheme::graphiteDeep);
    setColour (juce::Label::textColourId, ApolloTheme::textOnPanel);
    setColour (juce::PopupMenu::backgroundColourId, ApolloTheme::graphiteDark);
    setColour (juce::PopupMenu::textColourId, ApolloTheme::textOnPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ApolloTheme::orange);
    setColour (juce::PopupMenu::highlightedTextColourId, ApolloTheme::graphiteDeep);
    setColour (juce::TooltipWindow::backgroundColourId, ApolloTheme::chassisMid);
    setColour (juce::TooltipWindow::textColourId, ApolloTheme::textOnChassis);
    setColour (juce::TooltipWindow::outlineColourId, ApolloTheme::chassisEdge);
}

void ApolloLookAndFeel::drawKnobScale (Graphics& g, Point<float> centre, float outerRadius,
                                      float innerRadius, float startAngle, float endAngle,
                                      bool bipolar, bool dim)
{
    constexpr int ticks = 8;
    for (int i = 0; i <= ticks; ++i)
    {
        const float angle = startAngle + (float) i / ticks * (endAngle - startAngle);
        const bool middle = bipolar && i == ticks / 2;
        const bool major = i == 0 || i == ticks || middle;
        const float inner = innerRadius - (major ? sc (2.0f) : 0.0f);
        g.setColour (middle ? ApolloTheme::orange : ApolloTheme::textOnPanelDim.withAlpha (dim ? 0.5f : 0.8f));
        g.drawLine (centre.x + std::sin (angle) * inner, centre.y - std::cos (angle) * inner,
                    centre.x + std::sin (angle) * outerRadius, centre.y - std::cos (angle) * outerRadius,
                    sc (middle ? 1.5f : 0.8f));
    }
}

void ApolloLookAndFeel::drawInstrumentKnob (Graphics& g, Point<float> centre, float radius,
                                            float angle, bool dimmed, bool focused, bool active)
{
    const auto body = juce::Rectangle<float> (centre.x - radius, centre.y - radius, radius * 2, radius * 2);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (body.translated (0, sc (1.5f)));
    juce::ColourGradient finish (ApolloTheme::graphiteLight, body.getX(), body.getY(),
                                 ApolloTheme::graphiteDeep, body.getX(), body.getBottom(), false);
    g.setGradientFill (finish);
    g.fillEllipse (body);
    g.setColour (ApolloTheme::graphiteEdge);
    g.drawEllipse (body.reduced (sc (0.5f)), sc (0.8f));
    g.setColour (juce::Colours::white.withAlpha (active ? 0.14f : 0.06f));
    g.drawEllipse (body.reduced (sc (2.0f)), sc (0.8f));
    g.setColour (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::knobPointer);
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
    drawKnobScale (g, bounds.getCentre(), radius - sc (2), radius * 0.80f, start, end,
                   (bool) slider.getProperties()[ApolloTheme::bipolarProperty], dim);
    drawInstrumentKnob (g, bounds.getCentre(), radius * 0.68f, start + pos * (end - start),
                        dim, slider.hasKeyboardFocus (true), slider.isMouseOverOrDragging());
}

void ApolloLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                         float pos, float minPos, float maxPos,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, pos, minPos, maxPos, style, slider);
        return;
    }
    const auto b = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float cx = b.getCentreX(), top = b.getY(), bottom = b.getBottom();
    ApolloTheme::drawInsetWell (g, { cx - sc (3), top, sc (6), b.getHeight() }, sc (1));
    g.setColour (ApolloTheme::orange.withAlpha (0.35f));
    g.fillRect (juce::Rectangle<float> (cx - sc (1), pos, sc (2), bottom - pos));
    for (int i = 0; i <= 8; ++i)
    {
        const float yy = top + b.getHeight() * (float) i / 8;
        const float len = sc (i % 4 == 0 ? 12.0f : 6.0f);
        g.setColour (ApolloTheme::textOnPanelDim.withAlpha (0.7f));
        g.drawLine (cx - sc (14) - len, yy, cx - sc (14), yy, sc (0.8f));
        g.drawLine (cx + sc (14), yy, cx + sc (14) + len, yy, sc (0.8f));
    }
    auto cap = juce::Rectangle<float> (cx - sc (25), pos - sc (12), sc (50), sc (24));
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (cap.translated (0, sc (2)), sc (2));
    juce::ColourGradient finish (ApolloTheme::chassisMid, cap.getX(), cap.getY(),
                                 ApolloTheme::chassisEdge, cap.getX(), cap.getBottom(), false);
    g.setGradientFill (finish);
    g.fillRoundedRectangle (cap, sc (2));
    g.setColour (ApolloTheme::textOnChassis);
    g.drawLine (cap.getX() + sc (6), pos, cap.getRight() - sc (6), pos, sc (1.4f));
    if (slider.hasKeyboardFocus (true))
        drawFocusHalo (g, cap.expanded (sc (3)), sc (2));
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
        auto face = juce::Rectangle<float> (b.getX() + (float) i * b.getWidth() / count, b.getY(),
                                      b.getWidth() / count, b.getHeight()).reduced (sc (1));
        const bool selected = feed ? (i == (on ? 1 : 0)) : on;
        g.setColour (selected ? ApolloTheme::orange.withAlpha (dim ? 0.38f : 1.0f)
                               : ApolloTheme::graphiteLight);
        g.fillRoundedRectangle (face, sc (2));
        g.setColour (ApolloTheme::graphiteEdge.withAlpha (highlighted ? 1.0f : 0.5f));
        g.drawRoundedRectangle (face, sc (2), sc (0.8f));
        g.setColour (selected && ! dim ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
        g.setFont (ApolloTheme::valueFont (sc (10)));
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
    g.setColour (ApolloTheme::graphiteDeep);
    g.fillRoundedRectangle (face.translated (0, sc (2)), sc (3));
    juce::ColourGradient finish (active ? ApolloTheme::orange : ApolloTheme::graphiteLight,
                                 face.getX(), face.getY(),
                                 active ? ApolloTheme::orangeDeep : ApolloTheme::graphiteDark,
                                 face.getX(), face.getBottom(), false);
    g.setGradientFill (finish);
    g.fillRoundedRectangle (face, sc (3));
    g.setColour (ApolloTheme::graphiteEdge.withAlpha (highlighted ? 1.0f : 0.6f));
    g.drawRoundedRectangle (face, sc (3), sc (1));
    g.setColour (active ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
    g.setFont (ApolloTheme::headingFont (sc (17)));
    g.drawText ("PERFORM", face, Justification::centred, false);
}

void ApolloLookAndFeel::drawBypass (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (highlighted, down);
    const bool bypassed = button.getToggleState();
    auto face = button.getLocalBounds().toFloat().reduced (sc (2));
    g.setColour (ApolloTheme::graphite);
    g.fillRoundedRectangle (face, sc (2));
    g.setColour (bypassed ? ApolloTheme::red : ApolloTheme::orange);
    g.fillEllipse (face.getX() + sc (14), face.getCentreY() - sc (3), sc (6), sc (6));
    g.setFont (ApolloTheme::valueFont (sc (11)));
    g.drawText (bypassed ? "BYPASSED" : "ACTIVE", face.reduced (sc (24), 0).translated (sc (7), 0),
                Justification::centred, false);
}

void ApolloLookAndFeel::drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hover, bool down)
{
    switch ((ApolloTheme::ButtonStyle) (int) b.getProperties()[ApolloTheme::styleProperty])
    {
        case ApolloTheme::ButtonStyle::Rocker: drawRocker (g, b, hover, down); break;
        case ApolloTheme::ButtonStyle::Momentary: drawMomentary (g, b, hover, down); break;
        case ApolloTheme::ButtonStyle::Bypass: drawBypass (g, b, hover, down); break;
        default: LookAndFeel_V4::drawToggleButton (g, b, hover, down); break;
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
    const bool vertical = (bool) box.getProperties()[ApolloTheme::verticalProperty];
    const float gap = sc (4), w = vertical ? b.getWidth() : (b.getWidth() - gap * (n - 1)) / n;
    const float h = vertical ? (b.getHeight() - gap * (n - 1)) / n : b.getHeight();
    for (int i = 0; i < n; ++i)
    {
        const auto face = juce::Rectangle<float> (b.getX() + (vertical ? 0 : i * (w + gap)),
                                            b.getY() + (vertical ? i * (h + gap) : 0), w, h).reduced (sc (1));
        const bool on = box.getSelectedItemIndex() == i;
        g.setColour (on ? ApolloTheme::orange.withAlpha (dim ? 0.38f : 1.0f) : ApolloTheme::graphiteLight);
        g.fillRoundedRectangle (face, sc (2));
        g.setColour (ApolloTheme::graphiteEdge.withAlpha (0.5f));
        g.drawRoundedRectangle (face, sc (2), sc (0.8f));
        g.setColour (on && ! dim ? ApolloTheme::graphiteDeep : ApolloTheme::textOnPanel);
        g.setFont (ApolloTheme::valueFont (sc (9)));
        g.drawText (box.getItemText (i), face.reduced (sc (2)), Justification::centred, false);
    }
}

void ApolloLookAndFeel::drawLabel (Graphics& g, juce::Label& l)
{
    const bool display = (bool) l.getProperties()[ApolloTheme::displayProperty];
    const bool caption = (bool) l.getProperties()[ApolloTheme::captionProperty];
    if (! display && ! caption) { LookAndFeel_V4::drawLabel (g, l); return; }
    g.setColour (display ? (isDimmed (l) ? ApolloTheme::textOnPanelDim : ApolloTheme::orange)
                         : l.findColour (juce::Label::textColourId));
    g.setFont (l.getFont());
    g.drawText (l.getText(), l.getLocalBounds().toFloat(), l.getJustificationType(), false);
}
void ApolloLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    g.fillAll (ApolloTheme::graphiteDark);
    g.setColour (ApolloTheme::graphiteEdge);
    g.drawRect (0, 0, width, height, 1);
}
void ApolloLookAndFeel::drawPopupMenuItem (Graphics& g, const juce::Rectangle<int>& area,
    bool separator, bool active, bool highlighted, bool ticked, bool submenu,
    const juce::String& text, const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* colour)
{
    LookAndFeel_V4::drawPopupMenuItem (g, area, separator, active, highlighted, ticked, submenu, text, shortcut, icon, colour);
}
