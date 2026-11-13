#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ApolloSpaceField final : public juce::Component, private juce::Timer
{
public:
    explicit ApolloSpaceField (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    void updateTimer();

private:
    struct Settings
    {
        float predelay, decay, size, tone, rate, depth;
        int octave;
        bool diffusion, frozen, bypassed;

        bool operator== (const Settings&) const noexcept;
    };

    Settings readSettings() const noexcept;
    void timerCallback() override;

    const std::atomic<float>* const predelay;
    const std::atomic<float>* const decay;
    const std::atomic<float>* const size;
    const std::atomic<float>* const tone;
    const std::atomic<float>* const rate;
    const std::atomic<float>* const depth;
    const std::atomic<float>* const octaveMode;
    const std::atomic<float>* const diffusion;
    const std::atomic<float>* const action;
    const std::atomic<float>* const gate;
    const std::atomic<float>* const bypass;

    Settings lastSettings {};
    double lastFrame = 0.0;
    float phase = 0.0f;
    juce::Path envelope;
};
