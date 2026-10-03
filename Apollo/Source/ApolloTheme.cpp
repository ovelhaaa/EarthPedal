#include "ApolloTheme.h"
#include <BinaryData.h>
namespace ApolloTheme
{
    namespace
    {
        bool isSfntFontData (const void* data, int size)
        {
            if (data == nullptr || size < 4)
                return false;

            const auto* bytes = static_cast<const unsigned char*> (data);
            const bool trueType = bytes[0] == 0x00 && bytes[1] == 0x01
                               && bytes[2] == 0x00 && bytes[3] == 0x00;
            const bool openType = bytes[0] == 'O' && bytes[1] == 'T'
                               && bytes[2] == 'T' && bytes[3] == 'O';
            return trueType || openType;
        }

        juce::Typeface::Ptr loadTypeface (const void* data, int size)
        {
            // ApolloFontData contains raw upstream TTF bytes. Reject textual
            // Base64 (including wrapped Base64) rather than silently handing
            // invalid data to the platform font loader.
            if (! isSfntFontData (data, size))
                return {};
            return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
        }
    }
    juce::Typeface::Ptr embeddedTypeface (FontWeight weight)
    {
        static const auto regular = loadTypeface (ApolloFontData::MontserratRegular_ttf,
                                                   ApolloFontData::MontserratRegular_ttfSize);
        static const auto medium = loadTypeface (ApolloFontData::MontserratMedium_ttf,
                                                  ApolloFontData::MontserratMedium_ttfSize);
        static const auto semiBold = loadTypeface (ApolloFontData::MontserratSemiBold_ttf,
                                                    ApolloFontData::MontserratSemiBold_ttfSize);
        static const auto bold = loadTypeface (ApolloFontData::MontserratBold_ttf,
                                                ApolloFontData::MontserratBold_ttfSize);
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
        if (const auto typeface = embeddedTypeface (weight))
            return juce::Font (juce::FontOptions (typeface)).withHeight (size)
                .withExtraKerningFactor (tracking);

        jassertfalse;
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), size,
                                               juce::Font::plain))
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
    juce::String fontFamily()
    {
        if (const auto typeface = embeddedTypeface (FontWeight::Regular))
            return typeface->getName();
        return {};
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
    g.setFont (ApolloTheme::headingFont (ApolloTheme::Metrics::sectionHeading * s));
    g.drawText (title, strip, juce::Justification::centredLeft, false);
}
