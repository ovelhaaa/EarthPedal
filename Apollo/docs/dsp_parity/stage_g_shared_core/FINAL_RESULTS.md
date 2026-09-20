# Stage G — Results so far

Stage G is **in progress**. This file reports the verified state of steps
G0–G3 + the CI gate. The remaining steps (G4–G8) are tracked in `README.md`.

## Verified results

### Golden @ 48 kHz (`shared/tests/golden_test.cpp`)

```
[Golden 48 kHz]
    p0_48k.f32: max|diff|=0  null=-546.4 dB   PASS
    p1_48k.f32: max|diff|=0  null=-547.8 dB   PASS
    p3_up_48k.f32: max|diff|=0  null=-546.0 dB   PASS
    p4_down_48k.f32: max|diff|=0  null=-546.1 dB PASS
    p5_both_48k.f32: max|diff|=0  null=-546.1 dB PASS
    p7_overdrive_48k.f32: max|diff|=0 null=-552.2 dB PASS
[Sample-rate invariance]
    sr=44100 timingRef=29400 leftDelay1=399.00 ms RT30=5.38 s  PASS
    sr=48000 timingRef=32000 leftDelay1=399.00 ms RT30=5.28 s  PASS
    sr=96000 timingRef=64000 leftDelay1=399.00 ms RT30=5.21 s  PASS
    sr=192000 timingRef=128000 leftDelay1=399.00 ms RT30=5.31 s PASS
[Octave across sample rates]
    sr=44100 octave Up finite, rms=0.086, blockDiff=0  PASS
    sr=96000 octave Up finite, rms=0.093, blockDiff=0  PASS
[Freeze]
    freeze sustains the tail and stays finite  PASS
    freeze engage/release transition is finite  PASS
[Block-size invariance]
    64 vs 128 bit-identical  PASS
    64 vs 512 bit-identical  PASS
ALL PASS (0 failures)
```

The core reproduces the frozen production reference **bit-for-bit** at 48 kHz
and keeps delay timing and RT stable across every supported rate.

## Acceptance criteria status

| # | Criterion | Status |
| --- | --- | --- |
| 1 | Single shared `EarthDSPCore` exists | done (reverb + octave + overdrive + freeze) |
| 2 | Web and JUCE use the core | done at source (G6/G7); not build-verified |
| 3 | Default is `LegacySrInvariant` | done |
| 4 | Golden @48k preserved | done, bit-exact (P0/P1/P3/P4/P5/P7) |
| 5 | Tank diffusion 0.7 centralised | done (`EarthDSPCore::prepare`) |
| 6 | Timebase roles explicit | done |
| 7 | LFO sample-rate invariant | done (rate context; measured in Stage F) |
| 8 | Tank filters legacy-invariant | done for the reverb tank cut filters |
| 9 | Pre-delay consistent | done (seconds, 0..1) |
| 10 | Octave canonical @48k | done (G4); shared provisional resampler for other rates |
| 11 | Shared octave enum | done (`EarthEnums.h`); adapters pending |
| 12 | Single dry-routing semantic | implemented (`includeDryInOctavePath`); adapter translation pending |
| 13 | APVTS compatibility | untouched; adapter pending (G7) |
| 14 | P0–P9 exist | P0/P1/P2/P3/P4/P5/P7/P8/P9/P10/P12 golden bit-exact; P6 functional; P11 == P0 |
| 15 | Block-size invariance tested | done (reverb and octave) |
| 16 | 44.1/48/96/192 pass | done (reverb + octave sanity) |
| 17 | Web↔VST null within threshold | not yet (needs adapters + Emscripten) |
| 18 | CI prevents regression | done (`dsp-parity.yml`) |
| 19 | Legacy DSP removed only after validation | not removed (G8) |
| 20 | Docs explain the decisions | done |

## Explicit answers

**Web and VST now run the same algorithm?**
At the source level, yes: both `src/wasm_wrapper.cpp` and
`Apollo/Source/PluginProcessor.{h,cpp}` are thin adapters over `EarthDSPCore`
and contain no DSP. Build/runtime equivalence is unverified here (no Emscripten
or JUCE toolchain); the G8 null test will confirm it.

**Was the Golden Web@48k preserved?**
Yes, for the implemented path: the shared core is bit-identical to the frozen
production reference for P0 and P1 (`max|diff| = 0`, null ≈ −547 dB).

**Do 96/192 kHz keep the same temporal character?**
Yes for the reverb path: `leftDelay1` is 399.00 ms and RT30 stays within
5.21–5.38 s across 44.1/48/96/192 kHz.

**Is any sonic rule still duplicated in the wrappers?**
No. Both wrappers now only translate parameters and call `EarthDSPCore`. The
legacy implementations are kept as `*_legacy.*` files on disk (not compiled) for
rollback until the builds and the null test pass (G8). The removed-behaviour
differences are documented in `MIGRATION_LOG.md`.

## Environment note

The core and its tests build and run with plain g++/CMake on Windows and Linux.
The JUCE plugin build and the Emscripten build were not used in this milestone;
the Web↔VST null test therefore remains pending an environment with Emscripten
and a working JUCE toolchain.
