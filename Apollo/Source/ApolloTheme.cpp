#include "ApolloTheme.h"

namespace ApolloTheme
{
    juce::Font headingFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), size, juce::Font::bold))
            .withExtraKerningFactor (0.12f);
    }
    juce::Font labelFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), size, juce::Font::plain))
            .withExtraKerningFactor (0.08f);
    }
    juce::Font valueFont (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
    }
    void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float depth)
    {
        juce::ignoreUnused (depth);
        g.setColour (graphiteDeep);
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (graphiteEdge.withAlpha (0.5f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
    }
}

ApolloRackPanel::ApolloRackPanel (juce::String sectionTitle) : title (std::move (sectionTitle))
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
    setAccessible (false);
}
void ApolloRackPanel::setDimmed (bool value)
{
    if (dimmed != value) { dimmed = value; repaint(); }
}
void ApolloRackPanel::paint (juce::Graphics& g)
{
    const float s = (float) getParentWidth() / ApolloTheme::designWidth;
    auto strip = getLocalBounds().toFloat().reduced (24.0f * s, 14.0f * s).removeFromTop (22.0f * s);
    g.setColour (dimmed ? ApolloTheme::textOnPanelDim : ApolloTheme::textOnPanel);
    g.setFont (ApolloTheme::headingFont (12.0f * s));
    g.drawText (title, strip, juce::Justification::centredLeft, false);
}
