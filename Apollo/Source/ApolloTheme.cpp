#include "ApolloTheme.h"

#include <cmath>

namespace ApolloTheme
{
    juce::Font headingFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), size, juce::Font::bold))
                   .withExtraKerningFactor (0.14f);
    }

    juce::Font labelFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), size, juce::Font::plain))
                   .withExtraKerningFactor (0.06f);
    }

    juce::Font valueFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    }

    void drawRaisedPlate (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, bool dimmed)
    {
        const auto base = dimmed ? chassisMid.darker (0.32f) : chassisLight;

        juce::ColourGradient grad (base.brighter (0.06f), bounds.getX(), bounds.getY(),
                                   base.darker (0.20f), bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, corner);

        g.setColour (juce::Colours::white.withAlpha (dimmed ? 0.03f : 0.12f));
        g.drawLine (bounds.getX() + corner, bounds.getY() + 0.75f,
                    bounds.getRight() - corner, bounds.getY() + 0.75f, 1.0f);

        g.setColour (chassisShadow.withAlpha (dimmed ? 0.14f : 0.32f));
        g.drawLine (bounds.getX() + corner, bounds.getBottom() - 0.75f,
                    bounds.getRight() - corner, bounds.getBottom() - 0.75f, 1.6f);

        g.setColour (chassisEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
    }

    void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float depth)
    {
        juce::ColourGradient grad (graphiteDeep, bounds.getX(), bounds.getY(),
                                   graphite, bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, corner);

        g.setColour (juce::Colours::black.withAlpha (0.45f * juce::jlimit (0.0f, 1.0f, depth)));
        g.drawLine (bounds.getX() + corner, bounds.getY() + 1.0f,
                    bounds.getRight() - corner, bounds.getY() + 1.0f, 1.6f);

        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
    }

    void drawBrushedMetal (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour base)
    {
        juce::ColourGradient grad (base.brighter (0.05f), bounds.getX(), bounds.getY(),
                                   base.darker (0.12f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (bounds);

        const int y0 = (int) bounds.getY();
        const int y1 = (int) bounds.getBottom();
        for (int y = y0; y < y1; ++y)
        {
            const float n = std::sin ((float) y * 12.9898f + 4.1414f) * 43758.5453f;
            const float f = (n - std::floor (n)) - 0.5f;
            g.setColour (f > 0.0f ? juce::Colours::white.withAlpha (0.018f)
                                  : juce::Colours::black.withAlpha (0.022f));
            g.drawHorizontalLine (y, bounds.getX(), bounds.getRight());
        }
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
    {
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.fillEllipse (centre.x - radius - 1.0f, centre.y - radius - 1.0f,
                       radius * 2.0f + 2.0f, radius * 2.0f + 2.0f);

        juce::ColourGradient grad (metalScrew.brighter (0.28f), centre.x - radius * 0.4f, centre.y - radius * 0.7f,
                                   metalScrew.darker (0.32f), centre.x + radius * 0.4f, centre.y + radius * 0.7f, true);
        g.setGradientFill (grad);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour (metalHighlight.withAlpha (0.55f));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 0.8f);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (centre.x - radius * 0.6f, centre.y - radius * 0.6f,
                    centre.x + radius * 0.6f, centre.y + radius * 0.6f,
                    juce::jmax (1.0f, radius * 0.3f));
    }

    void drawEngravedText (juce::Graphics& g, const juce::String& textText, juce::Rectangle<float> area,
                           const juce::Font& font, juce::Justification justification,
                           juce::Colour faceColour, juce::Colour highlightColour)
    {
        g.setFont (font);
        g.setColour (highlightColour);
        g.drawText (textText, area.translated (0.0f, 1.0f), justification, false);
        g.setColour (faceColour);
        g.drawText (textText, area, justification, false);
    }

    void drawLamp (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour,
                   float intensity, float corner)
    {
        const float amount = juce::jlimit (0.0f, 1.0f, intensity);
        drawInsetWell (g, area, corner, 0.55f);

        auto glass = area.reduced (juce::jmax (1.0f, area.getWidth() * 0.10f));

        if (amount > 0.02f)
        {
            const auto lit = colour.withMultipliedBrightness (0.65f + 0.55f * amount);

            g.setColour (lit.withAlpha (0.20f * amount));
            g.fillRoundedRectangle (glass.expanded (2.0f), corner + 1.5f);

            g.setColour (lit.withAlpha (0.35f + 0.65f * amount));
            g.fillRoundedRectangle (glass, juce::jmax (1.0f, corner - 0.5f));

            auto reflection = glass.removeFromTop (glass.getHeight() * 0.42f);
            g.setColour (juce::Colours::white.withAlpha (0.20f * amount));
            g.fillRoundedRectangle (reflection.reduced (1.0f), juce::jmax (1.0f, corner - 1.0f));
        }
        else
        {
            g.setColour (lampOff);
            g.fillRoundedRectangle (glass, juce::jmax (1.0f, corner - 0.5f));

            g.setColour (juce::Colours::white.withAlpha (0.05f));
            auto reflection = glass.removeFromTop (glass.getHeight() * 0.42f);
            g.fillRoundedRectangle (reflection.reduced (1.0f), juce::jmax (1.0f, corner - 1.0f));
        }
    }
}

//==============================================================================
ApolloRackPanel::ApolloRackPanel (juce::String sectionTitle, juce::String sectionSubtitle)
    : title (std::move (sectionTitle)), subtitle (std::move (sectionSubtitle))
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
}

void ApolloRackPanel::setLampActive (bool shouldBeActive)
{
    if (lampActive != shouldBeActive)
    {
        lampActive = shouldBeActive;
        repaint();
    }
}

void ApolloRackPanel::setLampColour (juce::Colour colour)
{
    lampColour = colour;
    repaint();
}

void ApolloRackPanel::setDimmed (bool shouldBeDimmed)
{
    if (dimmed != shouldBeDimmed)
    {
        dimmed = shouldBeDimmed;
        repaint();
    }
}

void ApolloRackPanel::setSubtitle (const juce::String& newSubtitle)
{
    subtitle = newSubtitle;
    repaint();
}

void ApolloRackPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    const float corner = 5.0f;
    const auto base = dimmed ? juce::Colour (0xff1b1d21) : ApolloTheme::graphite;

    juce::ColourGradient grad (base.brighter (0.20f), b.getX(), b.getY(),
                               base.darker (0.38f), b.getRight(), b.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (b, corner);

    g.setColour (juce::Colours::white.withAlpha (dimmed ? 0.02f : 0.08f));
    g.drawLine (b.getX() + 5.0f, b.getY() + 1.0f, b.getRight() - 5.0f, b.getY() + 1.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (b.getX() + 5.0f, b.getBottom() - 1.0f, b.getRight() - 5.0f, b.getBottom() - 1.0f, 1.6f);

    g.setColour (ApolloTheme::graphiteEdge);
    g.drawRoundedRectangle (b, corner, 1.2f);

    const float screwRadius = juce::jlimit (2.4f, 3.6f, b.getHeight() * 0.018f);
    const float inset = 9.0f;
    ApolloTheme::drawScrew (g, { b.getX() + inset, b.getY() + inset }, screwRadius);
    ApolloTheme::drawScrew (g, { b.getRight() - inset, b.getY() + inset }, screwRadius);
    ApolloTheme::drawScrew (g, { b.getX() + inset, b.getBottom() - inset }, screwRadius);
    ApolloTheme::drawScrew (g, { b.getRight() - inset, b.getBottom() - inset }, screwRadius);

    auto strip = b.reduced (16.0f, 7.0f).removeFromTop (juce::jmax (16.0f, b.getHeight() * 0.11f));

    const float lampSize = juce::jmin (16.0f, strip.getHeight());
    auto lampArea = strip.removeFromRight (lampSize);
    lampArea = lampArea.withSizeKeepingCentre (lampSize, lampSize);
    strip.removeFromRight (10.0f);

    const auto titleColour = dimmed ? ApolloTheme::textOnPanelDim.darker (0.35f)
                                    : ApolloTheme::textOnPanel;
    ApolloTheme::drawEngravedText (g, title, strip, ApolloTheme::headingFont (juce::jlimit (10.0f, 14.0f, b.getHeight() * 0.045f)),
                                   juce::Justification::centredLeft,
                                   titleColour, juce::Colours::black.withAlpha (0.65f));

    if (subtitle.isNotEmpty())
        ApolloTheme::drawEngravedText (g, subtitle, strip, ApolloTheme::labelFont (juce::jlimit (7.0f, 9.5f, b.getHeight() * 0.030f)),
                                       juce::Justification::centredRight,
                                       dimmed ? ApolloTheme::textOnPanelDim.darker (0.4f) : ApolloTheme::textOnPanelDim,
                                       juce::Colours::black.withAlpha (0.5f));

    ApolloTheme::drawLamp (g, lampArea, lampColour, lampActive ? 1.0f : 0.0f, 2.5f);
}

//==============================================================================
ApolloAnnunciator::ApolloAnnunciator()
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
}

void ApolloAnnunciator::setText (const juce::String& newText)
{
    if (text != newText)
    {
        text = newText;
        repaint();
    }
}

void ApolloAnnunciator::setLamp (bool isLit, juce::Colour colour)
{
    lampOn = isLit;
    lampColour = colour;
    repaint();
}

void ApolloAnnunciator::setDimmed (bool shouldBeDimmed)
{
    dimmed = shouldBeDimmed;
    repaint();
}

void ApolloAnnunciator::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    ApolloTheme::drawInsetWell (g, b, 3.0f, 0.9f);

    auto inner = b.reduced (5.0f, 4.0f);
    const float lampSize = juce::jmin (inner.getHeight(), 12.0f);
    auto lampArea = inner.removeFromLeft (lampSize);
    inner.removeFromLeft (6.0f);

    ApolloTheme::drawLamp (g, lampArea, lampColour, lampOn ? 1.0f : 0.0f, 2.0f);

    auto colour = lampOn ? lampColour.brighter (0.15f)
                         : (dimmed ? ApolloTheme::textOnPanelDim.darker (0.35f)
                                   : ApolloTheme::textOnPanelDim);
    g.setColour (colour);
    g.setFont (ApolloTheme::labelFont (juce::jlimit (8.0f, 12.0f, b.getHeight() * 0.44f)));
    g.drawText (text, inner, juce::Justification::centredLeft, false);
}

//==============================================================================
ApolloStatusLamp::ApolloStatusLamp()
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
}

void ApolloStatusLamp::setState (bool isLit, juce::Colour colour)
{
    lit = isLit;
    lampColour = colour;
    repaint();
}

void ApolloStatusLamp::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float size = juce::jmin (b.getWidth(), b.getHeight());
    ApolloTheme::drawLamp (g, b.withSizeKeepingCentre (size, size), lampColour, lit ? 1.0f : 0.0f, 3.0f);
}
