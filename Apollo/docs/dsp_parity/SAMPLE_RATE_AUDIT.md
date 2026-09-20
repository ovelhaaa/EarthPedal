# Sample-Rate Audit

Rates considered: 44.1, 48, 88.2, 96, 192 kHz.

## 1. Pre-delay buffer

* Root (Web/WASM) `Dattorro/Dattorro.cpp:303`:
  `preDelay = InterpDelay(37000, 0.);` and `setSampleRate()` **does not**
  resize it (`Dattorro.cpp:395-411`).
* Apollo `Apollo/Source/DSP/Dattorro/Dattorro.cpp:406`:
  `preDelay = InterpDelay(ceil(sampleRate * 1s) + 1, 0.);` resized on every
  `setSampleRate()`.

`InterpDelay` uses a `std::vector` (`InterpDelay.hpp:23`), so the legacy
`sdramData[13][37000]` (`InterpDelay.cpp:3`) is dead storage — but the root
pre-delay is still capped at 37000 samples:

| Rate | Samples for 1 s | Root capacity (37000) | OK? |
| --- | --- | --- | --- |
| 44.1 kHz | 44100 | 37000 | no (max 0.84 s) |
| 48 kHz | 48000 | 37000 | no (max 0.77 s) |
| 96 kHz | 96000 | 37000 | no (max 0.39 s) |
| 192 kHz | 192000 | 37000 | no (max 0.19 s) |

**Fix:** port the Apollo dynamic allocation into the shared core.

## 2. Tank time scale — the important one

`Dattorro1997Tank` declares `float maxSampleRate = 32000.0;`
(`Dattorro.hpp:109`, identical in both copies) and `setSampleRate()` clamps
(`Dattorro.cpp:118-121`):

```cpp
sampleRate = newSampleRate;
sampleRate = sampleRate > maxSampleRate ? maxSampleRate : sampleRate;
sampleRateScale = sampleRate / dattorroSampleRate;   // 29761
```

The constructor parameter is named `initMaxSampleRate` but is **never assigned**
to `maxSampleRate`. Both ports construct with 48000 and then call
`setSampleRate(hostRate)`, so the tank always runs at a **32000 Hz scale** while
being clocked at the host rate.

The Dattorro delay lengths (`leftApf1Time = 672`, … , `rightDelay2Time = 3163`
samples) are defined at 29761 Hz. Real-time preservation requires
`scale = Fs / 29761`. With the clamp the effective real-time factor is
`32000 / Fs`:

| Host rate | Intended scale | Actual scale | Real-time factor |
| --- | --- | --- | --- |
| 44.1 kHz | 1.482 | 1.075 | 0.725 |
| 48 kHz | 1.613 | 1.075 | 0.667 |
| 88.2 kHz | 2.964 | 1.075 | 0.363 |
| 96 kHz | 3.226 | 1.075 | 0.333 |
| 192 kHz | 6.452 | 1.075 | 0.167 |

So the reverb tail is **rate dependent** and progressively collapses above
44.1 kHz. This is a shared bug, not a Web/VST divergence: fixing it to
`scale = Fs/29761` makes the 48 kHz tail **1.5x longer** than the currently
preferred Web sound. Because the Web sound is the provisional reference, this
change must be A/B'd and approved before it is committed (it can also be
implemented as an opt-in "SR-invariant" flag).

Suggested correct implementation:

```cpp
Dattorro1997Tank::Dattorro1997Tank(float initMaxSampleRate, ...) {
    maxSampleRate = initMaxSampleRate;   // <-- missing assignment
    ...
}
```
and construct with a max >= the highest supported host rate (e.g. 192000) so
`setSampleRate(host)` never clamps.

## 3. Octave branch domain

* The `Multirate.h` FIR coefficients are fixed for
  `48k -> /6 -> 8k -> Octave -> *6 -> 48k`.
* Web runs them at the AudioContext rate (`wasm_wrapper.cpp:20-22`), so the
  octave response drifts with the browser rate (item 8).
* VST resamples the branch to 48 kHz (`PluginProcessor.cpp:78,297-392`) — the
  correct architecture — but the anti-alias filter and Lagrange interpolators
  have rate-dependent group delay, which the fixed dry-delay estimate ignores
  (`DSP_NOTES.md` item 2).

**Fix:** canonical 48 kHz octave domain in the shared core, identical in both
targets, with the dry alignment decision made after measurement (item 9).

## 4. Recommended canonical pipeline

```
HOST RATE
  -> anti-alias / resampler
  -> 48 kHz
  -> /6
  -> 8 kHz OctaveGenerator
  -> *6
  -> 48 kHz
  -> shelves (designed @ 48 kHz)
  -> resampler
  -> HOST RATE
```

The Dattorro reverb should run entirely at the host rate with
`scale = Fs/29761` (after the item 2 decision).

## 5. Support matrix after fixes

| Rate | Pre-delay | Tank time | Octave | Shelves |
| --- | --- | --- | --- | --- |
| 44.1 | dynamic | (flag) | resampled | 48 kHz domain |
| 48 | dynamic | (flag) | native | 48 kHz domain |
| 88.2 | dynamic | (flag) | resampled | 48 kHz domain |
| 96 | dynamic | (flag) | resampled | 48 kHz domain |
| 192 | dynamic | (flag) | resampled | 48 kHz domain |
