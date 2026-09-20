//
// EarthPedal shared DSP core — canonical enums.
//
// The DSP never interprets raw integers (0/1/2/3). Wrappers translate their own
// UI/APVTS/Worklet values into these typed enums. See
// Apollo/docs/dsp_parity/stage_g_shared_core/PARAMETER_ADAPTERS.md.
//
#pragma once

namespace earth {

enum class OctaveMode {
    Off = 0,
    Up = 1,
    Down = 2,
    Both = 3
};

enum class ReverbSize {
    Small = 0,
    Medium = 1,
    Large = 2
};

enum class PerformanceMode {
    Freeze = 0,
    Overdrive = 1,
    Octave = 2
};

} // namespace earth
