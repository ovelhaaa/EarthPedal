#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <stdexcept>
#include "PluginProcessor.h"

using namespace juce;

void verifyPreDelayCapacity(double sampleRate)
{
    Dattorro reverb;
    reverb.setSampleRate(static_cast<float>(sampleRate));

    const auto expectedCapacity = static_cast<size_t>(std::ceil(sampleRate)) + 1;
    if (reverb.preDelay.delayData.size() < expectedCapacity)
        throw std::runtime_error("Pre-delay buffer cannot represent one second at this sample rate");
}

void runTestForSampleRate(double sampleRate, const String& filename)
{
    // std::cout << "Testing sample rate: " << sampleRate << " Hz..." << std::endl;

    // Create an audio buffer for 2 seconds of test signal
    int totalSamples = (int)(sampleRate * 2.0);
    AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    // std::cout << "  [trace] Generating test signal..." << std::endl;
    // Generate test signal: Impulse at start, followed by 440Hz sine wave starting at 0.5s for 0.5s
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    int sineStart = (int)(sampleRate * 0.5);
    int sineEnd = (int)(sampleRate * 1.0);
    double phase = 0.0;
    double phaseIncrement = 440.0 * 2.0 * juce::MathConstants<double>::pi / sampleRate;

    for (int i = sineStart; i < sineEnd; ++i)
    {
        float sample = (float)std::sin(phase) * 0.5f;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
        phase += phaseIncrement;
    }

    // std::cout << "  [trace] Instantiating Processor..." << std::endl;
    // Initialize Processor
    ApolloAudioProcessor processor;
    
    // std::cout << "  [trace] Calling prepareToPlay..." << std::endl;
    processor.setRateAndBufferSizeDetails(sampleRate, 256);
    processor.prepareToPlay(sampleRate, 256);

    // std::cout << "  [trace] Setting APVTS parameters..." << std::endl;
    // Force parameters to a state that uses all DSP (octave on, reverb on, overdrive on)
    processor.apvts.getParameter("effect_mode")->setValueNotifyingHost(0.333f); // Up Octave
    processor.apvts.getParameter("footswitch_mode")->setValueNotifyingHost(0.5f); // Overdrive
    processor.apvts.getParameter("momentary_effect")->setValueNotifyingHost(1.0f); // On
    processor.apvts.getParameter("mix")->setValueNotifyingHost(0.5f); // 50% Wet - TEST COMB FILTER
    processor.apvts.getParameter("decay")->setValueNotifyingHost(0.7f); // moderate decay

    // std::cout << "  [trace] Processing audio blocks..." << std::endl;
    // Process block by block
    int blockSize = 256;
    MidiBuffer midi;
    for (int start = 0; start < totalSamples; start += blockSize)
    {
        int numToProcess = juce::jmin(blockSize, totalSamples - start);
        AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, numToProcess);
        processor.processBlock(block, midi);
    }

    // std::cout << "  [trace] Writing WAV file..." << std::endl;
    // Write to WAV
    File outputFile(File::getCurrentWorkingDirectory().getChildFile(filename));
    outputFile.deleteFile();

    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(new FileOutputStream(outputFile), sampleRate, 2, 16, {}, 0));
    
    if (writer != nullptr)
    {
        writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
        // std::cout << "Successfully wrote " << filename << std::endl;
    }
    else
    {
        // std::cout << "Failed to write " << filename << std::endl;
    }
}

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);
    juce::ScopedJuceInitialiser_GUI guiInitialiser;
    // std::cout << "Starting Apollo DSP Validation Tests..." << std::endl;

    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        verifyPreDelayCapacity(sampleRate);
        runTestForSampleRate(sampleRate, "test_out_" + String(static_cast<int>(sampleRate)) + ".wav");
    }

    // std::cout << "All tests finished." << std::endl;
    return 0;
}
