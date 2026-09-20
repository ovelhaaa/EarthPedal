# Parity Results — Stage 1 (reverb)

Measured with the JUCE-free harness in `Apollo/tools/dsp_parity/`. Renders and
CSVs are in `Apollo/docs/dsp_parity/renders/`, plots in `plots/`.

Reproduce:

```powershell
# build (MinGW g++, no JUCE required)
g++ -std=c++20 -O2 -IApollo/Source/DSP -IApollo/Source/DSP/Dattorro -IApollo/Source/DSP/Util `
    Apollo/tools/dsp_parity/parity_harness.cpp `
    Apollo/Source/DSP/Dattorro/Dattorro.cpp `
    Apollo/Source/DSP/Dattorro/dsp/delays/InterpDelay.cpp `
    Apollo/Source/DSP/Dattorro/dsp/filters/OnePoleFilters.cpp `
    -o Apollo/tools/dsp_parity/parity_harness.exe
Apollo/tools/dsp_parity/parity_harness.exe Apollo/docs/dsp_parity/renders
python Apollo/tools/dsp_parity/analyze.py Apollo/docs/dsp_parity/renders Apollo/docs/dsp_parity/plots
```

All renders: mono impulse, 48 kHz, `timeScale = Large (4)`, canonical
`decay = 0.877465`, `modDepth norm = 0.0625`, `modSpeed norm = 0.0466`.

## Stage definitions

| Label | tank diffusion | input high cut | tank low/high cut | mod shape | shelves |
| --- | --- | --- | --- | --- | --- |
| `vst_current` | not called (0.0) | not called (10000 Hz) | not called | not called | 8 kHz design |
| `vst_A_tank_diffusion` | **0.7** | not called | not called | not called | 8 kHz |
| `vst_B_tank_init` | 0.7 | pitch 10 | pitch 0 / pitch 10 | called | 8 kHz |
| `vst_C_modshape` | 0.7 | pitch 10 | pitch 0 / pitch 10 | 0.5 | 8 kHz |
| `web_reference_default` | 0.7 | pitch 10 | pitch 0 / pitch 10 | 0.5 | 48 kHz |

## Reverb metrics (`renders/metrics.csv`)

| render | RT60 (s) | tail crest | density 0–100 ms (/s) | density 100–300 ms (/s) | energy (dB) |
| --- | --- | --- | --- | --- | --- |
| `vst_current` | 5.16 | **37.73** | **280** | 375 | 5.34 |
| `vst_A_tank_diffusion` | 5.19 | 22.55 | 920 | 535 | 5.99 |
| `vst_B_tank_init` | 5.28 | 24.07 | 920 | 470 | 6.25 |
| `vst_C_modshape` | 5.28 | 24.07 | 920 | 470 | 6.25 |
| `web_reference_default` | 5.28 | 24.07 | 920 | 470 | 6.25 |
| `diffusion_0.0` | 5.71 | 41.94 | 280 | 335 | 5.62 |
| `diffusion_0.7` | 5.28 | 24.07 | 920 | 470 | 6.25 |

## Null tests (`renders/NULL_REPORT.txt`)

`null_depth_db = 20*log10(rms(diff) / rms(reference))`; a large negative value
means an exact match.

| pair | null depth (dB) | correlation | lag |
| --- | --- | --- | --- |
| `vst_current` vs `vst_A_tank_diffusion` | +2.0 | 0.261 | 0 |
| `vst_A` vs `vst_B_tank_init` | −22.2 | 0.998 | 0 |
| `vst_B` vs `web_reference_default` | −553 (bit-exact) | 1.000 | 0 |
| `diffusion_0.0` vs `0.7` | +2.0 | 0.268 | 0 |

## Conclusions

1. **Tank diffusion is the dominant cause** of the perceived difference. One
   call — `setTankDiffusion(0.7)` — multiplies early reflection density by
   ~3.3x (280 → 920/s) and reduces the tail crest factor from 37.7 to 22.6.
   The two IRs correlate only 0.26, i.e. they are essentially different sounds.
2. **The remaining tank initialisation** (input high cut 14080 Hz, tank
   low/high cut) is a secondary, audible but small change: A vs B nulls at
   −22 dB with 0.998 correlation. It still matters because it is part of the
   original voicing.
3. **Mod shape (Stage C) is a no-op**: the tank constructor already sets
   `revPoint = 0.5`; C is bit-identical to B. Keep the explicit call for
   robustness but expect no sonic change.
4. `web_reference_default` is bit-identical to Stage B (null −553 dB), proving
   the harness reproduces the Web/earth.cpp reverb initialisation exactly.
5. Harness limitation: `vst_current` bypasses the (unimplemented) octave
   shelves and the per-block parameter path, so it is the reverb-only baseline.

## Shelf sample-rate bug (item 7)

`renders/shelf_response.csv` + `plots/shelf_response.png` compare the shelf pair
`EQ1 = -11 dB @ 140 Hz`, `EQ2 = +5 dB @ 160 Hz`:

* VST today: coefficients computed at 8 kHz, applied to a 48 kHz signal — the
  effective corner is shifted up by ~6x, so the intended low-mid tilt is not
  applied.
* Web: coefficients at 48 kHz — correct at 48 kHz.

`octave_shelf_vst8k.wav` / `octave_shelf_web48k.wav` render the same log sweep
through both. This is fixed by designing the shelves at 48 kHz (and sharing the
shelf implementation between targets).

## Octave dry-routing matrix (item 5)

`renders/octave_routing.csv`, 48 kHz octave branch, synthetic A2/E3/A4 chord,
shelves designed at 48 kHz.

| mode | include inner dry | RMS | corr. with dry input |
| --- | --- | --- | --- |
| Up | no | 0.0368 | 0.099 |
| Up | **yes** | 0.0647 | **0.853** |
| Down | no | 0.2357 | 0.044 |
| Down | yes | 0.2440 | 0.254 |
| Both | no | 0.2384 | 0.059 |
| Both | yes | 0.2468 | 0.266 |

Interpretation:

* For **Up**, the inner dry is decisive: it nearly doubles the RMS and raises
  the correlation with the dry signal from 0.10 to 0.85. The VST default
  (`octave_dry_mix = true`, Up mode) adds **no** inner dry, so it excites the
  reverb with a very different signal than the Web/hardware — a real timbre and
  level difference, independent of the reverb bugs above.
* For **Down/Both** the octave content dominates either way; the dry has a
  smaller relative effect.

Therefore `includeDryInOctavePath` must be an explicit, documented semantic
(and the legacy APVTS id kept, with the adapter translating), not a double
negation with a mode exception.

## Status vs acceptance criteria

| # | Criterion | Status |
| --- | --- | --- |
| 1 | Same DSP core | not yet (only reverb proven) |
| 2 | Tank diffusion explicit | **fix identified, measured** |
| 3 | Defaults equivalent | mapped, not yet applied |
| 4 | Pre-delay unit | bug identified |
| 5 | Octave enum | mapped, not yet applied |
| 6 | Dry routing semantic | mapped, waiting on matrix |
| 7 | Shelves in correct domain | **fix identified, measured** |
| 8 | Octave SR-consistent | architecture specified |
| 9 | No unsafe fixed buffers | identified |
| 10 | Dattorro time SR-consistent | identified, needs listening |
| 11 | A/B renders of intermediate states | **done (this folder)** |
| 12 | Objective Web vs VST comparison | Web render still needs `emcc` (absent) |
| 13 | Null test after unification | pending unification |
| 14 | No freeze/overdrive/bypass regression | pending |
| 15 | Web character preserved | items 1/2/7 preserve it by construction |

### Environment note

`emcc` is not installed (`emsdk/` is empty) and the JUCE test target cannot be
built here (the `juceaide` host tool fails under the only available compilers).
The Web reference render and the full VST null test therefore remain to be run
on a machine with Emscripten and MSVC/working JUCE build. The harness above is
JUCE-free precisely so the DSP can be measured without them.
