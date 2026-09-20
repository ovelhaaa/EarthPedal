# Golden Tests

Executable: `shared/tests/golden_test.cpp` (target `golden_test`, registered via
CTest as `golden`). Run:

```bash
cmake -S shared -B shared/build -DCMAKE_BUILD_TYPE=Release
cmake --build shared/build -j
ctest --test-dir shared/build --output-on-failure
```

## The frozen reference

`shared/tests/golden/p0_48k.f32` and `p1_48k.f32` are mono float32 impulse
responses (3 s @ 48 kHz) produced by `make_golden`, which uses the **production
Apollo Dattorro plus earth.cpp's output stage** — not the shared core. This
keeps the golden independent of the thing under test.

| file | preset | mix |
| --- | --- | --- |
| `p0_48k.f32` | reverb only, Large, decay 0.877465, input diffusion on | 100 % wet |
| `p1_48k.f32` | canonical default | 50 % |
| `p2_mod_48k.f32` | modulation extreme (depth 1.0, speed 0.5) | 50 % |
| `p3_up_48k.f32` | octave Up | 50 % |
| `p4_down_48k.f32` | octave Down | 50 % |
| `p5_both_48k.f32` | octave Both | 50 % |
| `p7_overdrive_48k.f32` | Overdrive active (drive 0.6) | 50 % |
| `p8_predelay100_48k.f32` | pre-delay 100 ms | 50 % |
| `p9_predelay500_48k.f32` | pre-delay 500 ms | 50 % |
| `p10_dry_48k.f32` | mix 0 % (dry only) | 0 % |
| `p12_nodiffusion_48k.f32` | input diffusion off | 50 % |

Regenerate (only if the golden must intentionally change):

```bash
cmake --build shared/build --target make_golden
./shared/build/tests/make_golden shared/tests/golden
```

## What is asserted

### 1. Golden @ 48 kHz

| preset | max \|diff\| | null depth | required |
| --- | --- | --- | --- |
| P0 | 0 | −546 dB | bit-exact (max \|diff\| < 1e-6) |
| P1 | 0 | −548 dB | bit-exact |
| P2 (mod extreme) | 0 | −548 dB | bit-exact |
| P3 (Up) | 0 | −546 dB | bit-exact |
| P4 (Down) | 0 | −546 dB | bit-exact |
| P5 (Both) | 0 | −546 dB | bit-exact |
| P7 (Overdrive) | 0 | −552 dB | bit-exact |
| P8 (pre-delay 100 ms) | 0 | −548 dB | bit-exact |
| P9 (pre-delay 500 ms) | 0 | −547 dB | bit-exact |
| P10 (dry) | 0 | −548 dB | bit-exact |
| P12 (no input diffusion) | 0 | −549 dB | bit-exact |

P6 (Freeze) is a stateful transition; it is covered by the functional test in
GOLDEN_TESTS §4 rather than a static golden. P11 (mix 100 %) equals P0.

### 2. Sample-rate invariance (wet only)

| host | timing ref | leftDelay1 | RT30 |
| --- | --- | --- | --- |
| 44.1 kHz | 29400 | 399.00 ms | 5.38 s |
| 48 kHz | 32000 | 399.00 ms | 5.28 s |
| 96 kHz | 64000 | 399.00 ms | 5.21 s |
| 192 kHz | 128000 | 399.00 ms | 5.31 s |

Assertions: `timingReferenceRate == host * 2/3`; `leftDelay1` within 0.5 ms of
the golden; RT30 within 0.45 s of 5.28 s; all samples finite.

### 3. Octave across sample rates

With `octaveMode = Up`, rendering a 220 Hz burst at 44.1 kHz and 96 kHz must be
finite, produce output (`rms > 1e-5`) and be block-size invariant
(128 vs 512 bit-identical). The resampled path is primed at `reset()` so the
first block never underruns.

### 4. Freeze

With `PerformanceMode::Freeze` and `performanceActive = true`, the tail energy
in 1.5–2.0 s must be more than double the un-frozen tail (decay ramps to 1.0).
Engaging and releasing freeze mid-render must stay finite.

### 5. Block-size invariance

Rendering the same input with block sizes 64, 128 and 512 must be
**bit-identical** (max diff == 0). Because smoothing is per-sample, block size
must not affect the output.

### 6. Finite output

NaN / Inf in any sample fails the test.

## Why P0/P1 only (for now)

The golden covers the reverb/output path. The octave branch (P3/P4/P5) and the
overdrive (P7) are not yet in the core (G4/G5); their production default is
inactive, so P0/P1 exercise the full currently-shipped path. Additional golden
references for P3–P7 will be added with G4/G5.

## CI

`.github/workflows/dsp-parity.yml` configures, builds and runs CTest on
`ubuntu-latest` for every change under `shared/`. The gate is the golden, the
SR-invariance and the block-invariance assertions above.
