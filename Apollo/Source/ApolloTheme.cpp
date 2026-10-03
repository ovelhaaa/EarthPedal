#include "ApolloTheme.h"
#include <BinaryData.h>
namespace ApolloTheme
{
    namespace
    {
        juce::Typeface::Ptr loadTypeface (const char* encodedData, int encodedSize)
        {
            jassert (encodedData != nullptr && encodedSize > 0);
            juce::MemoryOutputStream decoded;
            const auto encoded = juce::String::fromUTF8 (encodedData, encodedSize);
            if (! juce::Base64::convertFromBase64 (decoded, encoded))
            {
                jassertfalse;
                return {};
            }
            return juce::Typeface::createSystemTypefaceFor (decoded.getData(), decoded.getDataSize());
        }
    }
    juce::Typeface::Ptr embeddedTypeface (FontWeight weight)
    {
        static const auto regular = loadTypeface (ApolloFontData::MontserratRegular_b64,
                                                   ApolloFontData::MontserratRegular_b64Size);
        static const auto medium = loadTypeface (ApolloFontData::MontserratMedium_b64,
                                                  ApolloFontData::MontserratMedium_b64Size);
        static const auto semiBold = loadTypeface (ApolloFontData::MontserratSemiBold_b64,
                                                    ApolloFontData::MontserratSemiBold_b64Size);
        static const auto bold = loadTypeface (ApolloFontData::MontserratBold_b64,
                                                ApolloFontData::MontserratBold_b64Size);
        switch (weight)
        {
            case FontWeight::Medium: return medium;
            case FontWeight::SemiBold: return semiBold;
            case FontWeight::Bold: return bold;
            default: return regular;
        }
    }
    juce::Font font (float size, FontWeight weight, float tracking)
    {
        return juce::Font (juce::FontOptions (embeddedTypeface (weight))).withHeight (size)
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
    juce::String fontFamily() { return embeddedTypeface (FontWeight::Regular)->getName(); }
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
