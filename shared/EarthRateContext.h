//
// EarthPedal shared DSP core — explicit sample-rate roles.
//
// Historically the tank used a single `sampleRate` for five different meanings,
// which made the reverb character depend on the host rate. Stage F separated
// them. See Apollo/docs/dsp_parity/stage_f_timebase/TIMEBASE_ANALYSIS.md and
// Apollo/docs/dsp_parity/stage_g_shared_core/RATE_CONTEXT.md.
//
#pragma once

namespace earth {

struct EarthRateContext {
    // The rate `process()` is actually called at. Real-time durations (the
    // freeze crossfade, the outer input filters, the pre-delay and the input
    // diffusion all-passes) are referenced to this.
    double processSampleRate = 48000.0;

    // The rate used to convert the original Dattorro delay/tap lengths into
    // samples and to express the LFO excursion. This is NOT necessarily the
    // process rate: see EarthTimebase.h.
    double timingReferenceRate = 48000.0;

    // The rate used to compute the tank's one-pole filter coefficients. The
    // filters themselves are processed at processSampleRate; the historical
    // sound deliberately computes their coefficients at the tank timing rate.
    // DO NOT "fix" this without reading RATE_CONTEXT.md.
    double filterSampleRate = 48000.0;

    // The rate handed to TriSawLFO, so its phase advance corresponds to the
    // intended (and historically heard) Hz.
    double modulationSampleRate = 48000.0;
};

} // namespace earth
