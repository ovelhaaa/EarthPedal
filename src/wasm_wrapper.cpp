// Web/WASM adapter for the shared EarthDSPCore.
//
// This is a thin adapter: it translates the AudioWorklet parameters into
// earth::EarthParameters and calls the shared core. It contains no DSP logic.
// The previous implementation is kept in wasm_wrapper_legacy.cpp for rollback
// until the Web build is validated against the core.
//
#include <emscripten/bind.h>

#include <cstdint>

#include "EarthDSPCore.h"

using earth::EarthDSPCore;
using earth::EarthParameters;
using earth::OctaveMode;
using earth::ReverbSize;

namespace {

OctaveMode mapOctaveMode(int mode) {
    switch (mode) {
        case 1: return OctaveMode::Up;
        case 2: return OctaveMode::Down;
        case 3: return OctaveMode::Both;
        default: return OctaveMode::Off;
    }
}

ReverbSize mapReverbSize(int size) {
    switch (size) {
        case 0: return ReverbSize::Small;
        case 1: return ReverbSize::Medium;
        default: return ReverbSize::Large;
    }
}

} // namespace

class EarthAudioProcessor {
public:
    explicit EarthAudioProcessor(float sampleRate) {
        core_.prepare(sampleRate, 512);
        params_ = EarthParameters::defaults();
        core_.setParameters(params_);
        core_.snapParameters();
    }

    void setPreDelay(float value) { params_.preDelaySeconds = value; apply(); }
    void setMix(float value) { params_.mix = value; apply(); }
    void setDecay(float value) { params_.decay = value; apply(); }
    void setModDepth(float value) { params_.modulationDepth = value; apply(); }
    void setModSpeed(float value) { params_.modulationSpeed = value; apply(); }
    void setFilter(float value) { params_.damp = value; apply(); }
    void setEq1Gain(float gain) { params_.octaveHighShelfDb = gain; apply(); }
    void setEq2Gain(float gain) { params_.octaveLowShelfDb = gain; apply(); }
    void setReverbSize(int size) { params_.reverbSize = mapReverbSize(size); apply(); }
    void setOctaveMode(int mode) { params_.octaveMode = mapOctaveMode(mode); apply(); }
    void setDisableInputDiffusion(bool disabled) { params_.inputDiffusion = !disabled; apply(); }

    void process(uintptr_t inL_ptr, uintptr_t inR_ptr,
                 uintptr_t outL_ptr, uintptr_t outR_ptr, int size) {
        const float* inL = reinterpret_cast<const float*>(inL_ptr);
        const float* inR = reinterpret_cast<const float*>(inR_ptr);
        float* outL = reinterpret_cast<float*>(outL_ptr);
        float* outR = reinterpret_cast<float*>(outR_ptr);
        core_.process(inL, inR, outL, outR, size);
    }

private:
    void apply() { core_.setParameters(params_); }

    EarthDSPCore core_;
    EarthParameters params_;
};

EMSCRIPTEN_BINDINGS(earth_module) {
    emscripten::class_<EarthAudioProcessor>("EarthAudioProcessor")
        .constructor<float>()
        .function("setPreDelay", &EarthAudioProcessor::setPreDelay)
        .function("setMix", &EarthAudioProcessor::setMix)
        .function("setDecay", &EarthAudioProcessor::setDecay)
        .function("setModDepth", &EarthAudioProcessor::setModDepth)
        .function("setModSpeed", &EarthAudioProcessor::setModSpeed)
        .function("setFilter", &EarthAudioProcessor::setFilter)
        .function("setReverbSize", &EarthAudioProcessor::setReverbSize)
        .function("setOctaveMode", &EarthAudioProcessor::setOctaveMode)
        .function("setDisableInputDiffusion", &EarthAudioProcessor::setDisableInputDiffusion)
        .function("setEq1Gain", &EarthAudioProcessor::setEq1Gain)
        .function("setEq2Gain", &EarthAudioProcessor::setEq2Gain)
        .function("process", &EarthAudioProcessor::process, emscripten::allow_raw_pointers());
}
