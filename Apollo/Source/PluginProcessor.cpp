#include "PluginProcessor.h"
#include "PluginEditor.h"

ApolloAudioProcessor::ApolloAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout()),
       reverb(48000, 16, 4.0) // samplerate, max_lfo_depth, max_timescale
{
    overdriveLeft.Init();
    overdriveRight.Init();
    std::cout << "[Diagnostic 1d] ApolloAudioProcessor CONSTRUCTED. State fully reset." << std::endl;
}

ApolloAudioProcessor::~ApolloAudioProcessor()
{
    if (measure_count_in > 0) {
        double rms_in = std::sqrt(energy_in_above_24k / measure_count_in);
        double rms_out = std::sqrt(energy_out_above_24k / measure_count_out);
        std::cout << "[Diagnostic 1a] SR=" << getSampleRate() << " | RMS > 24kHz IN: " << rms_in << " | RMS > 24kHz OUT (after Lagrange): " << rms_out << std::endl;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ApolloAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Knobs
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"predelay", 1}, "Pre-Delay", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decay", 1}, "Decay", 0.0f, 1.0f, 0.877f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"moddepth", 1}, "Mod Depth", 0.0f, 1.0f, 0.0625f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modspeed", 1}, "Mod Speed", 0.0f, 1.0f, 0.0466f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"damp", 1}, "Damp", 0.0f, 1.0f, 0.5f));

    // Toggle Switches
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"time_scale", 1}, "Time Scale", juce::StringArray{"Small", "Medium", "Large"}, 2)); // Default Large
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"effect_mode", 1}, "Effect Mode", juce::StringArray{"None", "Up Octave", "Down Octave"}, 0)); // Default None
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
double ApolloAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ApolloAudioProcessor::getNumPrograms() { return 1; }
int ApolloAudioProcessor::getCurrentProgram() { return 0; }
void ApolloAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String ApolloAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void ApolloAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void ApolloAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // std::cout << "    [prepareToPlay] reverb.setSampleRate..." << std::endl;
    reverb.setSampleRate((float)sampleRate);
    // std::cout << "    [prepareToPlay] reverb.clear..." << std::endl;
    reverb.clear();

    // std::cout << "    [prepareToPlay] init OctaveGenerator..." << std::endl;
    // The OctaveGenerator is instantiated with 48000 Hz, since it runs in the resampled branch.
    octave = std::make_unique<OctaveGenerator>(48000.0f / resample_factor);
    
    // std::cout << "    [prepareToPlay] IIR Filter setup..." << std::endl;
    // Replace cycfi q filters with JUCE DSP IIR filters. These run inside the resampled 48kHz branch.
    eq1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(48000.0f / resample_factor, 140.0f, 0.707f, juce::Decibels::decibelsToGain(-11.0f));
    eq2.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(48000.0f / resample_factor, 160.0f, 0.707f, juce::Decibels::decibelsToGain(5.0f));
    
    juce::dsp::ProcessSpec spec { 48000.0 / resample_factor, (juce::uint32)samplesPerBlock, 1 };
    // std::cout << "    [prepareToPlay] IIR eq1.prepare..." << std::endl;
    eq1.prepare(spec);
    // std::cout << "    [prepareToPlay] IIR eq2.prepare..." << std::endl;
    eq2.prepare(spec);
    eq1.reset();
    eq2.reset();

    // std::cout << "    [prepareToPlay] overdriveInit..." << std::endl;
    overdriveLeft.Init();
    overdriveRight.Init();
    overdriveLeft.SetDrive(0.4f);
    overdriveRight.SetDrive(0.4f);

    // std::cout << "    [prepareToPlay] smoothers reset..." << std::endl;
    // Smoothers setup
    current_predelay.reset(sampleRate, 0.005);
    current_moddepth.reset(sampleRate, 0.005);
    current_modspeed.reset(sampleRate, 0.005);
    current_freezeDecay.reset(sampleRate, 0.005);
    current_ODswell.reset(sampleRate, 0.015);
    bypassFade.reset(sampleRate, 0.01); // 10ms smooth

    // std::cout << "    [prepareToPlay] smoothers load..." << std::endl;
    // Ensure parameters match defaults immediately
    current_predelay.setCurrentAndTargetValue(apvts.getRawParameterValue("predelay")->load());
    current_moddepth.setCurrentAndTargetValue(apvts.getRawParameterValue("moddepth")->load());
    current_modspeed.setCurrentAndTargetValue(apvts.getRawParameterValue("modspeed")->load());
    current_freezeDecay.setCurrentAndTargetValue(apvts.getRawParameterValue("decay")->load());
    current_ODswell.setCurrentAndTargetValue(0.4f);
    bypassFade.setCurrentAndTargetValue(0.0f);

    // Resampler setup
    octaveResamplerUp.reset();
    octaveResamplerDown.reset();
    
    // Anti-alias filter (8th order Butterworth LPF)
    // Cutoff: min(23000.0, sampleRate * 0.45)
    double cutoff = juce::jmin(23000.0, sampleRate * 0.45);
    auto filterCoeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(cutoff, sampleRate, 8);
    juce::dsp::ProcessSpec hostSpec { sampleRate, (juce::uint32)samplesPerBlock, 1 };
    
    for (int i = 0; i < 4; ++i) {
        if (i < filterCoeffs.size())
            antiAliasFilters[i].coefficients = filterCoeffs[i];
        antiAliasFilters[i].prepare(hostSpec);
        antiAliasFilters[i].reset();
    }
    
    // Sliding window FIFOs
    slideUp.setSize(1, 4096);
    slideUp.clear();
    slideUpValid = 0;
    phaseUp = 0.0;
    
    slideDown.setSize(1, 4096);
    slideDown.clear();
    // Pre-fill with latency to absorb fractional block jitter (e.g., 64 samples at 48kHz)
    slideDownValid = 64;
    phaseDown = 0.0;
    
    int max48kSamples = (int)(samplesPerBlock * (48000.0 / sampleRate)) + 32;
    resampleBuffer48k.setSize(1, max48kSamples);
    
    // Latency compensation for the Dry signal
    juce::dsp::ProcessSpec hostSpecDelay { sampleRate, (juce::uint32)samplesPerBlock, 1 };
    dryDelayL.prepare(hostSpecDelay);
    dryDelayR.prepare(hostSpecDelay);
    
    // Total 48kHz latency: 64 (slideDown) + 22 (FIR dec/int) = 86 samples at 48kHz.
    // Plus ~1.5 for Lagrange up/down = ~89 samples at 48kHz.
    // We convert this 48kHz-domain latency to host sample rate latency:
    float latencySamples = 89.0f * (float)(sampleRate / 48000.0);
    // Add additional latency for the anti-alias IIR filters (approx 8 samples)
    latencySamples += 8.0f; 
    
    dryDelayL.setDelay(latencySamples);
    dryDelayR.setDelay(latencySamples);
    
    // Report latency to host (if you want the DAW to compensate)
    setLatencySamples((int)latencySamples);
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
    auto numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    // std::cout << "      [processBlock] Started block of size " << numSamples << std::endl;

    // Parameters
    float vpredelay = apvts.getRawParameterValue("predelay")->load();
    float vmix = apvts.getRawParameterValue("mix")->load();
    float vdecay = apvts.getRawParameterValue("decay")->load();
    float vmoddepth = apvts.getRawParameterValue("moddepth")->load();
    float vmodspeed = apvts.getRawParameterValue("modspeed")->load();
    float vdamp = apvts.getRawParameterValue("damp")->load();
    
    int toggleValues0 = static_cast<int>(std::round(apvts.getRawParameterValue("time_scale")->load()));
    int effect_mode = static_cast<int>(std::round(apvts.getRawParameterValue("effect_mode")->load()));
    int footswitch_mode = static_cast<int>(std::round(apvts.getRawParameterValue("footswitch_mode")->load()));
    
    bool input_diffusion = apvts.getRawParameterValue("input_diffusion")->load();
    bool octave_dry_mix = apvts.getRawParameterValue("octave_dry_mix")->load();
    bool momentary_effect = apvts.getRawParameterValue("momentary_effect")->load();
    bool isBypass = apvts.getRawParameterValue("bypass")->load();

    // Time scale update
    float setTimeScale = 2.0f;
    if (toggleValues0 == 0) setTimeScale = 1.0f;
    else if (toggleValues0 == 2) setTimeScale = 4.0f;
    reverb.setTimeScale(setTimeScale);

    // Momentary logic
    if (footswitch_mode == 0 && momentary_effect) freeze = true; else freeze = false;
    if (footswitch_mode == 1 && momentary_effect) {
        setOD = 0.6f;
        odOn = true;
    } else {
        setOD = 0.4f;
    }
    
    // Mix smoothing logic (energy constant crossfade)
    if (pmix != vmix) {
        float x2 = 1.0f - vmix;
        float A = vmix * x2;
        float B = A * (1.0f + 1.4186f * A);
        float C = B + vmix;
        float D = B + x2;
        current_wetMix.setTargetValue(C * C);
        current_dryMix.setTargetValue(D * D);
        pmix = vmix;
    }

    // Damp update
    if (pdamp != vdamp) {
        if (vdamp < 0.5f) {
            float reverbDampHigh = vdamp * 2.0f;
            reverb.setInputFilterHighCutoffPitch(7.0f * reverbDampHigh + 3.0f);
        } else {
            float reverbDampLow = (vdamp - 0.5f) * 2.0f;
            reverb.setInputFilterLowCutoffPitch(9.0f * reverbDampLow);
        }
        pdamp = vdamp;
    }

    reverb.enableInputDiffusion(input_diffusion);

    // Bypass Fade Logic
    bypassFade.setTargetValue(isBypass ? 1.0f : 0.0f);

    // std::cout << "      [processBlock] Setting up Octave branch..." << std::endl;
    // --- OCTAVE BRANCH (RESAMPLED TO 48kHz) ---
    juce::AudioBuffer<float> octaveOutTemp(1, numSamples);
    octaveOutTemp.clear();

    if (effect_mode != 0) {
        // Average inputs for the mono octave path
        juce::AudioBuffer<float> monoInput(1, numSamples);
        monoInput.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (buffer.getNumChannels() > 1) {
            monoInput.addFrom(0, 0, buffer, 1, 0, numSamples);
            monoInput.applyGain(0.5f);
        }

        // Apply anti-aliasing filter (8th order = 4 biquads) in-place
        auto* monoPtr = monoInput.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i) {
            float s = monoPtr[i];
            for (int f = 0; f < 4; ++f)
                s = antiAliasFilters[f].processSample(s);
            monoPtr[i] = s;
        }

        // Measure energy > 24kHz IN (after LPF)
        for (int i = 0; i < numSamples; ++i) {
            float x = monoInput.getSample(0, i);
            float y = 0.5f * (x - prev_x_in);
            prev_x_in = x;
            energy_in_above_24k += (double)(y * y);
            measure_count_in++;
        }

        // Append to slideUp FIFO
        slideUp.copyFrom(0, slideUpValid, monoInput, 0, 0, numSamples);
        slideUpValid += numSamples;

        // Calculate how many 48kHz samples we can generate (leave 4 samples for interpolation margin)
        double ratioUp = getSampleRate() / 48000.0;
        int samples48k_to_generate = 0;
        if (slideUpValid > 4) {
            // How many output samples can we produce before phaseUp + (N * ratioUp) > (slideUpValid - 4)?
            double maxConsumed = (double)(slideUpValid - 4);
            samples48k_to_generate = (int)std::floor((maxConsumed - phaseUp) / ratioUp);
        }

        if (samples48k_to_generate > 0) {
            // Resample Up to 48kHz
            octaveResamplerUp.process(ratioUp, slideUp.getReadPointer(0), resampleBuffer48k.getWritePointer(0), samples48k_to_generate);

            if (!printed1c) {
                std::cout << "[Diagnostic 1b] octave_dry_mix applied INSIDE the 48kHz resampled loop. Mix is calculated over 48k buffers." << std::endl;
                std::cout << "[Diagnostic 1c] SR=" << getSampleRate() << " | Host numSamples: " << numSamples << " | resampled samples48k_to_generate: " << samples48k_to_generate << " | ratioUp: " << ratioUp << std::endl;
                std::cout << "                decimator chunk size: 6 | samples48k % 6 = " << (samples48k_to_generate % 6) << std::endl;
                printed1c = true;
            }

            // Measure energy > 24kHz OUT
            for (int i = 0; i < samples48k_to_generate; ++i) {
                float x = resampleBuffer48k.getSample(0, i);
                float y = 0.5f * (x - prev_x_out);
                prev_x_out = x;
                energy_out_above_24k += (double)(y * y);
                measure_count_out++;
            }

            // Calculate exactly how many input samples were consumed
            double exactConsumedUp = samples48k_to_generate * ratioUp;
            int consumedUp = (int)std::floor(phaseUp + exactConsumedUp);
            phaseUp = (phaseUp + exactConsumedUp) - consumedUp;
            
            if (consumedUp > slideUpValid) consumedUp = slideUpValid;
            
            // Shift slideUp
            int remainingUp = slideUpValid - consumedUp;
            for (int i = 0; i < remainingUp; ++i) {
                slideUp.setSample(0, i, slideUp.getSample(0, consumedUp + i));
            }
            slideUpValid = remainingUp;

            // Process Octave at 48kHz
            for (int i = 0; i < samples48k_to_generate; ++i) {
                float inSample = resampleBuffer48k.getSample(0, i);
                buff[bin_counter] = inSample;
                
                if (bin_counter > 4) {
                    std::span<const float, resample_factor> in_chunk(&(buff[0]), resample_factor);
                    const auto sample = decimate(in_chunk); 
                    
                    float octave_mix = 0.0f;
                    octave->update(sample);

                    if (effect_mode != 0)
                        octave_mix += octave->up1() * 2.0f;
                    if (effect_mode == 2) {
                        octave_mix += octave->down1() * 2.0f;
                        octave_mix += octave->down2() * 2.0f;
                    }

                    auto out_chunk = interpolate(octave_mix);
                    for (size_t j = 0; j < out_chunk.size(); ++j)
                    {
                        float eqSample = eq1.processSample(out_chunk[j]);
                        float mix = eq2.processSample(eqSample);

                        float dryLevel = 0.5f;
                        if (!octave_dry_mix || effect_mode == 2) 
                            mix += dryLevel * buff[j];
                            
                        buff_out[j] = mix;
                    }
                }
                
                bin_counter += 1;
                if (bin_counter > 5) bin_counter = 0;
                
                resampleBuffer48k.setSample(0, i, buff_out[bin_counter]);
            }

            // Append processed 48kHz samples to slideDown FIFO
            slideDown.copyFrom(0, slideDownValid, resampleBuffer48k, 0, 0, samples48k_to_generate);
            slideDownValid += samples48k_to_generate;
        }

        // Resample Down to Host Sample Rate (we MUST produce EXACTLY numSamples)
        double ratioDown = 48000.0 / getSampleRate();
        
        // We need (numSamples * ratioDown) + 4 samples
        double exactConsumedDown = numSamples * ratioDown;
        int required48k = (int)std::floor(phaseDown + exactConsumedDown) + 4;
        
        if (slideDownValid >= required48k) {
            octaveResamplerDown.process(ratioDown, slideDown.getReadPointer(0), octaveOutTemp.getWritePointer(0), numSamples);
            
            int consumedDown = (int)std::floor(phaseDown + exactConsumedDown);
            phaseDown = (phaseDown + exactConsumedDown) - consumedDown;
            
            if (consumedDown > slideDownValid) consumedDown = slideDownValid;
            
            int remainingDown = slideDownValid - consumedDown;
            for (int i = 0; i < remainingDown; ++i) {
                slideDown.setSample(0, i, slideDown.getSample(0, consumedDown + i));
            }
            slideDownValid = remainingDown;
        } else {
            // Underrun! The pre-filled latency should prevent this in steady state.
            octaveOutTemp.clear();
        }
    }
    
    // std::cout << "      [processBlock] Setting target parameters smoothing..." << std::endl;
    // Target parameters smoothing
    current_predelay.setTargetValue(vpredelay);
    current_moddepth.setTargetValue(vmoddepth);
    current_modspeed.setTargetValue(vmodspeed);
    if (freeze) {
        current_freezeDecay.setTargetValue(1.0f);
    } else {
        current_freezeDecay.setTargetValue(vdecay);
    }
    
    if (odOn) {
        current_ODswell.setTargetValue(setOD);
    } else {
        current_ODswell.setTargetValue(0.4f);
    }

    // std::cout << "      [processBlock] Main DSP loop..." << std::endl;
    // --- MAIN DSP LOOP (HOST RATE) ---
    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : channelDataL;

    for (int i = 0; i < numSamples; ++i)
    {
        float fade = bypassFade.getNextValue();

        // Parameter smoothing applied every sample
        reverb.setPreDelay(current_predelay.getNextValue());
        reverb.setTankModDepth(current_moddepth.getNextValue() * 8.0f);
        reverb.setTankModSpeed(0.3f + current_modspeed.getNextValue() * 15.0f);
        reverb.setDecay(current_freezeDecay.getNextValue());
        
        float currentOD = current_ODswell.getNextValue();
        if (odOn) {
            overdriveLeft.SetDrive(currentOD);
            overdriveRight.SetDrive(currentOD);
            if (currentOD < 0.41f && !momentary_effect) {
                odOn = false;
            }
        }

        float dryMix = current_dryMix.getNextValue();
        float wetMix = current_wetMix.getNextValue();

        float inputL = channelDataL[i];
        float inputR = channelDataR[i];
        
        // Sum to mono for EarthPedal algorithm
        float monoIn = (inputL + inputR) * 0.5f;

        float reverb_in = monoIn;
        if (effect_mode != 0) {
            if ((footswitch_mode == 2 && momentary_effect) || (footswitch_mode != 2)) {
                reverb_in = octaveOutTemp.getSample(0, i);
            }
        }

        // Calculate Reverb
        reverb.process(reverb_in, reverb_in); // L/R identical input for mono source

        float reverbLeftOut = reverb.getLeftOutput();  
        float reverbRightOut = reverb.getRightOutput();
        float effectLeftOut = 0.0f;
        float effectRightOut = 0.0f;

        if (odOn) {
            effectLeftOut = overdriveLeft.Process(reverbLeftOut * 0.25f) * (1.0f - (currentOD * currentOD * 2.8f - 0.1296f));
            effectRightOut = overdriveRight.Process(reverbRightOut * 0.25f) * (1.0f - (currentOD * currentOD * 2.8f - 0.1296f));
        } else {
            effectLeftOut = reverbLeftOut;
            effectRightOut = reverbRightOut;
        }
        // Final Mix (using delayed dry signal)
        float delayedInputL = dryDelayL.popSample((int)dryDelayL.getDelay());
        float delayedInputR = dryDelayR.popSample((int)dryDelayR.getDelay());
        dryDelayL.pushSample(inputL);
        dryDelayR.pushSample(inputR);

        float leftOutput = delayedInputL * dryMix + effectLeftOut * wetMix * 0.4f;
        float rightOutput = delayedInputR * dryMix + effectRightOut * wetMix * 0.4f;

        // Apply bypass crossfade
        channelDataL[i] = inputL * fade + leftOutput * (1.0f - fade);
        if (buffer.getNumChannels() > 1) {
            channelDataR[i] = inputR * fade + rightOutput * (1.0f - fade);
        }
    }
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
