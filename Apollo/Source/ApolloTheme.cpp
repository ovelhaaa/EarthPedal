#include "ApolloTheme.h"
namespace ApolloTheme
{
    namespace
    {
        constexpr auto montserratFamily = "Montserrat";

        juce::String styleName (FontWeight weight)
        {
            switch (weight)
            {
                case FontWeight::Medium: return "Medium";
                case FontWeight::SemiBold: return "SemiBold";
                case FontWeight::Bold: return "Bold";
                default: return "Regular";
            }
        }
    }
    juce::Font font (float size, FontWeight weight, float tracking)
    {
        return juce::Font (juce::FontOptions (montserratFamily, size, styleName (weight)))
            .withExtraKerningFactor (tracking);
    }
    juce::Font headingFont (float size)
    {
        return font (size, FontWeight::SemiBold, 0.105f);
    }
    juce::Font labelFont (float size)
    {
        return font (size, FontWeight::Medium, 0.055f);
    }
    juce::Font valueFont (float size)
    {
        return font (size, FontWeight::Regular, 0.015f);
    }
    juce::String fontFamily() { return montserratFamily; }
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
    g.setFont (ApolloTheme::headingFont (ApolloTheme::Metrics::sectionHeading * s));
    g.drawText (title, strip, juce::Justification::centredLeft, false);
}
