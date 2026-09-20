# Timebase Analysis — every sample-rate consumer in the tank

## 1. Root cause

`Dattorro1997Tank` declares:

```cpp
float maxSampleRate = 32000.0;   // Dattorro.hpp:109
```

but the constructor parameter `initMaxSampleRate` is **never assigned** to it.
The constructor only forwards it to `setSampleRate()`:

```cpp
Dattorro1997Tank::Dattorro1997Tank(const float initSampleRate, ...) {
    timePadding = initMaxLfoDepth;
    setSampleRate(initSampleRate);          // clamps to maxSampleRate == 32000
    ...
}
```

and `setSampleRate()` clamps:

```cpp
sampleRate = newSampleRate;
sampleRate = sampleRate > maxSampleRate ? maxSampleRate : sampleRate;  // Dattorro.cpp:118-121
sampleRateScale = sampleRate / dattorroSampleRate;                     // 29761
```

Therefore the tank runs at **32000 Hz scaling at every host rate**, while
`process()` is called at the host rate. Both the Apollo VST and the Web/WASM
builds use the same source, so this is a *shared* bug, not a Web/VST divergence.

## 2. Everything that consumes a sample rate

The class currently uses one `sampleRate` for at least five different meanings.
This is the conflation Stage F must untangle.

| # | Consumer | Code | Meaning | Correct source |
| --- | --- | --- | --- | --- |
| 1 | `sampleRateScale` → delay lengths | `Dattorro.cpp:117`, `:272-289` | seconds ↔ samples for delays | timing rate |
| 2 | `scaledOutputTaps` | `Dattorro.cpp:293-297`, `:129` | tap positions in samples | timing rate |
| 3 | `lfoExcursion = depth*16*sampleRateScale` | `Dattorro.cpp:158-161` | modulation depth in samples | timing rate |
| 4 | `fadeStep = 1/sampleRate` | `Dattorro.cpp:124` | freeze crossfade duration | process rate |
| 5a | `leftOutDCBlock/rightOutDCBlock.setSampleRate()` | `Dattorro.cpp:126-127` | DC blocker cutoff (Hz) | filter rate |
| 5b | tank high/low cut filters | **never updated** | cutoff (Hz) | filter rate (latent bug) |
| 6 | `TriSawLFO` phase step | `LFO.hpp:92-95` | LFO frequency (Hz) | modulation rate |
| 7 | Outer `Dattorro` input filters, input APFs, pre-delay | `Dattorro.cpp:402-421` | cutoffs (Hz), input diffusion | host / process |

### 2.1 LFO never sees the host rate

`TriSawLFO` defaults to `sampleRate = 32000` (`LFO.hpp:13`) and computes
`_stepSize = _frequency / _sampleRate` (`LFO.hpp:92-95`). The tank **never calls
`setSamplerate()`**, so the LFO advances `frequency/32000` per `process()` call
while `process()` runs at the host rate. The real LFO frequency is therefore:

```
realHz = targetHz * hostRate / 32000
```

At 48 kHz that is 1.5x the target; at 96 kHz, 3x; at 192 kHz, 6x. Measured in
`LFO_AUDIT.md`.

### 2.2 Tank cut filters are frozen at 32000

`Dattorro1997Tank::setSampleRate()` updates the DC blockers but **not**
`leftHighCutFilter` / `leftLowCutFilter` / their right counterparts. They were
built with `OnePoleLPFilter`'s default `initSampleRate = 32000`
(`OnePoleFilters.hpp:19`) and `setTankFilterHighCutFrequency()` only calls
`setCutoffFreq()`. So a cutoff configured as 14080 Hz is realised as
`14080 * host / 32000` — 21120 Hz at 48 kHz. `correct` and `invariant` sync
these to the timing rate; `legacy32k` leaves them at 32000.

### 2.3 Freeze fade

`fadeStep = 1/sampleRate` with `sampleRate` clamped to 32000, clocked at the
host rate, gives a constant 32000/host seconds of crossfade in `legacy32k`
(0.667 s at 48 kHz). Under `correct` it becomes 1.0 s. Under `invariant`
(timing rate = 2/3 host) it stays 0.667 s.

## 3. Why `timeScale *= 2/3` is not a fix

Scaling `timeScale` affects only the eight main delay/APF lengths (via
`sampleRateScale * timeScale`). It does **not** scale:

* `scaledOutputTaps` (`rescaleTapTimes` uses `sampleRateScale` only, `:293-297`);
* `lfoExcursion` (`setModDepth` uses `sampleRateScale` only, `:158-161`);
* `TriSawLFO` frequency;
* the tank cut/DC filters;
* the freeze fade.

If only `timeScale` were corrected, taps would sit 1.5x too far into delay
lines that are 1.5x too short, the modulation excursion would be 1.5x too large
relative to the loop, and the LFO would still be host-dependent. Stage F
therefore separates the rates explicitly.

## 4. The three models

Let `processSampleRate = host`. The Dattorro delay constants are expressed at
the canonical rate `29761 Hz`.

### MODEL 1 — `legacy32k` (current production)

```
timingReferenceRate = filterSampleRate = modulationSampleRate = min(host, 32000)
processSampleRate   = host
```

Real delay time scales as `min(host,32000)/host`; it is `2/3` at 48 kHz, `1/3`
at 96 kHz. Not SR-invariant.

### MODEL 2 — `correct`

```
timingReferenceRate = filterSampleRate = modulationSampleRate = processSampleRate = host
```

This is the textbook interpretation: delays keep their seconds, LFOs their Hz,
filters their cutoffs. SR-invariant, but at 48 kHz it is **1.5x longer** than
the preferred Web sound.

### MODEL 3 — `invariant` (the Stage F hypothesis)

```
timingReferenceRate = filterSampleRate = modulationSampleRate = host * (32000/48000)
                                                                          = host * 2/3
processSampleRate   = host
```

The ratio `timingReferenceRate / processSampleRate` is a constant `2/3`, so
every real-time quantity is independent of the host and equal to what
`legacy32k` does at 48 kHz. Measured bit-exact against `golden_legacy_48k`
(null −553 dB).

## 5. What was NOT changed in production

* The `maxSampleRate = 32000` member and the clamp remain.
* No wrapper, APVTS id, or parameter polarity was touched.
* The three models exist only inside `timebase_harness.cpp`, which sets the
  public tank fields before `setSampleRate()`.
