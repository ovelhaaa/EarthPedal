#include "PluginProcessor.h"
#include "PluginEditor.h"

ApolloAudioProcessorEditor::ApolloAudioProcessorEditor (ApolloAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Minimal temporary size before UI design phase
    setSize (400, 300);
}

ApolloAudioProcessorEditor::~ApolloAudioProcessorEditor()
{
}

void ApolloAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Apollo UI Placeholder", getLocalBounds(), juce::Justification::centred, 1);
}

void ApolloAudioProcessorEditor::resized()
{
}
