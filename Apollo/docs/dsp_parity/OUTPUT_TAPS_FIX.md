# Output-taps timeScale fix

## Symptom

With **Size = Large** (and to a lesser degree Medium) the reverb tail was
audible as a **distinct repetition / echo**, instead of a smooth decay.

## Root cause

`Dattorro1997Tank::rescaleTapTimes()` scaled the output-tap positions only by
`sampleRateScale`, never by `timeScale`:

```cpp
scaledOutputTaps[i] = (int)((float)kOutputTaps[i] * sampleRateScale);
```

The delay-line lengths *do* scale with `timeScale`
(`rescaleApfAndDelayTimes`), so at `timeScale = 4` (Large) the taps sat at
roughly 1/4 of their intended fraction of the (now 4x longer) delay lines. The
output therefore sampled a coherent early region of the tank loop, producing a
discrete echo. At `timeScale = 1` (Small) the bug is invisible, which is why it
went unnoticed.

This bug is present in **all three copies** (root Web, Apollo, shared) — it is
not a Stage G regression.

## Evidence (wet impulse response, 48 kHz, Size Large)

RMS envelope per 100 ms, dB:

| | 0–0.4 s | 0.4–0.5 s | after |
| --- | --- | --- | --- |
| before | −46, −50, −54, −56 | **−49 (a +7 dB burst)** | −52, −51, … |
| after | −49, −51, −50, −52 | −51 | −52, −51, … (smooth) |

* Size Small: bit-identical before/after (density 173/s).
* Size Large: burst removed; density 158 → 165/s; RT30 ~5.1 s.
* Taps stay inside their buffers (max scaled tap 11896 < 21587), no OOB.

## Fix

1. `rescaleTapTimes()` now multiplies by `timeScale` too.
2. `setTimeScale()` now calls `rescaleTapTimes()` after
   `rescaleApfAndDelayTimes()` (previously taps were never recomputed when the
   size changed).
3. `calcMaxTime()` sizes the buffers for `maxTimeScale` on the taps, so they
   always fit for any supported size.

Applied identically to `shared/Dattorro`, `Apollo/Source/DSP/Dattorro` and the
root `Dattorro/` copy. The golden `.f32` references were regenerated.

## Impact

This intentionally changes the **Medium/Large** reference sound (the previously
"preferred" Web also had the echo). Small is unchanged. The change was approved
explicitly before regenerating the Golden, per the "do not change the Golden
without evidence" rule.

## Reproduction

```
g++ ... shared/tests/... -> golden_test
make_golden shared/tests/golden
ctest ...
```

`golden_test` reports bit-exact (max|diff| = 0) for all presets and RT30
5.07–5.20 s across 44.1/48/96/192 kHz.
