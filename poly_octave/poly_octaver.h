#pragma once

#include <array>
#include <cstddef>

#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>

#include "multirate.h"
#include "octave_generator.h"

namespace poly_octave {

/*
Standalone polyphonic octaver extracted from Earth.

Algorithm:
- 6-sample block accumulation at full sample-rate.
- Decimation to reduced-rate processing.
- Multi-band analytic filtering + phase scaling for octave up/down voices.
- Voice summation and interpolation back to full sample-rate.

Modes:
- Off: octave path disabled.
- Up: octave-up only.
- UpDown: octave-up + two down-octave voices.

Latency/structure:
- Uses the original 6-sample multirate path for low CPU usage and behavior parity.
- Output is emitted through the same buffered reconstruction path used by Earth.

Intended use:
- Reusable embedded mono octave effect module, independent from pedal UI/hardware code.
*/
class PolyOctaver
{
  public:
    enum class Mode
    {
        Off = 0,
        Up = 1,
        UpDown = 2
    };

    void Init(float sample_rate);

    void SetMode(Mode mode) { mode_ = mode; }
    void SetDryBlend(float amount) { dry_blend_ = amount; }
    void SetUpGain(float gain) { up_gain_ = gain; }
    void SetDown1Gain(float gain) { down1_gain_ = gain; }
    void SetDown2Gain(float gain) { down2_gain_ = gain; }
    void SetInternalDryEnabled(bool enabled) { internal_dry_enabled_ = enabled; }

    float ProcessMono(float in);
    void ProcessBlockMono(const float* in, float* out, std::size_t size);

    void Reset();

  private:
    using highshelf_t = cycfi::q::highshelf;
    using lowshelf_t = cycfi::q::lowshelf;

    Decimator2      decimator_;
    Interpolator    interpolator_;
    OctaveGenerator octave_generator_;

    highshelf_t eq1_{-11, 140_Hz, 48000};
    lowshelf_t  eq2_{5, 160_Hz, 48000};

    std::array<float, kResampleFactor> input_bin_{};
    std::array<float, kResampleFactor> output_bin_{};
    std::size_t                         bin_counter_ = 0;

    Mode  mode_ = Mode::Off;
    float dry_blend_ = 0.5f;
    float up_gain_ = 2.0f;
    float down1_gain_ = 2.0f;
    float down2_gain_ = 2.0f;
    bool  internal_dry_enabled_ = true;

    float sample_rate_ = 48000.0f;

    // Optional extension point: a future compile-time HQ path could bypass
    // the multirate stage and process every input sample directly.
};

} // namespace poly_octave
