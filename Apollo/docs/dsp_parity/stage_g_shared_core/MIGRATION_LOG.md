# Migration Log

Small, reviewable steps. Each step leaves the project understandable and the
production code runnable.

## G0 — Stage F checkpoint

* Commit `6df0b21`, pushed to `origin/main`.
* Contains only Stage F artifacts (harness, audits, plots, metrics). No
  architectural change. Kept isolated on purpose.

## G1 — shared types

* Added `shared/EarthEnums.h`, `shared/EarthParameters.h`,
  `shared/EarthRateContext.h`, `shared/EarthTimebase.h`.
* No production audio change.

## G2 — shared Dattorro

* `shared/Dattorro/` is the Apollo copy with two added methods
  (`Dattorro1997Tank::setRateContext`, `Dattorro::setRateContext`). A diff
  against `Apollo/Source/DSP/Dattorro` shows only these additions (plus
  warnings in `OnePoleFilters.hpp`); the algorithm is byte-identical.
* `LegacySrInvariant` is implemented by feeding the rate context:
  `sampleRateScale`, taps and LFO excursion use `timingReferenceRate`; tank
  cut/DC coefficients use `filterSampleRate`; LFO phase uses
  `modulationSampleRate`; the outer input filters, pre-delay and input APFs use
  `processSampleRate`.
* **Regression caught and reverted:** forcing `OnePoleFilter::setSampleRate()`
  to recompute coefficients changed the outer DC blockers (which historically
  keep 32 kHz coefficients because `setCutoffFreq` early-returns). The golden
  failed with max|diff| 2.9e-4. The change was reverted and documented in
  `OnePoleFilters.hpp` and `RATE_CONTEXT.md`.

## G3 — EarthDSPCore (reverb / output path)

* `shared/EarthDSPCore.{h,cpp}`: prepare/reset/setParameters/snapParameters/
  process/getLatencySamples.
* Implements the historical initialisation (tank diffusion 0.7, input/tank cut
  filters, mod shape 0.5), the energy-preserving mix law, the `*0.4` wet
  headroom, pre-delay, damp mapping, freeze (`decay -> 1.0`) and bypass.
* Per-sample smoothing with windows stored in the core (5 ms).
* Octave and overdrive are not wired yet (G4/G5); their defaults are inactive.

## G3 tests + G9 (partial) — golden gate

* `shared/tests/make_golden.cpp` builds the frozen 48 kHz reference from the
  production Apollo Dattorro (independent of the core).
* `shared/tests/golden_test.cpp` asserts golden bit-exactness, sample-rate
  invariance, block-size invariance and finiteness.
* `shared/CMakeLists.txt` + `shared/tests/CMakeLists.txt`.
* `.github/workflows/dsp-parity.yml` runs the gate on Linux.

## G4 — shared octave engine

* Added `shared/Multirate/Multirate.h`, `shared/Octave/{OctaveGenerator,
  BandShifter,FastSqrt}.h` (copies of the Apollo DSP), `shared/Filters/
  ShelfFilter.h` (new shared RBJ high/low shelf) and `shared/Resampling/
  FractionalLinearResampler.h` (provisional host<->48k resampler).
* `EarthDSPCore` now runs the octave branch in the canonical 48 kHz domain.
  At exactly 48 kHz the resampler is bypassed and the path is bit-identical to
  the native Web/earth reference.
* The resamplers are primed at `reset()` so the octave path is block-size
  invariant at non-48k rates.
* Octave golden P3/P4/P5: bit-exact at 48 kHz.

## G5 — shared overdrive + freeze

* Added `shared/Effects/Overdrive.{h,cpp}` (DaisySP Overdrive port, same math).
* `EarthDSPCore` applies the overdrive to the wet signal with the historical
  compensation curve and the base/active drive 0.4/0.6, and implements the
  freeze `decay -> 1.0` with smoothing.
* Overdrive golden P7: bit-exact at 48 kHz. Freeze functional test passes.

## Not yet done

G6 Web adapter, G7 JUCE adapter, G8 removal of the duplicated legacy DSP, and
the Web↔VST null test. No production file under `Apollo/Source/` or `src/` was
modified in G1–G5.

## Regeneration

The golden `.f32` files in `shared/tests/golden/` were regenerated in G4/G5 to
add P3/P4/P5/P7. `make_golden` uses the production Apollo Dattorro / Multirate /
OctaveGenerator / Overdrive plus the shared shelf definition.
