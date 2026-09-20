//
// Shared biquad shelf filters (RBJ audio EQ cookbook).
//
// Single implementation used by the octave shelves in both targets. Replaces
// the previous split (cycfi q in Web, juce::dsp::IIR in the VST). Designed at
// 48 kHz because the shelves process the reconstructed 48 kHz octave signal.
//
#pragma once

#include <cmath>

namespace earth {

inline constexpr double kPi = 3.14159265358979323846;

struct Biquad {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    float process(float x) {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() { z1 = 0.0f; z2 = 0.0f; }
};

inline Biquad makeHighShelf(double sampleRate, double f0, double q, double gainDb) {
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f0 / sampleRate;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) - (A - 1.0) * cw + beta;
    Biquad qq;
    qq.b0 = float(A * ((A + 1.0) + (A - 1.0) * cw + beta) / a0);
    qq.b1 = float(-2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0);
    qq.b2 = float(A * ((A + 1.0) + (A - 1.0) * cw - beta) / a0);
    qq.a1 = float(2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0);
    qq.a2 = float(((A + 1.0) - (A - 1.0) * cw - beta) / a0);
    return qq;
}

inline Biquad makeLowShelf(double sampleRate, double f0, double q, double gainDb) {
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f0 / sampleRate;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) + (A - 1.0) * cw + beta;
    Biquad qq;
    qq.b0 = float(A * ((A + 1.0) - (A - 1.0) * cw + beta) / a0);
    qq.b1 = float(2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0);
    qq.b2 = float(A * ((A + 1.0) - (A - 1.0) * cw - beta) / a0);
    qq.a1 = float(-2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0);
    qq.a2 = float(((A + 1.0) + (A - 1.0) * cw - beta) / a0);
    return qq;
}

} // namespace earth
