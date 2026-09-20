#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Centralised visual language for the Apollo rack unit.
//
// The whole redesign draws from this single palette and these helper routines
// so that materials, lighting (top-left), bevels and typography stay consistent
// and no magic numbers are duplicated across the components.
//==============================================================================
namespace ApolloTheme
{
    //==========================================================================
    // Palette
    //==========================================================================
    inline const juce::Colour chassisLight   { 0xffe3ddcd }; // top-lit ivory panel
    inline const juce::Colour chassisMid     { 0xffcdc5b2 };
    inline const juce::Colour chassisDark    { 0xffa79e8a };
    inline const juce::Colour chassisEdge    { 0xff6d6656 };
    inline const juce::Colour chassisShadow  { 0xff3d3930 };

    inline const juce::Colour graphiteLight  { 0xff383d45 }; // black module body
    inline const juce::Colour graphite       { 0xff262a30 };
    inline const juce::Colour graphiteDark   { 0xff15181c };
    inline const juce::Colour graphiteDeep   { 0xff0b0d10 };
    inline const juce::Colour graphiteEdge   { 0xff050607 };

    inline const juce::Colour plateBlack     { 0xff0d0f11 }; // glass / display wells
    inline const juce::Colour plateBlackLit  { 0xff1a1210 };

    inline const juce::Colour textOnPanel    { 0xffeae4d6 }; // warm silk-screen
    inline const juce::Colour textOnPanelDim { 0xff8d897d };
    inline const juce::Colour textOnChassis  { 0xff2c2b26 }; // dark engraving
    inline const juce::Colour textOnChassisSoft { 0xff5c574c };

    inline const juce::Colour engraveHighlight { 0xfffffdf6 };
    inline const juce::Colour engraveShadow    { 0xff2a2924 };

    inline const juce::Colour orange        { 0xffff7d1a }; // parameter / normal status
    inline const juce::Colour orangeDeep    { 0xffcf5c0f };
    inline const juce::Colour orangeGlow    { 0xffffab63 };
    inline const juce::Colour red           { 0xffe35240 }; // bypass / warning
    inline const juce::Colour redDeep       { 0xff9c2a1c };
    inline const juce::Colour cyan          { 0xff74c8d6 }; // secondary indicators
    inline const juce::Colour lampOff       { 0xff37342e };

    inline const juce::Colour metalScrew    { 0xffb3ac9a };
    inline const juce::Colour metalCap      { 0xff7d786c };
    inline const juce::Colour metalHighlight { 0xfff4f0e6 };

    // Marconi / vintage instrument knob materials (warm black bakelite + chrome).
    inline const juce::Colour bakeliteLight { 0xff3c372f };
    inline const juce::Colour bakelite      { 0xff1d1a16 };
    inline const juce::Colour bakeliteDark  { 0xff0a0908 };
    inline const juce::Colour chromeLight   { 0xffd4cfc4 };
    inline const juce::Colour chromeMid     { 0xff8f897d };
    inline const juce::Colour chromeDark    { 0xff514d45 };
    inline const juce::Colour knobPointer   { 0xffeae3d1 };

    //==========================================================================
    // Design space. All layout coordinates are authored against this grid and
    // uniformly scaled by the editor to honour resize while keeping the exact
    // same architecture.
    //==========================================================================
    inline constexpr int designWidth  = 900;
    inline constexpr int designHeight = 620;

    //==========================================================================
    // Property keys. Decorative roles are passed to controls through dynamic
    // properties so a single LookAndFeel can style every instance without a
    // proliferation of subclasses.
    //==========================================================================
    inline const char* const styleProperty   = "apolloStyle";
    inline const char* const dimProperty     = "apolloDim";
    inline const char* const displayProperty = "apolloDisplay";
    inline const char* const captionProperty = "apolloCaption";
    inline const char* const verticalProperty = "apolloVertical";
    inline const char* const pushBankProperty = "apolloPushBank";
    inline const char* const bipolarProperty = "apolloBipolar";

    enum class ButtonStyle
    {
        Rocker = 0,      // small clamped rocker switch
        Momentary,       // large illuminated push button (hold)
        Bypass,          // large illuminated bypass button
        Plain
    };

    //==========================================================================
    // Typography (JUCE bundled fonts only, so nothing external is required).
    //==========================================================================
    juce::Font headingFont (float size); // tracked industrial caps
    juce::Font labelFont   (float size); // compact legible captions
    juce::Font valueFont   (float size); // technical monospaced readout

    //==========================================================================
    // Material helpers (all vector, HiDPI friendly, top-left light source).
    //==========================================================================
    void drawRaisedPlate  (juce::Graphics&, juce::Rectangle<float> bounds, float corner, bool dimmed = false);
    void drawInsetWell    (juce::Graphics&, juce::Rectangle<float> bounds, float corner, float depth = 1.0f);
    void drawBrushedMetal (juce::Graphics&, juce::Rectangle<float> bounds, juce::Colour base);
    void drawScrew        (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawEngravedText (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                           const juce::Font& font, juce::Justification justification,
                           juce::Colour faceColour, juce::Colour highlightColour);
    void drawLamp         (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour,
                           float intensity, float corner);
}

//==============================================================================
// A physical processing module: raised graphite plate, corner screws,
// silk-screened section title, optional status lamp. Purely decorative so it
// never steals mouse clicks or keyboard focus.
//==============================================================================
class ApolloRackPanel : public juce::Component
{
public:
    ApolloRackPanel (juce::String sectionTitle, juce::String sectionSubtitle);

    void setLampActive (bool shouldBeActive);
    void setLampColour (juce::Colour colour);
    void setDimmed (bool shouldBeDimmed);
    void setSubtitle (const juce::String& newSubtitle);

    void paint (juce::Graphics&) override;

private:
    juce::String title, subtitle;
    bool lampActive = false;
    bool dimmed = false;
    juce::Colour lampColour { ApolloTheme::orange };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloRackPanel)
};

//==============================================================================
// Console status annunciator: recessed dark glass window with an internal lamp
// and an engraved caption. Decorative and non-focusable.
//==============================================================================
class ApolloAnnunciator : public juce::Component
{
public:
    ApolloAnnunciator();

    void setText (const juce::String& newText);
    void setLamp (bool isLit, juce::Colour colour);
    void setDimmed (bool shouldBeDimmed);

    void paint (juce::Graphics&) override;

private:
    juce::String text;
    bool lampOn = false;
    bool dimmed = false;
    juce::Colour lampColour { ApolloTheme::orange };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAnnunciator)
};

//==============================================================================
// Small standalone pilot lamp (e.g. LFO activity). Decorative.
//==============================================================================
class ApolloStatusLamp : public juce::Component
{
public:
    ApolloStatusLamp();

    void setState (bool isLit, juce::Colour colour);

    void paint (juce::Graphics&) override;

private:
    bool lit = false;
    juce::Colour lampColour { ApolloTheme::orange };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloStatusLamp)
};

//==============================================================================
// A ComboBox that is still a plain ComboBox for attachment and keyboard
// purposes, but reacts to a click on a segment by selecting that position
// directly. A click on the right-hand rocker opens the native styled popup.
//==============================================================================
class ApolloSelector : public juce::ComboBox
{
public:
    void mouseDown (const juce::MouseEvent& event) override
    {
        if (! isEnabled())
        {
            juce::ComboBox::mouseDown (event);
            return;
        }

        const auto n = getNumItems();
        if (n <= 0)
        {
            juce::ComboBox::mouseDown (event);
            return;
        }

        const auto area = getLocalBounds();

        // DC-2 style interlocked push-button bank: any click selects a position.
        if ((bool) getProperties()[ApolloTheme::pushBankProperty])
        {
            const bool bankVertical = (bool) getProperties()[ApolloTheme::verticalProperty];
            const int index = bankVertical
                ? juce::jlimit (0, n - 1, (event.y * n) / juce::jmax (1, area.getHeight()))
                : juce::jlimit (0, n - 1, (event.x * n) / juce::jmax (1, area.getWidth()));
            selectIndex (index);
            return;
        }

        const bool vertical = getProperties()[ApolloTheme::verticalProperty];

        // Reserve the trailing rocker zone for the native popup so the control
        // stays fully keyboard/accessible friendly.
        if (vertical)
        {
            if (event.y >= area.getBottom() - 12)
            {
                juce::ComboBox::mouseDown (event);
                return;
            }
            const int index = juce::jlimit (0, n - 1, (event.y * n) / juce::jmax (1, area.getHeight()));
            selectIndex (index);
        }
        else
        {
            if (event.x >= area.getRight() - 16)
            {
                juce::ComboBox::mouseDown (event);
                return;
            }
            const int index = juce::jlimit (0, n - 1, (event.x * n) / juce::jmax (1, area.getWidth()));
            selectIndex (index);
        }
    }

private:
    void selectIndex (int index)
    {
        grabKeyboardFocus();
        if (index != getSelectedItemIndex())
            setSelectedItemIndex (index, juce::sendNotification);
    }
};
