#include "ApolloSpaceField.h"
#include "ApolloTheme.h"

#include <cmath>
#include <tuple>

namespace
{
constexpr int framesPerSecond = 30;
constexpr int hiddenPollIntervalMs = 500;
}

ApolloSpaceField::ApolloSpaceField (juce::AudioProcessorValueTreeState& state)
    : predelay (state.getRawParameterValue ("predelay")),
      decay (state.getRawParameterValue ("decay")),
      size (state.getRawParameterValue ("time_scale")),
      tone (state.getRawParameterValue ("damp")),
      rate (state.getRawParameterValue ("modspeed")),
      depth (state.getRawParameterValue ("moddepth")),
      octaveMode (state.getRawParameterValue ("effect_mode")),
      diffusion (state.getRawParameterValue ("input_diffusion")),
      action (state.getRawParameterValue ("footswitch_mode")),
      gate (state.getRawParameterValue ("momentary_effect")),
      bypass (state.getRawParameterValue ("bypass"))
{
    setTitle ("Space Field");
    setDescription ("Parametric model of early reflections and the reverb tail.");
    setInterceptsMouseClicks (false, false);
    envelope.preallocateSpace (400);
}

bool ApolloSpaceField::Settings::operator== (const Settings& rhs) const noexcept
{
    return std::tie (predelay, decay, size, tone, rate, depth, octave, diffusion, frozen, bypassed)
        == std::tie (rhs.predelay, rhs.decay, rhs.size, rhs.tone, rhs.rate, rhs.depth,
                     rhs.octave, rhs.diffusion, rhs.frozen, rhs.bypassed);
}

ApolloSpaceField::Settings ApolloSpaceField::readSettings() const noexcept
{
    const auto load = [] (const std::atomic<float>* value) { return value->load (std::memory_order_relaxed); };
    const auto performance = load (action);
    const bool held = load (gate) > 0.5f;
    return { load (predelay), load (decay), load (size), load (tone), load (rate), load (depth),
             performance == 2.0f && ! held ? 0 : (int) load (octaveMode),
             load (diffusion) > 0.5f, performance == 0.0f && held, load (bypass) > 0.5f };
}

void ApolloSpaceField::visibilityChanged() { updateTimer(); }
void ApolloSpaceField::parentHierarchyChanged() { updateTimer(); }

void ApolloSpaceField::updateTimer()
{
    if (! isShowing())
    {
        // A minimised native peer can restore without a visibility callback.
        if (getTimerInterval() != hiddenPollIntervalMs)
            startTimer (hiddenPollIntervalMs);
        return;
    }

    if (getTimerInterval() != 1000 / framesPerSecond)
    {
        lastSettings = readSettings();
        lastFrame = juce::Time::getMillisecondCounterHiRes();
        startTimerHz (framesPerSecond);
        repaint();
    }
}

void ApolloSpaceField::timerCallback()
{
    // A native peer can be minimised and restored without a visibility callback.
    if (! isShowing())
    {
        if (getTimerInterval() != hiddenPollIntervalMs)
            startTimer (hiddenPollIntervalMs);
        return;
    }

    updateTimer();

    const auto now = juce::Time::getMillisecondCounterHiRes();
    const float seconds = juce::jlimit (0.0f, 0.1f, (float) ((now - lastFrame) / 1000.0));
    lastFrame = now;
    const auto settings = readSettings();
    const bool moving = ! settings.bypassed && settings.depth > 0.0f;
    if (moving)
    {
        // The tank's rate multiplier drives a deliberately slowed display animation.
        phase = std::fmod (phase + seconds * (0.3f + 15.0f * settings.rate) * 1.4f,
                           juce::MathConstants<float>::twoPi);
    }

    if (moving || ! (settings == lastSettings))
        repaint();

    lastSettings = settings;
}

void ApolloSpaceField::paint (juce::Graphics& g)
{
    using namespace ApolloTheme;
    const auto settings = readSettings();
    const float s = getWidth() / 852.0f;
    auto bounds = getLocalBounds().toFloat();
    g.setColour (graphiteDeep);
    g.fillRoundedRectangle (bounds, 4.0f * s);

    auto header = bounds.removeFromTop (35.0f * s).reduced (16.0f * s, 0.0f);
    g.setColour (orange);
    g.fillRect (header.getX(), header.getCentreY() - 3.0f * s, 3.0f * s, 6.0f * s);
    header.removeFromLeft (12.0f * s);
    g.setColour (textOnPanel);
    g.setFont (headingFont (12.0f * s));
    g.drawText ("SPACE FIELD", header.removeFromLeft (140.0f * s), juce::Justification::centredLeft);
    g.setFont (labelFont (10.0f * s));
    g.setColour (textOnPanelDim);
    g.drawText ("PARAMETRIC MODEL", header.removeFromRight (148.0f * s), juce::Justification::centredRight);
    if (settings.bypassed || settings.frozen)
    {
        g.setColour (settings.bypassed ? textOnPanel : orangeBright);
        g.drawText (settings.bypassed ? "BYPASSED" : "FREEZE ACTIVE", header.withTrimmedRight (16.0f * s),
                    juce::Justification::centredRight);
    }

    auto plot = bounds.reduced (28.0f * s, 0.0f);
    auto legend = plot.removeFromBottom (26.0f * s);
    plot.removeFromTop (4.0f * s);
    plot.removeFromBottom (3.0f * s);
    const float middle = plot.getCentreY();
    const auto x = [&plot] (float t) { return plot.getX() + t * plot.getWidth(); };

    g.setColour (graphiteEdge.withAlpha (0.48f));
    for (int i = 0; i <= 12; ++i)
        g.drawVerticalLine (juce::roundToInt (x ((float) i / 12.0f)), plot.getY(), plot.getBottom());
    for (int i = 0; i <= 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * (float) i / 4.0f),
                              plot.getX(), plot.getRight());

    g.setColour (textOnPanelDim);
    g.drawText ("INPUT", legend, juce::Justification::centredLeft);
    g.drawText ("RELATIVE TIME", legend, juce::Justification::centredRight);

    if (settings.bypassed)
        return;

    const float onset = 0.035f + settings.predelay * 0.27f;
    const float spread = 0.66f + settings.size * 0.17f;
    const float duration = settings.frozen ? 1.0f - onset
                         : (0.98f - onset) * spread * (0.26f + 0.74f * settings.decay);
    const float height = plot.getHeight() * 0.41f;
    const float detail = 0.75f + 0.5f * settings.tone;
    const float modulation = settings.depth * 0.26f;
    const auto amplitude = [&] (float t)
    {
        return height * (settings.frozen ? 0.76f - 0.12f * t
                                        : std::exp (-t * (3.9f - 1.65f * settings.decay)) * (1.0f - t));
    };

    g.setColour (orangeBright);
    g.fillEllipse (x (0) - 2.5f * s, middle - 2.5f * s, 5.0f * s, 5.0f * s);
    g.setColour (orange.withAlpha (0.45f));
    g.drawLine (x (0) + 4.0f * s, middle, x (onset), middle, s);

    // Fixed tap positions keep the pattern coherent while parameters are automated.
    for (int i = 0; i < 7; ++i)
    {
        const float tap = (float) i / 6.0f;
        const float xx = x (onset + tap * duration * 0.38f);
        const float peak = amplitude (tap * 0.38f) * (0.56f + 0.36f * std::cos ((float) i * 2.3f));
        g.setColour (orangeBright.withAlpha (0.9f - tap * 0.37f));
        g.drawLine (xx, middle - peak, xx, middle + peak * 0.78f, s);
        g.fillEllipse (xx - s, middle - peak - s, 2.0f * s, 2.0f * s);
    }

    const int count = juce::roundToInt ((settings.diffusion ? 154.0f : 80.0f) * detail);
    g.setColour (orange.withAlpha (settings.diffusion ? 0.56f : 0.66f));
    for (int i = 0; i < count; ++i)
    {
        const float fraction = (float) i / (float) count;
        const float t = 0.09f + 0.91f * std::pow (fraction, 0.75f);
        const float scatter = (float) ((i * 53) % 97) / 48.0f - 1.0f;
        const float motion = modulation * std::sin (phase + t * 12.0f + (float) i * 0.36f);
        const float yy = middle + amplitude (t) * (scatter * (settings.diffusion ? 0.87f : 0.52f) + motion);
        const float diameter = (1.0f + 0.7f * (1.0f - t)) * s;
        g.fillEllipse (x (onset + duration * t) - diameter * 0.5f, yy - diameter * 0.5f, diameter, diameter);
    }

    const auto trace = [&] (float lane, juce::Colour colour)
    {
        envelope.clear();
        for (int i = 0; i <= 56; ++i)
        {
            const float t = (float) i / 56.0f;
            const float ripple = modulation * std::sin (phase + t * (8.0f + 9.0f * detail));
            const auto xx = x (onset + duration * t);
            const auto yy = middle + amplitude (t) * (lane + ripple);
            if (i == 0) envelope.startNewSubPath (xx, yy);
            else        envelope.lineTo (xx, yy);
        }
        g.setColour (colour);
        g.strokePath (envelope, juce::PathStrokeType (s));
    };

    trace (-0.90f, orange.withAlpha (0.57f));
    trace ( 0.90f, orange.withAlpha (0.57f));
    if ((settings.octave & 1) != 0) trace (-0.56f, orangeBright.withAlpha (0.78f));
    if ((settings.octave & 2) != 0) trace ( 0.56f, orangeBright.withAlpha (0.78f));
}
