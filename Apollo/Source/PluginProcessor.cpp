#include "PluginProcessor.h"
#include "PluginEditor.h"

using earth::EarthParameters;
using earth::OctaveMode;
using earth::PerformanceMode;
using earth::ReverbSize;

namespace {

ReverbSize mapReverbSize(int choice)
{
    switch (choice)
    {
        case 0: return ReverbSize::Small;
        case 1: return ReverbSize::Medium;
        default: return ReverbSize::Large;
    }
}

// APVTS effect_mode: 0 None, 1 Up Octave, 2 Down Octave, 3 Both Octaves.
OctaveMode mapOctaveMode(int choice)
{
    switch (choice)
    {
        case 1: return OctaveMode::Up;
        case 2: return OctaveMode::Down;
        case 3: return OctaveMode::Both;
        default: return OctaveMode::Off;
    }
}

PerformanceMode mapPerformanceMode(int choice)
{
    switch (choice)
    {
        case 0: return PerformanceMode::Freeze;
        case 1: return PerformanceMode::Overdrive;
        default: return PerformanceMode::Octave;
    }
}

} // namespace

ApolloAudioProcessor::ApolloAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

ApolloAudioProcessor::~ApolloAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ApolloAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Knobs (ids, ranges, defaults preserved).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"predelay", 1}, "Pre-Delay", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decay", 1}, "Decay", 0.0f, 1.0f, 0.877465f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"moddepth", 1}, "Mod Depth", 0.0f, 1.0f, 0.0625f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modspeed", 1}, "Mod Speed", 0.0f, 1.0f, 0.0466667f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"damp", 1}, "Damp", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"eq1_gain", 1}, "EQ1 Gain", -24.0f, 24.0f, -11.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"eq2_gain", 1}, "EQ2 Gain", -24.0f, 24.0f, 5.0f));

    // Toggle Switches
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"time_scale", 1}, "Time Scale", juce::StringArray{"Small", "Medium", "Large"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"effect_mode", 1}, "Effect Mode", juce::StringArray{"None", "Up Octave", "Down Octave", "Both Octaves"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"footswitch_mode", 1}, "Momentary Mode", juce::StringArray{"Freeze", "Overdrive", "Effect"}, 0));

    // Dip Switches & Toggles
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"input_diffusion", 1}, "Input Diffusion", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"octave_dry_mix", 1}, "Octave Dry Mix", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "UI Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"momentary_effect", 1}, "Momentary Switch", false));

    return { params.begin(), params.end() };
}

const juce::String ApolloAudioProcessor::getName() const { return "Apollo"; }
bool ApolloAudioProcessor::acceptsMidi() const { return false; }
bool ApolloAudioProcessor::producesMidi() const { return false; }
bool ApolloAudioProcessor::isMidiEffect() const { return false; }
double ApolloAudioProcessor::getTailLengthSeconds() const { return 8.0; }
int ApolloAudioProcessor::getNumPrograms() { return 1; }
int ApolloAudioProcessor::getCurrentProgram() { return 0; }
void ApolloAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String ApolloAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void ApolloAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void ApolloAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    core_.prepare(sampleRate, samplesPerBlock);
    setLatencySamples(core_.getLatencySamples());
}

void ApolloAudioProcessor::releaseResources() {}

bool ApolloAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void ApolloAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    EarthParameters p = EarthParameters::defaults();
    p.preDelaySeconds     = apvts.getRawParameterValue("predelay")->load();
    p.mix                 = apvts.getRawParameterValue("mix")->load();
    p.decay               = apvts.getRawParameterValue("decay")->load();
    p.modulationDepth     = apvts.getRawParameterValue("moddepth")->load();
    p.modulationSpeed     = apvts.getRawParameterValue("modspeed")->load();
    p.damp                = apvts.getRawParameterValue("damp")->load();
    p.octaveHighShelfDb   = apvts.getRawParameterValue("eq1_gain")->load();
    p.octaveLowShelfDb    = apvts.getRawParameterValue("eq2_gain")->load();

    const int timeScale   = (int) std::round(apvts.getRawParameterValue("time_scale")->load());
    const int effectMode  = (int) std::round(apvts.getRawParameterValue("effect_mode")->load());
    const int footswitch  = (int) std::round(apvts.getRawParameterValue("footswitch_mode")->load());

    p.reverbSize     = mapReverbSize(timeScale);
    p.inputDiffusion = apvts.getRawParameterValue("input_diffusion")->load();

    // Canonical inner-dry semantic. The legacy APVTS id and stored value are
    // preserved; the meaning is now positive (ON = include 0.5*dry), matching
    // the Web/earth reference. See PARAMETER_ADAPTERS.md.
    p.includeDryInOctavePath = apvts.getRawParameterValue("octave_dry_mix")->load();

    const bool momentary = apvts.getRawParameterValue("momentary_effect")->load();
    p.performanceMode  = mapPerformanceMode(footswitch);
    p.performanceActive = momentary;

    // Octave selection. In "Effect" momentary mode the octave only engages
    // while held; otherwise it follows effect_mode directly.
    OctaveMode octave = mapOctaveMode(effectMode);
    if (footswitch == 2 && !momentary)
        octave = OctaveMode::Off;
    p.octaveMode = octave;

    p.bypass = apvts.getRawParameterValue("bypass")->load();

    core_.setParameters(p);

    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : channelDataL;

    core_.process(channelDataL, channelDataR, channelDataL, channelDataR, numSamples);
}

bool ApolloAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* ApolloAudioProcessor::createEditor() { return new ApolloAudioProcessorEditor (*this); }

void ApolloAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ApolloAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ApolloAudioProcessor();
}
