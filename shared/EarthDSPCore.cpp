#include "EarthDSPCore.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>

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

// Overdrive (earth.cpp footswitch 2 / Apollo momentary). Base and active drive
// are the historical values; the swell smoothing is the Apollo behaviour.
constexpr float kOdBaseDrive = 0.4f;
constexpr float kOdActiveDrive = 0.6f;
constexpr float kOdReleaseThreshold = 0.41f;
constexpr double kOdSwellSeconds = 0.015;
constexpr double kBypassSmoothingSeconds = 0.01;

inline bool isOverdriveActive(const EarthParameters& p) {
    return p.performanceActive && p.performanceMode == PerformanceMode::Overdrive;
}

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
    odSwell_.setTime(sampleRate, kOdSwellSeconds);
    bypass_.setTime(sampleRate, kBypassSmoothingSeconds);

    lastTimeScale_ = -1.0f;
    lastDamp_ = -1.0f;
    inputDiffusionApplied_ = false;

    // Octave branch: always runs at the canonical 48 kHz domain.
    octave_ = std::make_unique<OctaveGenerator>(static_cast<float>(kCanonicalRate / resample_factor));
    native48_ = std::fabs(sampleRate - kCanonicalRate) < 1e-6;
    upResampler_.configure(sampleRate, kCanonicalRate);
    downResampler_.configure(kCanonicalRate, sampleRate);
    lastEq1_ = lastEq2_ = -9999.0f;
    updateShelves();

    prepared_ = true;
    reset();
}

void EarthDSPCore::reset() {
    if (!prepared_) return;
    reverb_.clear();

    decimate_ = Decimator2();
    interpolate_ = Interpolator();
    if (octave_) {
        octave_ = std::make_unique<OctaveGenerator>(static_cast<float>(kCanonicalRate / resample_factor));
    }
    buff_.fill(0.0f);
    buffOut_.fill(0.0f);
    binCounter_ = 0;
    highShelf_.reset();
    lowShelf_.reset();
    overdriveLeft_.init();
    overdriveRight_.init();
    odOn_ = false;
    upResampler_.reset();
    downResampler_.reset();
    octaveIn48_.clear();
    octaveOut48_.clear();
    downFifo_.clear();
    downRead_ = 0;

    // Prime the host<->48k resamplers so the first real block never underruns.
    // Priming is done here (block-size independent), which keeps the octave
    // branch block-size invariant at non-48k rates.
    if (!native48_ && octave_) {
        octaveIn48_.clear();
        for (int k = 0; k < 64; ++k) {
            upResampler_.push(0.0f, [this](float y) { octaveIn48_.push_back(y); });
        }
        octaveOut48_.assign(octaveIn48_.size(), 0.0f);
        processOctave48Block(octaveIn48_.data(), octaveOut48_.data(),
                             static_cast<int>(octaveIn48_.size()));
        for (float s : octaveOut48_) {
            downResampler_.push(s, [this](float y) { downFifo_.push_back(y); });
        }
        octaveIn48_.clear();
        octaveOut48_.clear();
    }

    setParameters(params_);
    snapParameters();
}

void EarthDSPCore::updateShelves() {
    if (params_.octaveHighShelfDb != lastEq1_) {
        highShelf_ = makeHighShelf(kCanonicalRate, 140.0, 0.707, params_.octaveHighShelfDb);
        lastEq1_ = params_.octaveHighShelfDb;
    }
    if (params_.octaveLowShelfDb != lastEq2_) {
        lowShelf_ = makeLowShelf(kCanonicalRate, 160.0, 0.707, params_.octaveLowShelfDb);
        lastEq2_ = params_.octaveLowShelfDb;
    }
}

float EarthDSPCore::processOctave48Sample(float input) {
    const OctaveMode mode = params_.octaveMode;
    buff_[binCounter_] = input;

    if (binCounter_ > 4) {
        std::span<const float, resample_factor> chunk(&buff_[0], resample_factor);
        const float sample = decimate_(chunk);
        octave_->update(sample);

        float oct = 0.0f;
        if (mode == OctaveMode::Up || mode == OctaveMode::Both) oct += octave_->up1() * 2.0f;
        if (mode == OctaveMode::Down || mode == OctaveMode::Both) {
            oct += octave_->down1() * 2.0f;
            oct += octave_->down2() * 2.0f;
        }

        const auto outChunk = interpolate_(oct);
        for (size_t j = 0; j < outChunk.size(); ++j) {
            float v = lowShelf_.process(highShelf_.process(outChunk[j]));
            if (params_.includeDryInOctavePath) v += 0.5f * buff_[j];
            buffOut_[j] = (mode != OctaveMode::Off) ? v : 0.0f;
        }
    }

    binCounter_ += 1;
    if (binCounter_ > 5) binCounter_ = 0;
    return buffOut_[binCounter_];
}

void EarthDSPCore::processOctave48Block(const float* in, float* out, int count) {
    for (int i = 0; i < count; ++i) out[i] = processOctave48Sample(in[i]);
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
    odSwell_.setTarget(isOverdriveActive(parameters) ? kOdActiveDrive : kOdBaseDrive);
    bypass_.setTarget(parameters.bypass ? 1.0f : 0.0f);

    if (prepared_) {
        applyStaticParameters(parameters);
        updateShelves();
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
    odSwell_.snap(isOverdriveActive(params_) ? kOdActiveDrive : kOdBaseDrive);
    bypass_.snap(params_.bypass ? 1.0f : 0.0f);
    odOn_ = isOverdriveActive(params_);
    applyStaticParameters(params_);
    updateShelves();
    applyDamp(damp_.current);
}

void EarthDSPCore::process(const float* inputLeft, const float* inputRight,
                           float* outputLeft, float* outputRight, int numSamples) {
    if (!prepared_) return;

    const bool freeze = params_.performanceActive &&
                        params_.performanceMode == PerformanceMode::Freeze;
    decay_.setTarget(freeze ? 1.0f : params_.decay);
    const bool odActive = isOverdriveActive(params_);
    odSwell_.setTarget(odActive ? kOdActiveDrive : kOdBaseDrive);

    const bool octaveActive = params_.octaveMode != OctaveMode::Off;
    const bool resampled = octaveActive && !native48_;

    if (resampled) {
        // Convert the block to the canonical 48 kHz domain, run the octave
        // branch, then convert back. At 48 kHz this path is bypassed entirely.
        octaveIn48_.clear();
        for (int i = 0; i < numSamples; ++i) {
            const float mono = 0.5f * (inputLeft[i] + inputRight[i]);
            upResampler_.push(mono, [this](float y) { octaveIn48_.push_back(y); });
        }
        octaveOut48_.assign(octaveIn48_.size(), 0.0f);
        processOctave48Block(octaveIn48_.data(), octaveOut48_.data(),
                             static_cast<int>(octaveIn48_.size()));
        for (float s : octaveOut48_) {
            downResampler_.push(s, [this](float y) { downFifo_.push_back(y); });
        }
    }

    for (int i = 0; i < numSamples; ++i) {
        const float inL = inputLeft[i];
        const float inR = inputRight[i];

        reverb_.setPreDelay(preDelay_.next());
        reverb_.setDecay(decay_.next());
        reverb_.setTankModDepth(modDepth_.next() * 8.0f);
        reverb_.setTankModSpeed(0.3f + modSpeed_.next() * 15.0f);

        const float damp = damp_.next();
        if (damp != lastDamp_) applyDamp(damp);

        const float mono = 0.5f * (inL + inR);

        float reverbIn;
        if (!octaveActive) {
            reverbIn = mono;
        } else if (native48_) {
            reverbIn = processOctave48Sample(mono);
        } else if (downRead_ < downFifo_.size()) {
            reverbIn = downFifo_[downRead_++];
        } else {
            reverbIn = mono; // startup underrun; self-corrects in steady state
        }
        reverb_.process(reverbIn, reverbIn);

        float wetL = reverb_.getLeftOutput();
        float wetR = reverb_.getRightOutput();

        // Overdrive (earth.cpp:533-541): applied to the wet signal with the
        // historical level-compensation curve.
        const float od = odSwell_.next();
        if (odActive) {
            odOn_ = true;
        } else if (od < kOdReleaseThreshold) {
            odOn_ = false;
        }
        if (odOn_) {
            overdriveLeft_.setDrive(od);
            overdriveRight_.setDrive(od);
            const float comp = 1.0f - (od * od * 2.8f - 0.1296f);
            wetL = overdriveLeft_.process(wetL * 0.25f) * comp;
            wetR = overdriveRight_.process(wetR * 0.25f) * comp;
        }

        float dryGain = 1.0f, wetGain = 1.0f;
        mixGains(mix_.next(), dryGain, wetGain);

        float outL = inL * dryGain + wetL * wetGain * kWetHeadroom;
        float outR = inR * dryGain + wetR * wetGain * kWetHeadroom;

        // Bypass crossfade (0 = active, 1 = bypassed), smoothed to avoid clicks.
        const float bp = bypass_.next();
        if (bp > 0.0f) {
            outL = outL * (1.0f - bp) + inL * bp;
            outR = outR * (1.0f - bp) + inR * bp;
        }

        outputLeft[i] = outL;
        outputRight[i] = outR;
    }

    if (downRead_ > 0) {
        downFifo_.erase(downFifo_.begin(),
                        downFifo_.begin() + static_cast<std::ptrdiff_t>(std::min(downRead_, downFifo_.size())));
        downRead_ = 0;
    }
}

} // namespace earth
