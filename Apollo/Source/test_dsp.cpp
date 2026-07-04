#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "PluginProcessor.h"

using namespace juce;

void runTestForSampleRate(double sampleRate, const String& filename)
{
    std::cout << "Testing sample rate: " << sampleRate << " Hz..." << std::endl;

    // Create an audio buffer for 2 seconds of test signal
    int totalSamples = (int)(sampleRate * 2.0);
    AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    std::cout << "  [trace] Generating test signal..." << std::endl;
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

    std::cout << "  [trace] Instantiating Processor..." << std::endl;
    // Initialize Processor
    ApolloAudioProcessor processor;
    
    std::cout << "  [trace] Calling prepareToPlay..." << std::endl;
    processor.prepareToPlay(sampleRate, 256);

    std::cout << "  [trace] Setting APVTS parameters..." << std::endl;
    // Force parameters to a state that uses all DSP (octave on, reverb on, overdrive on)
    processor.apvts.getParameter("effect_mode")->setValueNotifyingHost(0.5f); // Up Octave
    processor.apvts.getParameter("footswitch_mode")->setValueNotifyingHost(0.5f); // Overdrive
    processor.apvts.getParameter("momentary_effect")->setValueNotifyingHost(1.0f); // On
    processor.apvts.getParameter("mix")->setValueNotifyingHost(1.0f); // 100% Wet
    processor.apvts.getParameter("decay")->setValueNotifyingHost(0.7f); // moderate decay

    std::cout << "  [trace] Processing audio blocks..." << std::endl;
    // Process block by block
    int blockSize = 256;
    MidiBuffer midi;
    for (int start = 0; start < totalSamples; start += blockSize)
    {
        int numToProcess = juce::jmin(blockSize, totalSamples - start);
        AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, numToProcess);
        processor.processBlock(block, midi);
    }

    std::cout << "  [trace] Writing WAV file..." << std::endl;
    // Write to WAV
    File outputFile(File::getCurrentWorkingDirectory().getChildFile(filename));
    outputFile.deleteFile();

    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(new FileOutputStream(outputFile), sampleRate, 2, 16, {}, 0));
    
    if (writer != nullptr)
    {
        writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
        std::cout << "Successfully wrote " << filename << std::endl;
    }
    else
    {
        std::cout << "Failed to write " << filename << std::endl;
    }
}

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);
    juce::ScopedJuceInitialiser_GUI guiInitialiser;
    std::cout << "Starting Apollo DSP Validation Tests..." << std::endl;

    runTestForSampleRate(44100.0, "test_out_44100.wav");
    runTestForSampleRate(48000.0, "test_out_48000.wav");
    runTestForSampleRate(96000.0, "test_out_96000.wav");

    std::cout << "All tests finished." << std::endl;
    return 0;
}
