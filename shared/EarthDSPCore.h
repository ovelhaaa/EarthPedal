//
// EarthPedal shared DSP core.
//
// Single source of truth for every sonic rule: parameter mapping, smoothing,
// mix law, pre-delay, damp, Dattorro initialisation and the timebase policy.
// The Web/WASM adapter and the JUCE adapter must contain no DSP logic.
//
// This stage (G3) implements the reverb/output path. The octave branch and the
// overdrive are added in G4/G5; until then they are explicit no-ops (octave
// Off == the production default, so the Golden test is unaffected).
//
// No JUCE, Emscripten, WebAudio or Daisy hardware dependencies.
//
#pragma once

#include "EarthParameters.h"
#include "EarthRateContext.h"
#include "EarthTimebase.h"
#include "Dattorro/Dattorro.hpp"
#include "Multirate/Multirate.h"
#include "Octave/OctaveGenerator.h"
#include "Filters/ShelfFilter.h"
#include "Resampling/FractionalLinearResampler.h"
#include "Effects/Overdrive.h"

#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace earth {

class EarthDSPCore {
public:
    EarthDSPCore() = default;

    // Allocates and initialises the DSP for the given host rate.
    void prepare(double sampleRate, int maximumBlockSize);

    // Clears all internal state (delay lines, filters, LFOs, smoothers).
    void reset();

    // Sets the parameter targets. Smoothing ramps from the current values.
    void setParameters(const EarthParameters& parameters);

    // Immediately applies the last parameter targets (no ramp). Used at
    // prepare time and by offline golden tests.
    void snapParameters();

    // In-place-capable stereo processing. inputLeft/Right and outputLeft/Right
    // may alias.
    void process(const float* inputLeft, const float* inputRight,
                 float* outputLeft, float* outputRight, int numSamples);

    // Algorithmic latency of the core in host samples. The reverb path is
    // zero-latency; the octave branch adds latency in G4.
    int getLatencySamples() const { return 0; }

    // --- research / diagnostics (not exposed in the product UI) -----------
    void setTimebaseModel(TimebaseModel model);
    TimebaseModel timebaseModel() const { return timebaseModel_; }
    const EarthRateContext& rateContext() const { return rateContext_; }

    // Direct read access for tests (delay timing inspection).
    const Dattorro& reverb() const { return reverb_; }
    double processSampleRate() const { return rateContext_.processSampleRate; }

private:
    // A minimal linear parameter smoother. Snapping (current = target) is used
    // before the first block so offline renders are deterministic.
    struct Smoothed {
        float current = 0.0f;
        float target = 0.0f;
        float step = 1.0f; // per-sample increment (signed)
        bool active = false;

        void setTime(double sampleRate, double seconds) {
            step = (seconds > 0.0) ? static_cast<float>(1.0 / (sampleRate * seconds)) : 1.0f;
        }
        void snap(float v) { current = target = v; active = false; }
        void setTarget(float v) {
            target = v;
            active = (current != v);
        }
        float next() {
            if (!active) return current;
            if (current < target) {
                current += std::abs(step);
                if (current >= target) { current = target; active = false; }
            } else {
                current -= std::abs(step);
                if (current <= target) { current = target; active = false; }
            }
            return current;
        }
    };

    void applyRateContext();
    void applyStaticParameters(const EarthParameters& p);
    void applyDamp(float damp);
    void updateShelves();

    // Octave branch in the canonical 48 kHz domain. `processOctave48Sample`
    // implements the exact earth.cpp/Web routing for one 48 kHz sample.
    float processOctave48Sample(float input);
    void processOctave48Block(const float* in, float* out, int count);

    TimebaseModel timebaseModel_ = TimebaseModel::LegacySrInvariant;
    EarthRateContext rateContext_;
    Dattorro reverb_ { 48000, 16, 4.0 };

    bool prepared_ = false;
    double processSampleRate_ = 48000.0;
    float lastTimeScale_ = -1.0f;
    float lastDamp_ = -1.0f;
    bool lastInputDiffusion_ = true;
    bool inputDiffusionApplied_ = false;

    // Mirrored target parameters.
    EarthParameters params_;

    Smoothed preDelay_;
    Smoothed decay_;
    Smoothed modDepth_;
    Smoothed modSpeed_;
    Smoothed mix_;
    Smoothed damp_;
    Smoothed odSwell_;
    Smoothed bypass_;
    Overdrive overdriveLeft_;
    Overdrive overdriveRight_;
    bool odOn_ = false;

    // Octave branch (canonical 48 kHz domain).
    static constexpr double kCanonicalRate = 48000.0;
    std::unique_ptr<OctaveGenerator> octave_;
    Decimator2 decimate_;
    Interpolator interpolate_;
    Biquad highShelf_;
    Biquad lowShelf_;
    std::array<float, resample_factor> buff_{};
    std::array<float, resample_factor> buffOut_{};
    int binCounter_ = 0;
    float lastEq1_ = -9999.0f;
    float lastEq2_ = -9999.0f;

    bool native48_ = true;
    FractionalLinearResampler upResampler_;
    FractionalLinearResampler downResampler_;
    std::vector<float> octaveIn48_;
    std::vector<float> octaveOut48_;
    std::vector<float> downFifo_;
    size_t downRead_ = 0;
};

} // namespace earth
