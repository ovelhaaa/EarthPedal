//
// EarthPedal shared DSP core — timebase model.
//
// LegacySrInvariant is the production default. It is NOT a workaround: it is a
// deliberate decision to keep the historical Earth identity (the sound the Web
// build has at 48 kHz) while making it independent of the host sample rate.
//
#pragma once

#include "EarthRateContext.h"

namespace earth {

// The effective tank rate the hardware had at its native 48 kHz host rate
// (32000 / 48000). This constant encodes the historical identity.
inline constexpr double kLegacyHostRate = 48000.0;
inline constexpr double kLegacyTankRate = 32000.0;
inline constexpr double kLegacyTankRatio = kLegacyTankRate / kLegacyHostRate; // 2/3

enum class TimebaseModel {
    // Production default. Tank timing/filters/modulation referenced to
    // host * 2/3, so all real-time behaviour matches the golden 48 kHz build at
    // every host rate. Bit-exact with the golden at 48 kHz.
    LegacySrInvariant = 0,

    // Textbook interpretation: tank referenced to host. Sample-rate invariant,
    // but a musically different reverb (1.5x longer tail at 48 kHz). Research /
    // tests only; not exposed in the product UI.
    Correct = 1
};

inline EarthRateContext makeRateContext(double hostSampleRate,
                                        TimebaseModel model = TimebaseModel::LegacySrInvariant) {
    EarthRateContext rc;
    rc.processSampleRate = hostSampleRate;

    switch (model) {
        case TimebaseModel::LegacySrInvariant:
            rc.timingReferenceRate = hostSampleRate * kLegacyTankRatio;
            break;
        case TimebaseModel::Correct:
            rc.timingReferenceRate = hostSampleRate;
            break;
    }

    rc.filterSampleRate = rc.timingReferenceRate;
    rc.modulationSampleRate = rc.timingReferenceRate;
    return rc;
}

} // namespace earth
