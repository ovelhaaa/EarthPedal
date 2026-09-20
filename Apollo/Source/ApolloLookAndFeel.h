#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ApolloTheme.h"

//==============================================================================
// Single LookAndFeel that draws every physical control of the Apollo rack:
// vintage knobs with printed scales, a recessed fader slot, rocker switches,
// illuminated push buttons, segmented mechanical selectors and technical
// readout labels. Geometry and painting are kept in dedicated helpers so no
// method grows into a monolith.
//==============================================================================
class ApolloLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ApolloLookAndFeel();

    void setUiScale (float scale) { uiScale = juce::jlimit (0.75f, 2.0f, scale); }
    float getUiScale() const { return uiScale; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isChecked,
                            bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    static bool isDimmed (const juce::Component& c) { return (bool) c.getProperties()[ApolloTheme::dimProperty]; }

private:
    float uiScale = 1.0f;

    float sc (float value) const { return value * uiScale; }

    void drawKnobScale (juce::Graphics&, juce::Point<float> centre, float outerRadius,
                        float innerRadius, float startAngle, float endAngle,
                        bool bipolar, bool dim);

    void drawVintageKnob (juce::Graphics&, juce::Point<float> centre, float radius,
                          float angle, bool dimmed, bool focused, bool active);

    void drawRocker (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down);
    void drawMomentary (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down);
    void drawBypass (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down);

    void drawFocusHalo (juce::Graphics&, juce::Rectangle<float> bounds, float corner);

    void drawPushBank (juce::Graphics&, juce::Rectangle<float> bounds, juce::ComboBox&, bool dimmed);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloLookAndFeel)
};
