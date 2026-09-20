# Recommendation — Definitive Timebase

## Decision

**Adopt Model C — `LEGACY_SOUND_SR_INVARIANT`.**

Rejected:

* **A. `LEGACY_32K`** — preserves the 48 kHz sound but is not sample-rate
  invariant: delay times, RT60 and LFO rates all change with the host
  (`leftDelay1` 399 → 199.5 ms, LFO 0.15 → 0.60 Hz, RT30 5.3 → 6.3 s from
  48 → 96 kHz). Shipping this means the plugin sounds different in every DAW.
* **B. `DATTORRO_CORRECT`** — the textbook interpretation, SR-invariant, but it
  **does not reproduce the preferred sound**: at 48 kHz the tail is 1.5x
  longer, RT60 differs by 11-24 %, and the IR correlates only 0.04 with the
  golden reference. Technically clean, musically a different reverb.
* **D. Hybrid** — not needed; Model C already reproduces the golden exactly
  while fixing the rate dependence, and no hybrid measured better.

## Why Model C

It is the only model that satisfies all four goals simultaneously:

| Goal | `legacy32k` | `correct` | `invariant` |
| --- | --- | --- | --- |
| Match current Web @ 48 kHz | ✅ (it is the ref) | ❌ | ✅ bit-exact |
| Same sound 44.1/48/96 | ❌ | ✅ | ✅ |
| Delay times constant in seconds | ❌ | ✅ | ✅ |
| Preserve RT60 / LFO / timbre | ✅ @48k only | ❌ | ✅ at all rates |

The implementation is a separation of the currently conflated `sampleRate`
into explicit roles, with the legacy behaviour expressed as a *ratio*, not a
hardcoded clamp.

## Implementation plan (for Stage G — shared core)

Introduce named roles. `kLegacyTankRatio = 32000/48000 = 2/3` is the only magic
number and must be documented as "the effective tank rate the hardware had at
its native 48 kHz".

```cpp
processSampleRate     = hostSampleRate;
timingReferenceRate   = hostSampleRate * kLegacyTankRatio;  // tank delays, taps, excursion
filterSampleRate      = timingReferenceRate;                // tank cut + DC filters
modulationSampleRate  = timingReferenceRate;                // TriSawLFO setSamplerate()
// outer Dattorro input filters / input APFs / pre-delay stay at processSampleRate
```

Concrete changes to the shared tank:

1. Replace `maxSampleRate`/clamp with `timingReferenceRate` used for
   `sampleRateScale`, `scaledOutputTaps`, and `lfoExcursion`.
2. `fadeStep = 1 / processSampleRate` (freeze fade stays 0.667 s).
3. Call `setSampleRate(filterSampleRate)` on the four tank cut filters (fixes
   the latent "frozen at 32000" bug **without changing the golden 48 kHz
   sound**, because at 48 kHz `filterSampleRate == 32000`).
4. Call `lfoN.setSamplerate(modulationSampleRate)` in `setSampleRate`.
5. Keep a `TimebaseModel` enum (`Legacy32k`, `Correct`, `LegacySrInvariant`)
   with `LegacySrInvariant` as the default, so the other models remain testable
   and production behaviour is explicitly chosen rather than accidental.

Expected validation (already measured):
* 48 kHz: bit-exact with `golden_legacy_48k` (null ≤ −140 dB; measured
  −553 dB).
* 44.1/96/192 kHz: RT30 within ±4 %, `leftDelay1` 399.0 ms, LFO1 0.150 Hz.
* No regression in freeze, overdrive, bypass or automation.

## What NOT to do

* Do **not** keep the `maxSampleRate = 32000` clamp as the mechanism; it makes
  the identity a function of the host rate.
* Do **not** apply `timeScale *= 2/3` as the fix (see `TIMEBASE_ANALYSIS.md`
  §3): taps, excursion, filters and LFO would remain inconsistent.
* Do **not** silently switch to `correct`; it audibly changes the instrument.
* Do **not** remap the `decay` parameter for Model C; the audit shows the
  RT-vs-decay curve is preserved automatically (`DECAY_AUDIT.md`).

## Production untouched in Stage F

No file under `Apollo/Source/` or `src/` was modified in this stage. The three
models live only in `Apollo/tools/dsp_parity/timebase_harness.cpp`, which
overrides public tank members before `setSampleRate()`. The `maxSampleRate`
clamp, all APVTS ids, defaults and parameter polarities are unchanged.

## Proposed Stage G

1. Create `shared/EarthDSPCore` with the explicit rate roles above and a
   `TimebaseModel` parameter defaulting to `LegacySrInvariant`.
2. Move the Web/WASM wrapper and the Apollo `PluginProcessor` onto the core as
   thin adapters (no DSP logic in the wrappers).
3. Unify the octave enum (`Off/Up/Down/Both`) and the octave dry-routing
   semantic (`includeDryInOctavePath`) behind explicit adapters, preserving
   APVTS ids/polarities.
4. Extend the harness into the A/B/null gate:
   `golden_legacy_48k` vs core at 44.1/48/96/192 × block sizes × presets
   P0–P9, with the null depth budget and a CI check.
5. Regenerate the Web reference render once Emscripten and a working JUCE
   build are available, and close the Web↔VST null test.
