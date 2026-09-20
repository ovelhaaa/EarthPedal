#include "EarthDSPCore.h"

#include <algorithm>
#include <cmath>

namespace earth {
namespace {

float sizeToTimeScale(ReverbSize size) {
    switch (size) {
        case ReverbSize::Small: return 1.0f;
        case ReverbSize::Medium: return 2.0f;
        case ReverbSize::Large: return 4.0f;
    }
    return 4.0f;
}

// Smoothing windows (seconds). Kept in the core so Web and JUCE ramp
// identically. Values match the previous VST behaviour.
constexpr double kParamSmoothingSeconds = 0.005;
constexpr double kMixSmoothingSeconds = 0.005;

// Historical Earth output headroom applied to the wet signal (earth.cpp:543).
constexpr float kWetHeadroom = 0.4f;

// Energy-preserving dry/wet crossfade (earth.cpp:405-418).
inline void mixGains(float mix, float& dryGain, float& wetGain) {
    const float x2 = 1.0f - mix;
    const float a = mix * x2;
    const float b = a * (1.0f + 1.4186f * a);
    const float c = b + mix;
    const float d = b + x2;
    wetGain = c * c;
    dryGain = d * d;
}

} // namespace

void EarthDSPCore::prepare(double sampleRate, int /*maximumBlockSize*/) {
    processSampleRate_ = sampleRate;
    rateContext_ = makeRateContext(sampleRate, timebaseModel_);

    applyRateContext();

    // Fixed historical initialisation (earth.cpp:642-657). These do not depend
    // on user parameters and must not be re-derived per block.
    reverb_.setInputFilterLowCutoffPitch(0.0f);   // 13.75 Hz
    reverb_.setInputFilterHighCutoffPitch(10.0f); // 14080 Hz
    reverb_.enableInputDiffusion(true);
    reverb_.setTankDiffusion(0.7f);
    reverb_.setTankFilterLowCutFrequency(0.0f);
    reverb_.setTankFilterHighCutFrequency(10.0f);
    reverb_.setTankModShape(0.5f);

    preDelay_.setTime(sampleRate, kParamSmoothingSeconds);
    decay_.setTime(sampleRate, kParamSmoothingSeconds);
    modDepth_.setTime(sampleRate, kParamSmoothingSeconds);
    modSpeed_.setTime(sampleRate, kParamSmoothingSeconds);
    mix_.setTime(sampleRate, kMixSmoothingSeconds);
    damp_.setTime(sampleRate, kParamSmoothingSeconds);

    lastTimeScale_ = -1.0f;
    lastDamp_ = -1.0f;
    inputDiffusionApplied_ = false;

    prepared_ = true;
    reset();
}

void EarthDSPCore::reset() {
    if (!prepared_) return;
    reverb_.clear();
    setParameters(params_);
    snapParameters();
}

void EarthDSPCore::setTimebaseModel(TimebaseModel model) {
    timebaseModel_ = model;
    if (prepared_) {
        rateContext_ = makeRateContext(processSampleRate_, timebaseModel_);
        applyRateContext();
        reverb_.clear();
        snapParameters();
    }
}

void EarthDSPCore::applyRateContext() {
    reverb_.setRateContext(rateContext_);
}

void EarthDSPCore::applyStaticParameters(const EarthParameters& p) {
    const float ts = sizeToTimeScale(p.reverbSize);
    if (ts != lastTimeScale_) {
        reverb_.setTimeScale(ts);
        lastTimeScale_ = ts;
    }
    if (!inputDiffusionApplied_ || p.inputDiffusion != lastInputDiffusion_) {
        reverb_.enableInputDiffusion(p.inputDiffusion);
        lastInputDiffusion_ = p.inputDiffusion;
        inputDiffusionApplied_ = true;
    }
}

void EarthDSPCore::applyDamp(float damp) {
    if (damp < 0.5f) {
        const float high = damp * 2.0f;
        reverb_.setInputFilterHighCutoffPitch(7.0f * high + 3.0f); // 3..10
    } else {
        const float low = (damp - 0.5f) * 2.0f;
        reverb_.setInputFilterLowCutoffPitch(9.0f * low); // 0..9
    }
    lastDamp_ = damp;
}

void EarthDSPCore::setParameters(const EarthParameters& parameters) {
    params_ = parameters;

    preDelay_.setTarget(parameters.preDelaySeconds);
    modDepth_.setTarget(parameters.modulationDepth);
    modSpeed_.setTarget(parameters.modulationSpeed);
    mix_.setTarget(parameters.mix);
    damp_.setTarget(parameters.damp);

    const bool freeze = parameters.performanceActive &&
                        parameters.performanceMode == PerformanceMode::Freeze;
    decay_.setTarget(freeze ? 1.0f : parameters.decay);

    if (prepared_) {
        applyStaticParameters(parameters);
    }
}

void EarthDSPCore::snapParameters() {
    preDelay_.snap(params_.preDelaySeconds);
    modDepth_.snap(params_.modulationDepth);
    modSpeed_.snap(params_.modulationSpeed);
    mix_.snap(params_.mix);
    damp_.snap(params_.damp);
    const bool freeze = params_.performanceActive &&
                        params_.performanceMode == PerformanceMode::Freeze;
    decay_.snap(freeze ? 1.0f : params_.decay);
    applyStaticParameters(params_);
    applyDamp(damp_.current);
}

void EarthDSPCore::process(const float* inputLeft, const float* inputRight,
                           float* outputLeft, float* outputRight, int numSamples) {
    if (!prepared_) return;

    const bool freeze = params_.performanceActive &&
                        params_.performanceMode == PerformanceMode::Freeze;
    decay_.setTarget(freeze ? 1.0f : params_.decay);

    for (int i = 0; i < numSamples; ++i) {
        const float inL = inputLeft[i];
        const float inR = inputRight[i];

        reverb_.setPreDelay(preDelay_.next());
        reverb_.setDecay(decay_.next());
        reverb_.setTankModDepth(modDepth_.next() * 8.0f);
        reverb_.setTankModSpeed(0.3f + modSpeed_.next() * 15.0f);

        const float damp = damp_.next();
        if (damp != lastDamp_) applyDamp(damp);

        // Octave branch is Off in G3: the reverb is excited by the mono input.
        const float mono = 0.5f * (inL + inR);
        reverb_.process(mono, mono);

        const float wetL = reverb_.getLeftOutput();
        const float wetR = reverb_.getRightOutput();

        float dryGain = 1.0f, wetGain = 1.0f;
        mixGains(mix_.next(), dryGain, wetGain);

        float outL = inL * dryGain + wetL * wetGain * kWetHeadroom;
        float outR = inR * dryGain + wetR * wetGain * kWetHeadroom;

        if (params_.bypass) {
            outL = inL;
            outR = inR;
        }

        outputLeft[i] = outL;
        outputRight[i] = outR;
    }
}

} // namespace earth
