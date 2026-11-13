#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Vector instrument surfaces on a single ivory chassis.
namespace ApolloTheme
{
    namespace Metrics
    {
        inline constexpr float sectionHeading = 11.0f;
        inline constexpr float controlLabel = 11.5f;
        inline constexpr float controlValue = 12.0f;
    }
    inline const juce::Colour chassis           { 0xfff0ede6 };
    inline const juce::Colour chassisMid        { 0xffe6e2d8 };
    inline const juce::Colour chassisEdge       { 0xffb9b7b0 };
    inline const juce::Colour graphiteLight     { 0xff333639 };
    inline const juce::Colour graphite          { 0xff292b2d };
    inline const juce::Colour graphiteDeep      { 0xff17191b };
    inline const juce::Colour graphiteEdge      { 0xff55585a };
    inline const juce::Colour textOnPanel       { 0xfff3f0e9 };
    inline const juce::Colour textOnPanelDim    { 0xffb8b8b6 };
    inline const juce::Colour textOnChassis     { 0xff252729 };
    inline const juce::Colour textOnChassisSoft { 0xff56585a };
    inline const juce::Colour orange            { 0xfff36b2b };
    inline const juce::Colour orangeBright      { 0xfff6824b };
    inline const juce::Colour logoLensRim       { 0xff561e1a };
    inline const juce::Colour logoLensRed       { 0xffdf301f };
    inline const juce::Colour logoLensCore      { 0xffffa066 };
    inline constexpr int designWidth = 900, designHeight = 620;
    inline const char* const styleProperty    = "apolloStyle";
    inline const char* const dimProperty      = "apolloDim";
    inline const char* const captionProperty  = "apolloCaption";
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
    void drawInsetWell (juce::Graphics&, juce::Rectangle<float>, float corner);
}

// Section background and title behind the attached controls.
class ApolloRackPanel : public juce::Component
{
public:
    explicit ApolloRackPanel (juce::String title);
    void setDimmed (bool);
    void setScale (float value) { scale = value; }
    void paint (juce::Graphics&) override;
private:
    juce::String title;
    bool dimmed = false;
    float scale = 1.0f;
};

// Retains the stock ComboBox attachment, keyboard and accessibility contract.
class ApolloSelector : public juce::ComboBox
{
public:
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDown (const juce::MouseEvent& event) override
    {
        if (! isEnabled() || ! event.mods.isLeftButtonDown() || getNumItems() == 0)
            return;
        grabKeyboardFocus();
        setSelectedItemIndex (juce::jlimit (0, getNumItems() - 1,
                              event.x * getNumItems() / juce::jmax (1, getWidth())), juce::sendNotificationSync);
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
