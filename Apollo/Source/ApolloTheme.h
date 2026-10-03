#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Vector instrument surfaces on a single ivory chassis.
namespace ApolloTheme
{
    namespace Metrics
    {
        inline constexpr float sectionHeading = 11.0f;
        inline constexpr float controlLabel = 9.5f;
        inline constexpr float controlValue = 9.5f;
        inline constexpr float microLabel = 7.0f;
        inline constexpr float cornerRadius = 2.0f;
        inline constexpr float hairline = 0.8f;
    }
    inline const juce::Colour chassisMid        { 0xffeee9dd };
    inline const juce::Colour chassisEdge       { 0xffb9b4a8 };
    inline const juce::Colour graphiteLight     { 0xff343638 };
    inline const juce::Colour graphite          { 0xff222426 };
    inline const juce::Colour graphiteDark      { 0xff191b1d };
    inline const juce::Colour graphiteDeep      { 0xff101214 };
    inline const juce::Colour graphiteEdge      { 0xff4b4c4b };
    inline const juce::Colour plateBlack        { 0xff101214 };
    inline const juce::Colour textOnPanel       { 0xffeee9dd };
    inline const juce::Colour textOnPanelDim    { 0xffb3afa5 };
    inline const juce::Colour textOnChassis     { 0xff252729 };
    inline const juce::Colour textOnChassisSoft { 0xff696960 };
    inline const juce::Colour orange            { 0xffefa34b };
    inline const juce::Colour orangeDeep        { 0xffbd7d35 };
    inline const juce::Colour red               { 0xffdc6554 };
    inline const juce::Colour cyan              { 0xff85bec5 };
    inline const juce::Colour lampOff           { 0xff474640 };
    inline const juce::Colour knobPointer       { 0xffeee9dd };
    inline constexpr int designWidth = 900, designHeight = 620;
    inline const char* const styleProperty    = "apolloStyle";
    inline const char* const dimProperty      = "apolloDim";
    inline const char* const displayProperty  = "apolloDisplay";
    inline const char* const captionProperty  = "apolloCaption";
    inline const char* const verticalProperty = "apolloVertical";
    inline const char* const pushBankProperty = "apolloPushBank";
    inline const char* const bipolarProperty  = "apolloBipolar";
    inline const char* const feedProperty     = "apolloFeed";
    enum class ButtonStyle { Rocker = 0, Momentary, Bypass, Plain };
    enum class FontWeight { Regular = 0, Medium, SemiBold, Bold };
    juce::Font font (float size, FontWeight weight, float tracking = 0.0f);
    juce::Font headingFont (float size);
    juce::Font labelFont (float size);
    juce::Font valueFont (float size);
    juce::String fontFamily();
    juce::Typeface::Ptr embeddedTypeface (FontWeight weight);
    void drawInsetWell (juce::Graphics&, juce::Rectangle<float>, float corner, float depth = 1.0f);
}

// Transparent functional regions on the editor's shared instrument surface.
class ApolloRackPanel : public juce::Component
{
public:
    explicit ApolloRackPanel (juce::String title);
    void setDimmed (bool);
    void paint (juce::Graphics&) override;
private:
    juce::String title;
    bool dimmed = false;
};

// Retains the stock ComboBox attachment, keyboard and accessibility contract.
class ApolloSelector : public juce::ComboBox
{
public:
    void mouseDown (const juce::MouseEvent& event) override
    {
        if (! isEnabled() || ! event.mods.isLeftButtonDown() || getNumItems() == 0)
            return;
        grabKeyboardFocus();
        const bool vertical = (bool) getProperties()[ApolloTheme::verticalProperty];
        const int position = vertical ? event.y : event.x;
        const int extent = vertical ? getHeight() : getWidth();
        setSelectedItemIndex (juce::jlimit (0, getNumItems() - 1,
                              position * getNumItems() / juce::jmax (1, extent)), juce::sendNotificationSync);
    }
    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (isEnabled())
            setSelectedItemIndex (defaultIndex, juce::sendNotificationSync);
    }
    void setDefaultIndex (int index) { defaultIndex = index; }
private:
    int defaultIndex = 0;
};
