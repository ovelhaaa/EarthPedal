# RATE_CONTEXT — why `timingReferenceRate = host * 2/3` is deliberate

This document exists to stop a future "fix" that would silently destroy the
Earth sound. Read it before changing any sample-rate handling.

## The decision

Production uses `TimebaseModel::LegacySrInvariant`:

```
processSampleRate    = hostSampleRate
timingReferenceRate  = hostSampleRate * (32000 / 48000)     // == host * 2/3
filterSampleRate     = timingReferenceRate
modulationSampleRate = timingReferenceRate
```

The outer input filters, the pre-delay and the input diffusion all-passes stay
referenced to `processSampleRate`.

## Why not `timingReferenceRate = hostSampleRate` (the "correct" value)?

Because the historical Earth (the Web/WASM build that is the musical reference)
runs its tank at an effective **32000 Hz** rate while processing at 48000 Hz.
This was an implementation accident (`Dattorro1997Tank::maxSampleRate` was never
assigned), but the resulting 2/3 time compression is part of the instrument's
identity: tail length, early reflection pattern, RT60 and modulation depth were
all tuned/heard under it.

Stage F measured both options against the golden 48 kHz reference:

| model | tank rate | golden @48k | 48k→96k behaviour |
| --- | --- | --- | --- |
| `Legacy32k` (old production) | `min(host,32000)` | reference | **changes** (delay 399→199.5 ms, RT30 +19 %) |
| `Correct` | `host` | null +3.2 dB, corr 0.04 | stable, but **1.5x longer tail** |
| **`LegacySrInvariant`** | `host * 2/3` | **null −553 dB (bit-exact)** | **stable** |

`Correct` is mathematically clean but is a different reverb. `Legacy32k` keeps
the sound only at 48 kHz and changes with every host rate.

`LegacySrInvariant` is the only model that is both sample-rate invariant and
bit-identical to the preferred 48 kHz sound.

## The constants

```cpp
constexpr double kLegacyHostRate = 48000.0;
constexpr double kLegacyTankRate = 32000.0;
constexpr double kLegacyTankRatio = kLegacyTankRate / kLegacyHostRate; // 2/3
```

`32000` is the effective tank rate the hardware had at its native 48 kHz host
rate. It is a fixed property of the reference sound, not a limit to remove.

## What each rate drives

| Rate | Consumers |
| --- | --- |
| `processSampleRate` | outer input LPF/HPF, input DC blockers, pre-delay, input diffusion all-passes, freeze fade duration |
| `timingReferenceRate` | tank delay lengths, output taps, LFO excursion (samples) |
| `filterSampleRate` | tank high/low cut one-pole coefficients |
| `modulationSampleRate` | `TriSawLFO` phase step |

## Two intentional "do not fix" behaviours

1. **Tank filters are processed at `processSampleRate` but their coefficients
   are computed at `filterSampleRate`.** Filtering a 48 kHz signal with
   coefficients designed for 32 kHz shifts the realised cutoff (e.g. 14080 Hz →
   21120 Hz). This is the historical sound and is exactly what
   `LegacySrInvariant` reproduces at every host rate.

2. **`OnePoleFilter::setSampleRate()` early-returns when the cutoff is
   unchanged.** Filters whose cutoff is not re-set (the DC blockers) therefore
   keep 32 kHz coefficients even at high host rates. During Stage G this was
   briefly "fixed" and the golden immediately failed (max|diff| 2.9e-4). It was
   reverted; the comment in `OnePoleFilters.hpp` now warns against changing it.

## Verification

* 48 kHz: `golden_test` reports `max|diff| = 0` for P0 and P1 (null −547 dB),
  i.e. the core is bit-identical to the frozen production reference.
* 44.1/48/96/192 kHz: `scaledLeftDelay1Time` is always 399.00 ms and RT30 stays
  within 5.21–5.38 s (Stage F tolerance ±4 %).
