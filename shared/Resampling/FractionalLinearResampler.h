//
// Provisional fractional linear resampler for the octave branch (Stage G, G4).
//
// The octave branch's canonical domain is 48 kHz. This resampler converts
// host <-> 48 kHz so the musical DSP always sees 48 kHz. It is intentionally
// simple: the 48 kHz octave signal is band-limited to ~3.6 kHz by the multirate
// interpolator, so linear interpolation is accurate enough for the branch.
//
// A bit-exact shared resampler matching JUCE/Emscripten is a separate step
// (G4b/G2). At exactly 48 kHz the core bypasses this class entirely and the
// octave path is bit-identical to the native Web/earth reference.
//
#pragma once

namespace earth {

// Streams input samples at `inRate` and emits interpolated samples at
// `outRate`. Past-only (no lookahead), so it adds no algorithmic latency beyond
// one sample of interpolation.
class FractionalLinearResampler {
public:
    void configure(double inRate, double outRate) {
        step_ = inRate / outRate;
        reset();
    }

    void reset() {
        prev_ = 0.0f;
        cur_ = 0.0f;
        pos_ = 0.0;
        primed_ = false;
    }

    // Pushes one input sample and calls emit(y) for each output sample.
    template <typename Emit>
    void push(float s, Emit&& emit) {
        if (!primed_) {
            prev_ = s;
            cur_ = s;
            primed_ = true;
            return;
        }
        prev_ = cur_;
        cur_ = s;
        while (pos_ < 1.0) {
            const float y = prev_ + static_cast<float>(pos_) * (cur_ - prev_);
            emit(y);
            pos_ += step_;
        }
        pos_ -= 1.0;
    }

private:
    double step_ = 1.0;
    double pos_ = 0.0;
    float prev_ = 0.0f;
    float cur_ = 0.0f;
    bool primed_ = false;
};

} // namespace earth
