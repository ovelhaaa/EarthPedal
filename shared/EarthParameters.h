//
// EarthPedal shared DSP core — canonical parameter struct and defaults.
//
// This is the single source of truth for parameter meaning. The Web adapter and
// the JUCE adapter both map their native controls onto this struct; neither
// wrapper contains any sonic rule.
//
// Units are canonical and explicit:
//   preDelaySeconds  seconds, 0.0 .. 1.0
//   mix              0.0 .. 1.0 (energy-preserving crossfade, see EarthDSPCore)
//   decay            0.0 .. 1.0 tank feedback
//   modulationDepth  0.0 .. 1.0 mapped to excursion = value * 8 (hardware)
//   modulationSpeed  0.0 .. 1.0 mapped to Hz = 0.3 + value * 15 (hardware)
//   damp             0.0 .. 1.0 input tone control
//   *ShelfDb         dB gain of the octave shelves
//
#pragma once

#include "EarthEnums.h"

namespace earth {

struct EarthParameters {
    float preDelaySeconds = 0.0f;
    float mix = 0.5f;
    float decay = 0.877465f;

    float modulationDepth = 0.0625f;
    float modulationSpeed = 0.0466667f;

    float damp = 0.5f;

    float octaveHighShelfDb = -11.0f; // @ 140 Hz, designed at 48 kHz
    float octaveLowShelfDb = 5.0f;    // @ 160 Hz, designed at 48 kHz

    ReverbSize reverbSize = ReverbSize::Large;
    OctaveMode octaveMode = OctaveMode::Off;
    PerformanceMode performanceMode = PerformanceMode::Freeze;

    bool inputDiffusion = true;
    bool includeDryInOctavePath = true;
    bool performanceActive = false; // momentary footswitch held / automated
    bool bypass = false;

    // Canonical factory defaults reproduce the Golden Web@48k reference.
    static EarthParameters defaults() { return EarthParameters{}; }
};

} // namespace earth
