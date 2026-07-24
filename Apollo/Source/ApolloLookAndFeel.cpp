#include "ApolloLookAndFeel.h"

ApolloLookAndFeel::ApolloLookAndFeel()
{
    // Define the color palette
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffff6a00)); // Glowing Orange
    setColour(juce::Slider::trackColourId, juce::Colour(0xff111111));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff6a00));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff222222));
    
    setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff6a00));
    setColour(juce::ToggleButton::textColourId, juce::Colour(0xffdddddd));
    
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffdddddd));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffff6a00));
    
    // Popup Menu colors (used by ComboBox)
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffdddddd));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffff6a00));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff000000));
}

void ApolloLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, const float rotaryStartAngle,
                                          const float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    const bool focused = slider.hasKeyboardFocus (true);
    const bool pressed = slider.isMouseOverOrDragging();

    // Draw background track
    g.setColour (findColour (juce::Slider::rotarySliderOutlineColourId));
    g.fillEllipse (rx, ry, rw, rw);
    
    // Draw LED Ring (Glow)
    if (slider.isEnabled())
    {
        juce::Path filledArc;
        filledArc.addPieSegment (rx - 2.0f, ry - 2.0f, rw + 4.0f, rw + 4.0f, rotaryStartAngle, angle, 0.75f);
        
        // Inner faint glow
        g.setColour (findColour (juce::Slider::rotarySliderFillColourId).withAlpha(0.2f));
        g.fillPath (filledArc);
        
        // Outer vivid arc
        juce::Path arcLine;
        arcLine.addCentredArc (centreX, centreY, radius + 1.0f, radius + 1.0f, 0.0f, rotaryStartAngle, angle, true);
        
        g.setColour (findColour (juce::Slider::rotarySliderFillColourId).withAlpha(pressed ? 0.55f : 0.3f));
        g.strokePath (arcLine, juce::PathStrokeType (pressed ? 5.0f : 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (arcLine, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw inner knob body
    float innerRadius = radius - 4.0f;
    juce::ColourGradient knobGrad (juce::Colour(0xff2a2a2a), centreX, centreY - innerRadius,
                                   juce::Colour(0xff0a0a0a), centreX, centreY + innerRadius, false);
    g.setGradientFill (knobGrad);
    g.fillEllipse (centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    
    // Draw indicator dot
    juce::Path p;
    auto pointerLength = innerRadius - 5.0f;
    p.addEllipse(-2.0f, -pointerLength, 4.0f, 4.0f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    g.setColour (slider.isEnabled() ? juce::Colour(0xffffffff) : juce::Colour(0xff555555));
    g.fillPath (p);

    if (focused)
    {
        g.setColour (juce::Colour (0xffffa13b));
        g.drawEllipse (rx - 3.0f, ry - 3.0f, rw + 6.0f, rw + 6.0f, 1.5f);
    }
}

void ApolloLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float minSliderPos, float maxSliderPos,
                                          const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto trackWidth = 6.0f;
    auto trackX = (float) x + (float) width * 0.5f - trackWidth * 0.5f;
    auto trackY = (float) y;
    auto trackH = (float) height;
    
    // Background track
    g.setColour (juce::Colour(0xff111111));
    g.fillRoundedRectangle (trackX, trackY, trackWidth, trackH, 3.0f);
    
    // Filled glowing track
    if (slider.isEnabled())
    {
        auto fillRect = juce::Rectangle<float> (trackX, sliderPos, trackWidth, trackY + trackH - sliderPos);
        g.setColour (findColour (juce::Slider::thumbColourId).withAlpha(0.2f));
        g.fillRoundedRectangle (fillRect.expanded(2.0f), 4.0f); // Glow
        g.setColour (findColour (juce::Slider::thumbColourId));
        g.fillRoundedRectangle (fillRect, 3.0f);
    }
    
    // Thumb
    auto thumbWidth = 26.0f;
    auto thumbHeight = 12.0f;
    juce::Rectangle<float> thumbRect ((float)x + (float)width * 0.5f - thumbWidth * 0.5f, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight);
    
    g.setColour (juce::Colour (0xff333333));
    g.fillRoundedRectangle (thumbRect, 2.0f);
    g.setColour (slider.isEnabled() ? juce::Colour(0xffffffff) : juce::Colour(0xff555555));
    g.drawRoundedRectangle (thumbRect, 2.0f, 1.0f);
    
    // Thumb line
    g.fillRect (thumbRect.getCentreX() - 6.0f, thumbRect.getCentreY() - 0.5f, 12.0f, 1.0f);
}

void ApolloLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    bool state = button.getToggleState();
    bool isBypass = button.getButtonText().startsWithIgnoreCase ("BYPASS");
    
    juce::Colour baseColor = state ? (isBypass ? juce::Colour(0xffff0000) : findColour(juce::ToggleButton::tickColourId)) 
                                   : juce::Colour(0xff333333);
    
    if (state) {
        g.setColour(baseColor.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds.expanded(2.0f), 4.0f); // Outer glow
    }
    
    g.setColour(baseColor.withAlpha(state ? 0.9f : 1.0f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    g.setColour (shouldDrawButtonAsHighlighted ? juce::Colour (0xffffa13b) : juce::Colour (0xff111111));
    g.drawRoundedRectangle(bounds, 4.0f, shouldDrawButtonAsHighlighted ? 1.5f : 1.0f);
    
    g.setColour(state ? juce::Colour(0xff000000) : juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(isBypass ? 15.0f : 13.0f).withStyle(isBypass ? juce::Font::bold : juce::Font::plain));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);

    if (shouldDrawButtonAsDown)
    {
        g.setColour (juce::Colour (0xffffffff).withAlpha (0.75f));
        g.drawText ("HOLD", bounds.withTrimmedTop (bounds.getHeight() * 0.52f), juce::Justification::centred, true);
    }

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (juce::Colour (0xffffa13b));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.5f);
    }
}

void ApolloLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    auto cornerSize = 4.0f;
    juce::Rectangle<int> boxBounds (0, 0, width, height);
    
    g.setColour (findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
    
    g.setColour (box.isMouseOver() ? findColour (juce::ComboBox::arrowColourId) : findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);
    
    juce::Rectangle<int> arrowZone (width - 22, 0, 22, height);
    juce::Path path;
    path.startNewSubPath ((float) arrowZone.getX() + 6.0f, (float) arrowZone.getCentreY() - 2.0f);
    path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
    path.lineTo ((float) arrowZone.getRight() - 6.0f, (float) arrowZone.getCentreY() - 2.0f);
    
    g.setColour (findColour (juce::ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath (path, juce::PathStrokeType (2.0f));

    if (box.hasKeyboardFocus (true))
    {
        g.setColour (juce::Colour (0xffffa13b));
        g.drawRoundedRectangle (boxBounds.toFloat().reduced (1.5f), cornerSize, 1.5f);
    }
}
