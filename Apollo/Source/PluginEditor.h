#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ApolloAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ApolloAudioProcessorEditor (ApolloAudioProcessor&);
    ~ApolloAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ApolloAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloAudioProcessorEditor)
};
