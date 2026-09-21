#include "ApolloLookAndFeel.h"

#include <cmath>

using juce::Colour;
using juce::ColourGradient;
using juce::Graphics;
using juce::Justification;
using juce::Line;
using juce::Path;
using juce::Point;
using juce::Rectangle;

//==============================================================================
ApolloLookAndFeel::ApolloLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    ApolloTheme::orange);
    setColour (juce::Slider::rotarySliderOutlineColourId, ApolloTheme::graphiteDeep);
    setColour (juce::Slider::thumbColourId,               ApolloTheme::orange);
    setColour (juce::Slider::trackColourId,               ApolloTheme::graphiteDeep);

    setColour (juce::ToggleButton::textColourId,          ApolloTheme::textOnPanel);
    setColour (juce::ToggleButton::tickColourId,          ApolloTheme::orange);

    setColour (juce::ComboBox::textColourId,              juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId,        ApolloTheme::graphiteDeep);
    setColour (juce::ComboBox::outlineColourId,           ApolloTheme::graphiteEdge);
    setColour (juce::ComboBox::arrowColourId,             ApolloTheme::orange);

    setColour (juce::Label::textColourId,                 ApolloTheme::textOnPanel);

    setColour (juce::PopupMenu::backgroundColourId,            ApolloTheme::graphiteDark);
    setColour (juce::PopupMenu::textColourId,                  ApolloTheme::textOnPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ApolloTheme::orange);
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::black);

    setColour (juce::TextEditor::backgroundColourId,      ApolloTheme::plateBlack);
    setColour (juce::TextEditor::textColourId,            ApolloTheme::orange);
    setColour (juce::TextEditor::highlightColourId,       ApolloTheme::orange.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId,       ApolloTheme::orange);
}

//==============================================================================
void ApolloLookAndFeel::drawKnobScale (Graphics& g, Point<float> centre, float outerRadius,
                                       float innerRadius, float startAngle, float endAngle,
                                       bool bipolar, bool dim)
{
    constexpr int ticks = 10;

    for (int i = 0; i <= ticks; ++i)
    {
        const float t = (float) i / (float) ticks;
        const float angle = startAngle + t * (endAngle - startAngle);

        const bool major = (i % 5 == 0);
        const bool centreTick = bipolar && i == ticks / 2;

        float inner = innerRadius;
        float outer = outerRadius;
        float thickness = major ? sc (1.7f) : sc (1.0f);

        if (major)
            inner = innerRadius - sc (4.5f);

        if (centreTick)
        {
            inner = innerRadius - sc (7.0f);
            outer = outerRadius + sc (1.0f);
            thickness = sc (1.8f);
        }

        Colour colour = dim ? ApolloTheme::textOnPanelDim.withAlpha (0.35f)
                            : ApolloTheme::textOnPanelDim.withAlpha (major ? 0.9f : 0.55f);

        if (centreTick)
            colour = dim ? ApolloTheme::orangeDeep.withAlpha (0.25f)
                         : ApolloTheme::orange.withAlpha (0.85f);

        const Point<float> p1 (centre.x + std::sin (angle) * inner, centre.y - std::cos (angle) * inner);
        const Point<float> p2 (centre.x + std::sin (angle) * outer, centre.y - std::cos (angle) * outer);

        g.setColour (colour);
        g.drawLine (Line<float> (p1, p2), thickness);
    }
}

void ApolloLookAndFeel::drawVintageKnob (Graphics& g, Point<float> centre, float radius,
                                         float angle, bool dimmed, bool focused, bool active)
{
    juce::ignoreUnused (focused);

    const auto polar = [&centre] (float r, float a)
    {
        return juce::Point<float> (centre.x + std::sin (a) * r, centre.y - std::cos (a) * r);
    };

    const float twoPi = juce::MathConstants<float>::twoPi;

    // Drop shadow on the panel.
    g.setColour (juce::Colours::black.withAlpha (dimmed ? 0.25f : 0.42f));
    g.fillEllipse (centre.x - radius, centre.y - radius + sc (2.5f), radius * 2.0f, radius * 2.0f);

    // Fluted bakelite skirt.
    {
        ColourGradient skirt (ApolloTheme::bakeliteLight, centre.x - radius, centre.y - radius,
                              ApolloTheme::bakeliteDark, centre.x + radius * 0.6f, centre.y + radius, false);
        g.setGradientFill (skirt);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        constexpr int flutes = 22;
        const float grooveInner = radius - juce::jmax (sc (4.5f), radius * 0.20f);
        const float grooveOuter = radius - sc (0.6f);

        for (int i = 0; i < flutes; ++i)
        {
            const float a = (float) i / (float) flutes * twoPi;
            g.setColour (juce::Colours::black.withAlpha (dimmed ? 0.16f : 0.34f));
            g.drawLine (Line<float> (polar (grooveInner, a), polar (grooveOuter, a)), sc (1.9f));

            const float aRidge = a + 0.5f / (float) flutes * twoPi;
            g.setColour (juce::Colours::white.withAlpha (dimmed ? 0.03f : 0.08f));
            g.drawLine (Line<float> (polar (grooveInner + sc (0.5f), aRidge),
                                     polar (grooveOuter, aRidge)), sc (1.0f));
        }

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, sc (1.0f));
    }

    // Domed body.
    const float bodyRadius = radius * 0.74f;
    {
        ColourGradient body (ApolloTheme::bakeliteLight.brighter (0.10f),
                             centre.x - bodyRadius * 0.42f, centre.y - bodyRadius * 0.52f,
                             ApolloTheme::bakeliteDark,
                             centre.x + bodyRadius * 0.55f, centre.y + bodyRadius * 0.65f, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (dimmed ? 0.03f : 0.11f));
        g.fillEllipse (centre.x - bodyRadius * 0.60f, centre.y - bodyRadius * 0.82f,
                       bodyRadius * 1.05f, bodyRadius * 0.66f);

        // Chrome step between the dome and the fluted skirt.
        g.setColour (ApolloTheme::chromeDark.withAlpha (dimmed ? 0.35f : 0.85f));
        g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, sc (1.3f));
        g.setColour (ApolloTheme::chromeLight.withAlpha (dimmed ? 0.10f : 0.30f));
        g.drawEllipse (centre.x - bodyRadius - sc (0.8f), centre.y - bodyRadius - sc (0.8f),
                       (bodyRadius + sc (0.8f)) * 2.0f, (bodyRadius + sc (0.8f)) * 2.0f, sc (0.8f));
    }

    // Ivory pointer running across the dome onto the skirt.
    {
        const Point<float> p1 = polar (radius * 0.20f, angle);
        const Point<float> p2 = polar (radius * 0.90f, angle);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (Line<float> (p1.translated (0.0f, sc (1.0f)), p2.translated (0.0f, sc (1.0f))), sc (2.6f));

        g.setColour (dimmed ? ApolloTheme::textOnPanelDim.withAlpha (0.45f)
                            : (active ? juce::Colours::white : ApolloTheme::knobPointer));
        g.drawLine (Line<float> (p1, p2), sc (2.3f));

        g.setColour (dimmed ? ApolloTheme::textOnPanelDim.withAlpha (0.4f) : ApolloTheme::knobPointer);
        g.fillEllipse (p2.x - sc (1.6f), p2.y - sc (1.6f), sc (3.2f), sc (3.2f));
    }

    // Chrome centre cap.
    {
        const float capRadius = bodyRadius * 0.30f;
        ColourGradient cap (ApolloTheme::chromeLight,
                            centre.x - capRadius * 0.45f, centre.y - capRadius * 0.55f,
                            ApolloTheme::chromeDark,
                            centre.x + capRadius * 0.55f, centre.y + capRadius * 0.60f, true);
        g.setGradientFill (cap);
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        g.setColour (ApolloTheme::chromeDark.withAlpha (0.7f));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, sc (0.8f));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (centre.x - capRadius * 0.45f, centre.y - capRadius * 0.55f,
                       capRadius * 0.6f, capRadius * 0.4f);
    }
}

void ApolloLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, const float rotaryStartAngle,
                                          const float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const Point<float> centre = bounds.getCentre();

    const bool dimmed = isDimmed (slider);
    const bool focused = slider.hasKeyboardFocus (true);
    const bool active = slider.isMouseOverOrDragging();
    const bool bipolar = (bool) slider.getProperties()[ApolloTheme::bipolarProperty];

    const float outerRadius = size * 0.5f;
    const float knobRadius = outerRadius * 0.68f;
    const float scaleInner = knobRadius + sc (4.0f);
    const float scaleOuter = outerRadius - sc (1.5f);

    drawKnobScale (g, centre, scaleOuter, scaleInner, rotaryStartAngle, rotaryEndAngle, bipolar, dimmed);

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    drawVintageKnob (g, centre, knobRadius, angle, dimmed, focused, active);

    // No focus ring is drawn (see drawFocusHalo).
    juce::ignoreUnused (focused);
}

//==============================================================================
void ApolloLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float minSliderPos, float maxSliderPos,
                                          const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    if (style != juce::Slider::LinearVertical && style != juce::Slider::LinearBarVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const bool dimmed = isDimmed (slider);
    const bool focused = slider.hasKeyboardFocus (true);

    const float slotWidth = sc (9.0f);
    const float slotX = bounds.getX() + bounds.getWidth() * 0.30f - slotWidth * 0.5f;
    const float top = bounds.getY() + sc (4.0f);
    const float bottom = bounds.getBottom() - sc (4.0f);
    const float trackHeight = bottom - top;

    ApolloTheme::drawInsetWell (g, { slotX - sc (2.0f), top - sc (2.0f), slotWidth + sc (4.0f), trackHeight + sc (4.0f) },
                                3.0f, 1.0f);

    const float fillTop = juce::jlimit (top, bottom, sliderPos);
    if (bottom - fillTop > 1.0f)
    {
        g.setColour ((dimmed ? ApolloTheme::orangeDeep : ApolloTheme::orange).withAlpha (dimmed ? 0.16f : 0.30f));
        g.fillRoundedRectangle ({ slotX, fillTop, slotWidth, bottom - fillTop }, 2.5f);
    }

    const float scaleX = bounds.getX() + bounds.getWidth() * 0.60f;
    g.setFont (ApolloTheme::valueFont (sc (8.0f)));

    constexpr int tickCount = 4;
    for (int i = 0; i <= tickCount; ++i)
    {
        const float t = (float) i / (float) tickCount;
        const float ty = bottom - t * trackHeight;

        g.setColour ((dimmed ? ApolloTheme::textOnPanelDim.darker (0.4f) : ApolloTheme::textOnPanelDim).withAlpha (0.75f));
        g.drawLine (scaleX, ty, scaleX + sc (6.0f), ty, sc (1.0f));

        g.setColour ((dimmed ? ApolloTheme::textOnPanelDim.darker (0.45f) : ApolloTheme::textOnPanelDim).withAlpha (0.70f));
        g.drawText (juce::String (juce::roundToInt (t * 100.0f)),
                    Rectangle<float> (scaleX + sc (9.0f), ty - sc (7.0f), sc (28.0f), sc (14.0f)),
                    Justification::centredLeft, false);
    }

    const float capWidth = bounds.getWidth() * 0.36f;
    const float capHeight = sc (26.0f);
    const Rectangle<float> cap (slotX + slotWidth * 0.5f - capWidth * 0.5f,
                                sliderPos - capHeight * 0.5f, capWidth, capHeight);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (cap.translated (0.0f, sc (1.5f)), sc (3.0f));

    {
        ColourGradient capGrad (ApolloTheme::metalCap.brighter (0.40f), cap.getX(), cap.getY(),
                                ApolloTheme::metalCap.darker (0.45f), cap.getRight(), cap.getBottom(), false);
        g.setGradientFill (capGrad);
        g.fillRoundedRectangle (cap, sc (3.0f));
    }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (cap, sc (3.0f), sc (1.0f));

    g.setColour (juce::Colours::black.withAlpha (0.30f));
    for (int i = 1; i <= 3; ++i)
    {
        const float yy = cap.getY() + cap.getHeight() * (float) i / 4.0f;
        g.drawLine (cap.getX() + sc (4.0f), yy, cap.getRight() - sc (4.0f), yy, sc (0.8f));
    }

    g.setColour (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::metalHighlight);
    g.drawLine (cap.getX() + sc (4.0f), cap.getCentreY(), cap.getRight() - sc (4.0f), cap.getCentreY(), sc (1.4f));

    if (focused)
        drawFocusHalo (g, bounds, sc (3.0f));
}

//==============================================================================
void ApolloLookAndFeel::drawFocusHalo (Graphics& g, Rectangle<float> bounds, float corner)
{
    // Focus indicators are intentionally not drawn. Controls still receive
    // keyboard focus (navigation/automation keep working), but the orange halo
    // that appeared whenever a control was clicked is suppressed.
    juce::ignoreUnused (g, bounds, corner);
}

void ApolloLookAndFeel::drawRocker (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (highlighted, down);

    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const bool dimmed = isDimmed (button);

    ApolloTheme::drawInsetWell (g, bounds, 4.0f, 0.8f);

    auto paddle = bounds.reduced (2.5f);
    const Colour top = on ? ApolloTheme::orangeDeep : ApolloTheme::graphiteLight;
    const Colour bottom = on ? ApolloTheme::orangeDeep.darker (0.35f) : ApolloTheme::graphiteDeep;

    ColourGradient grad (top, paddle.getX(), paddle.getY(), bottom, paddle.getX(), paddle.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (paddle, 3.0f);

    g.setColour (juce::Colours::white.withAlpha (on ? 0.28f : 0.10f));
    g.drawLine (paddle.getX() + 3.0f, paddle.getY() + 1.0f, paddle.getRight() - 3.0f, paddle.getY() + 1.0f, sc (1.0f));

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (paddle, 3.0f, sc (1.0f));

    g.setColour (on ? (dimmed ? ApolloTheme::textOnPanelDim.darker (0.3f) : juce::Colours::black)
                    : (dimmed ? ApolloTheme::textOnPanelDim.darker (0.45f) : ApolloTheme::textOnPanel));
    g.setFont (ApolloTheme::valueFont (sc (10.0f)));
    g.drawText (on ? "ON" : "OFF", paddle, Justification::centred, false);
}

void ApolloLookAndFeel::drawMomentary (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (highlighted);

    auto bounds = button.getLocalBounds().toFloat();
    const bool active = button.getToggleState() || down;
    const bool dimmed = isDimmed (button);

    ApolloTheme::drawInsetWell (g, bounds, 5.0f, 1.0f);

    auto cap = bounds.reduced (sc (3.0f));
    if (active)
        cap = cap.translated (0.0f, sc (1.5f));

    g.setColour (ApolloTheme::graphiteDeep);
    g.fillRoundedRectangle (cap.translated (0.0f, sc (2.0f)), 5.0f);

    ColourGradient grad (active ? ApolloTheme::orange : ApolloTheme::graphiteLight,
                         cap.getX(), cap.getY(),
                         active ? ApolloTheme::orangeDeep : ApolloTheme::graphiteDeep,
                         cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (cap, 5.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (cap, 5.0f, sc (1.0f));

    auto content = cap.reduced (sc (7.0f), sc (5.0f));
    const float lampSize = content.getHeight();
    auto lampArea = content.removeFromLeft (lampSize);
    content.removeFromLeft (sc (8.0f));

    ApolloTheme::drawLamp (g, lampArea, active ? ApolloTheme::orange : ApolloTheme::lampOff, active ? 1.0f : 0.0f, 2.5f);

    g.setColour (active ? juce::Colours::black : (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::textOnPanel));
    g.setFont (ApolloTheme::headingFont (sc (13.0f)));
    g.drawText (button.getButtonText(), content, Justification::centred, false);

    if (active)
    {
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.setFont (ApolloTheme::valueFont (sc (8.0f)));
        g.drawText ("HOLD", content, Justification::bottomRight, false);
    }
}

void ApolloLookAndFeel::drawBypass (Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (highlighted, down);

    auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();
    const bool dimmed = isDimmed (button);

    ApolloTheme::drawInsetWell (g, bounds, 5.0f, 1.0f);

    auto cap = bounds.reduced (sc (3.0f));

    g.setColour (on ? ApolloTheme::redDeep.darker (0.35f) : ApolloTheme::graphiteDeep);
    g.fillRoundedRectangle (cap.translated (0.0f, sc (2.0f)), 5.0f);

    ColourGradient grad (on ? ApolloTheme::red : ApolloTheme::graphiteLight,
                         cap.getX(), cap.getY(),
                         on ? ApolloTheme::redDeep : ApolloTheme::graphiteDeep,
                         cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (cap, 5.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (cap, 5.0f, sc (1.0f));

    auto content = cap.reduced (sc (8.0f), sc (5.0f));
    const float lampSize = content.getHeight();
    auto lampArea = content.removeFromLeft (lampSize);
    content.removeFromLeft (sc (9.0f));

    ApolloTheme::drawLamp (g, lampArea, on ? ApolloTheme::red : ApolloTheme::orange,
                           on ? 1.0f : (dimmed ? 0.0f : 0.55f), 2.5f);

    g.setColour (on ? juce::Colours::white : (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::textOnPanel));
    g.setFont (ApolloTheme::headingFont (sc (14.0f)));
    g.drawText ("BYPASS", content, Justification::centred, false);

    g.setColour (on ? juce::Colours::white.withAlpha (0.85f) : ApolloTheme::textOnPanelDim);
    g.setFont (ApolloTheme::valueFont (sc (8.0f)));
    g.drawText (on ? "BYPASSED" : "ACTIVE", content, Justification::bottomRight, false);
}

void ApolloLookAndFeel::drawToggleButton (Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto style = (ApolloTheme::ButtonStyle) (int) button.getProperties()[ApolloTheme::styleProperty];

    switch (style)
    {
        case ApolloTheme::ButtonStyle::Rocker:
            drawRocker (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            break;

        case ApolloTheme::ButtonStyle::Momentary:
            drawMomentary (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            break;

        case ApolloTheme::ButtonStyle::Bypass:
            drawBypass (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            break;

        case ApolloTheme::ButtonStyle::Plain:
        default:
            LookAndFeel_V4::drawToggleButton (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
    }

    if (button.hasKeyboardFocus (true))
        drawFocusHalo (g, button.getLocalBounds().toFloat().reduced (1.0f), sc (3.0f));
}

//==============================================================================
void ApolloLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH);

    const auto bounds = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    const bool dimmed = isDimmed (box);
    const bool vertical = (bool) box.getProperties()[ApolloTheme::verticalProperty];
    const int itemCount = box.getNumItems();
    const int selected = box.getSelectedItemIndex();

    if ((bool) box.getProperties()[ApolloTheme::pushBankProperty])
    {
        drawPushBank (g, bounds, box, dimmed);

        if (box.hasKeyboardFocus (true))
            drawFocusHalo (g, bounds, 4.0f);

        return;
    }

    ApolloTheme::drawInsetWell (g, bounds, 4.0f, 0.8f);

    auto inner = bounds.reduced (2.0f);

    const Colour activeFill = (dimmed ? ApolloTheme::orangeDeep : ApolloTheme::orange).withAlpha (dimmed ? 0.32f : 0.92f);
    const Colour activeText = dimmed ? ApolloTheme::textOnPanelDim.darker (0.30f) : juce::Colours::black;
    const Colour idleText   = dimmed ? ApolloTheme::textOnPanelDim.darker (0.45f) : ApolloTheme::textOnPanel;

    if (itemCount > 0)
    {
        if (vertical)
        {
            const float usableHeight = inner.getHeight() - sc (10.0f);
            const float segmentHeight = usableHeight / (float) itemCount;

            for (int i = 0; i < itemCount; ++i)
            {
                Rectangle<float> segment (inner.getX(), inner.getY() + (float) i * segmentHeight,
                                          inner.getWidth(), segmentHeight - sc (1.0f));
                const bool on = (i == selected);

                if (on)
                {
                    g.setColour (activeFill);
                    g.fillRoundedRectangle (segment.reduced (1.0f), 3.0f);
                }

                g.setColour (on ? activeText : idleText);
                g.setFont (ApolloTheme::valueFont (sc (8.5f)));
                g.drawText (box.getItemText (i), segment.reduced (1.0f), Justification::centred, false);
            }

            if (itemCount > 1)
            {
                g.setColour (juce::Colours::black.withAlpha (0.5f));
                for (int i = 1; i < itemCount; ++i)
                {
                    const float yy = inner.getY() + (float) i * segmentHeight;
                    g.drawLine (inner.getX() + 2.0f, yy, inner.getRight() - 2.0f, yy, sc (1.0f));
                }
            }

            Path arrow;
            const float ax = inner.getRight() - sc (7.0f);
            const float ay = inner.getBottom() - sc (5.0f);
            arrow.addTriangle (ax - sc (3.0f), ay - sc (2.0f), ax + sc (3.0f), ay - sc (2.0f), ax, ay + sc (2.0f));
            g.setColour (idleText);
            g.fillPath (arrow);

            g.setColour (ApolloTheme::graphiteEdge);
            g.drawLine (inner.getX(), inner.getBottom() - sc (10.0f), inner.getRight(), inner.getBottom() - sc (10.0f), sc (1.0f));
        }
        else
        {
            const float usableWidth = inner.getWidth() - sc (16.0f);
            const float segmentWidth = usableWidth / (float) itemCount;

            for (int i = 0; i < itemCount; ++i)
            {
                Rectangle<float> segment (inner.getX() + (float) i * segmentWidth, inner.getY(),
                                          segmentWidth - sc (1.0f), inner.getHeight());
                const bool on = (i == selected);

                if (on)
                {
                    g.setColour (activeFill);
                    g.fillRoundedRectangle (segment.reduced (1.0f), 3.0f);
                }

                g.setColour (on ? activeText : idleText);
                g.setFont (ApolloTheme::valueFont (sc (8.5f)));
                g.drawText (box.getItemText (i), segment.reduced (1.0f), Justification::centred, false);
            }

            if (itemCount > 1)
            {
                g.setColour (juce::Colours::black.withAlpha (0.5f));
                for (int i = 1; i < itemCount; ++i)
                {
                    const float xx = inner.getX() + (float) i * segmentWidth;
                    g.drawLine (xx, inner.getY() + 2.0f, xx, inner.getBottom() - 2.0f, sc (1.0f));
                }
            }

            Path arrow;
            const float ax = inner.getRight() - sc (7.0f);
            const float ay = inner.getCentreY();
            arrow.addTriangle (ax - sc (3.0f), ay - sc (2.0f), ax + sc (3.0f), ay - sc (2.0f), ax, ay + sc (2.0f));
            g.setColour (idleText);
            g.fillPath (arrow);

            g.setColour (ApolloTheme::graphiteEdge);
            g.drawLine (inner.getRight() - sc (15.0f), inner.getY() + 2.0f,
                        inner.getRight() - sc (15.0f), inner.getBottom() - 2.0f, sc (1.0f));
        }
    }

    if (box.hasKeyboardFocus (true))
        drawFocusHalo (g, bounds, 4.0f);
}

void ApolloLookAndFeel::drawPushBank (Graphics& g, Rectangle<float> bounds, juce::ComboBox& box, bool dimmed)
{
    const int itemCount = box.getNumItems();
    if (itemCount <= 0)
        return;

    const int selected = box.getSelectedItemIndex();
    const bool vertical = (bool) box.getProperties()[ApolloTheme::verticalProperty];
    const float gap = sc (4.0f);

    float buttonWidth = bounds.getWidth();
    float buttonHeight = bounds.getHeight();

    if (vertical)
        buttonHeight = (bounds.getHeight() - gap * (float) (itemCount - 1)) / (float) itemCount;
    else
        buttonWidth = (bounds.getWidth() - gap * (float) (itemCount - 1)) / (float) itemCount;

    for (int i = 0; i < itemCount; ++i)
    {
        Rectangle<float> button;

        if (vertical)
            button = { bounds.getX(), bounds.getY() + (float) i * (buttonHeight + gap), buttonWidth, buttonHeight };
        else
            button = { bounds.getX() + (float) i * (buttonWidth + gap), bounds.getY(), buttonWidth, buttonHeight };

        const bool on = (i == selected);

        if (on)
        {
            // Interlocked mechanical key: the selected position stays pushed down.
            ApolloTheme::drawInsetWell (g, button, 4.0f, 1.0f);

            auto face = button.reduced (sc (2.5f));
            const auto fill = dimmed ? ApolloTheme::orangeDeep.withAlpha (0.35f)
                                     : ApolloTheme::orange.withAlpha (0.92f);
            g.setColour (fill);
            g.fillRoundedRectangle (face, 3.0f);

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawLine (face.getX() + 3.0f, face.getY() + 1.0f, face.getRight() - 3.0f, face.getY() + 1.0f, sc (1.4f));

            g.setColour (dimmed ? ApolloTheme::textOnPanelDim.darker (0.30f) : juce::Colours::black);
        }
        else
        {
            // Raised key, catching the light from the top-left.
            ColourGradient cap (ApolloTheme::graphiteLight, button.getX(), button.getY(),
                                ApolloTheme::graphiteDeep, button.getX(), button.getBottom(), false);
            g.setGradientFill (cap);
            g.fillRoundedRectangle (button, 4.0f);

            g.setColour (juce::Colours::white.withAlpha (dimmed ? 0.04f : 0.13f));
            g.drawLine (button.getX() + 3.0f, button.getY() + 1.0f, button.getRight() - 3.0f, button.getY() + 1.0f, sc (1.0f));

            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawLine (button.getX() + 3.0f, button.getBottom() - 1.0f, button.getRight() - 3.0f, button.getBottom() - 1.0f, sc (1.6f));

            g.setColour (ApolloTheme::graphiteEdge);
            g.drawRoundedRectangle (button.reduced (0.5f), 4.0f, sc (1.0f));

            g.setColour (dimmed ? ApolloTheme::textOnPanelDim.darker (0.35f) : ApolloTheme::textOnPanel);
        }

        g.setFont (ApolloTheme::valueFont (sc (9.0f)));
        g.drawText (box.getItemText (i), button.reduced (2.0f), Justification::centred, false);
    }
}

//==============================================================================
void ApolloLookAndFeel::drawLabel (Graphics& g, juce::Label& label)
{
    const bool display = (bool) label.getProperties()[ApolloTheme::displayProperty];
    const bool caption = (bool) label.getProperties()[ApolloTheme::captionProperty];

    if (! display && ! caption)
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }

    const bool dimmed = isDimmed (label);
    const auto bounds = label.getLocalBounds().toFloat();

    if (display)
    {
        ApolloTheme::drawInsetWell (g, bounds, 3.0f, 0.7f);
        g.setColour (dimmed ? ApolloTheme::orangeDeep.withAlpha (0.45f) : ApolloTheme::orange);
        g.setFont (label.getFont());
        g.drawText (label.getText(), bounds.reduced (4.0f, 1.0f), label.getJustificationType(), false);
        return;
    }

    g.setColour (dimmed ? ApolloTheme::textOnPanelDim.darker (0.35f)
                        : label.findColour (juce::Label::textColourId));
    g.setFont (label.getFont());
    g.drawText (label.getText(), bounds, label.getJustificationType(), false);
}

//==============================================================================
void ApolloLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    g.fillAll (ApolloTheme::graphiteDark);
    g.setColour (ApolloTheme::graphiteEdge);
    g.drawRect (0, 0, width, height, 1);
}

void ApolloLookAndFeel::drawPopupMenuItem (Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                           bool hasSubMenu, const juce::String& text,
                                           const juce::String& shortcutKeyText,
                                           const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused (hasSubMenu, shortcutKeyText, icon);

    if (isSeparator)
    {
        auto r = area.reduced (5, 0);
        r.removeFromTop (juce::roundToInt ((float) r.getHeight() * 0.5f - 0.5f));
        g.setColour (ApolloTheme::graphiteEdge);
        g.fillRect (r.removeFromTop (1));
        return;
    }

    auto r = area.toFloat().reduced (2.0f);

    if (isHighlighted && isActive)
    {
        g.setColour (ApolloTheme::orange.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 3.0f);
    }

    const auto colour = ! isActive ? ApolloTheme::textOnPanelDim
                                   : (isHighlighted ? juce::Colours::black
                                                    : (textColour != nullptr ? *textColour : ApolloTheme::textOnPanel));

    g.setColour (colour);
    g.setFont (ApolloTheme::valueFont (sc (12.0f)));
    g.drawText (text, r.reduced (8.0f, 0.0f), Justification::centredLeft, false);

    if (isTicked)
    {
        g.setColour (isHighlighted && isActive ? juce::Colours::black : ApolloTheme::orange);
        g.setFont (ApolloTheme::labelFont (sc (12.0f)));
        g.drawText ("\u2022", r.removeFromLeft (14.0f), Justification::centred, false);
    }
}
