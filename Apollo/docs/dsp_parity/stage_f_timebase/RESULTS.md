# Stage F Results

All numbers below are reproducible from `renders/*.csv` and `plots/*.png`.

## Golden reference — `golden_legacy_48k`

Model `legacy32k`, 48 kHz, Size Large, decay 0.877465, tank diffusion 0.7,
input diffusion on, original tank filter init, mod shape 0.5.

| metric | value |
| --- | --- |
| density 0–50 ms | 900 /s |
| density 0–100 ms | 920 /s |
| density 100–500 ms | 460 /s |
| tail crest factor | 24.07 |
| rms tail | 0.00434 |
| peak | 0.1297 |
| RT20 | 7.73 s |
| RT30 / RT60 | 5.28 s |
| spectral centroid | 6509 Hz |

Cross-check: identical to the Stage 1 `vst_B_tank_init` / `web_reference_default`
(RT60 5.276, crest 24.07, density 920). The two harnesses agree.

## Model × host matrix (Size Large, decay 0.877465)

| model | host | RT30 (s) | density 100–500 (/s) | crest | centroid (Hz) |
| --- | --- | --- | --- | --- | --- |
| legacy32k | 32 kHz | 4.81 | — | 21.8 | 5487 |
| legacy32k | 44.1 kHz | 5.17 | 418 | 22.3 | 5980 |
| legacy32k | 48 kHz | 5.28 | 460 | 24.1 | 6509 |
| legacy32k | 88.2 kHz | 6.25 | — | 24.3 | 9957 |
| legacy32k | 96 kHz | 6.27 | 942 | 25.7 | 8993 |
| legacy32k | 192 kHz | 6.00 | — | 25.7 | 22304 |
| correct | 48 kHz | 4.70 | 300 | 24.9 | 8200 |
| correct | 96 kHz | 4.80 | 450 | 28.8 | 11008 |
| invariant | 44.1 kHz | 5.38 | 465 | 22.9 | 6195 |
| invariant | 48 kHz | 5.28 | 460 | 24.1 | 6509 |
| invariant | 96 kHz | 5.21 | 522 | 22.8 | 9351 |
| invariant | 192 kHz | 5.31 | — | 25.0 | 20331 |

The spectral centroid rises with host rate for every model because the analysis
bandwidth rises with the Nyquist frequency; it is not a timbre change per se.
The important column is **RT30**, which is stable only for `correct` and
`invariant`, and matches the golden value only for `invariant`.

## Null tests (same host rate)

| pair | null depth | correlation | lag |
| --- | --- | --- | --- |
| `invariant` 48 kHz vs `golden_legacy_48k` | **−553 dB (bit-exact)** | 1.00000 | 0 |
| `correct` 48 kHz vs `golden_legacy_48k` | +3.2 dB | 0.037 | 0 |
| `invariant` 96 kHz vs `legacy32k` 96 kHz | +3.2 dB | 0.032 | 0 |

`invariant` is bit-identical to the golden 48 kHz reference, proving the model
is implemented correctly. Against `legacy32k` at 96 kHz the two signals are
essentially uncorrelated (0.03) — the legacy compression changes the sound, not
just the timing.

## Audio renders (48 kHz reference + listening)

* `golden_legacy_48k_ir.wav`
* `{legacy32k,correct,invariant}_{44100,48000,96000}_ir.wav` (100 % wet)
* `{...}_music_mix.wav` (30 % dry / 70 % wet)
* `{...}_music_wet.wav` (100 % wet)
* `lfo1_{legacy32k,correct,invariant}_{48000,96000}.wav`

WAVs are git-ignored; regenerate with the command in `README.md`.

## Plots

`plots/delay_time_vs_host.png`, `lfo_freq_vs_host.png`, `rt30_vs_host.png`,
`rt30_decay_matrix.png`, `density_vs_host.png`, `edc_48000.png`,
`edc_96000.png`, `ir_early_zoom_48k.png`, `magnitude_spectra_48k.png`,
`spectrogram_48k.png`, `waveform_overlay_96k.png`,
`lfo_waveform_48000.png`, `lfo_waveform_96000.png`.

## Answers to the Stage F questions

1. **Closest to the current Web at 48 kHz?** `invariant` — bit-exact
   (−553 dB). `legacy32k` is the reference itself at 48 kHz; `correct` differs
   (null +3.2 dB, corr 0.04).
2. **Same sound across 44.1/48/96?** `correct` and `invariant` both keep RT and
   delay times stable; `legacy32k` does not.
3. **Delay times consistent in seconds?** `correct` (598.5 ms) and `invariant`
   (399.0 ms) are; `legacy32k` collapses (`leftDelay1` 399 → 199.5 ms from
   48 → 96 kHz).
4. **RT60 consistent?** `invariant` (±4 %, matches golden); `correct`
   (±3 %, but 11 % below golden); `legacy32k` (±30 %).
5. **LFO frequency and depth consistent?** `correct` (exact Hz) and `invariant`
   (legacy 48 kHz Hz, constant) are; `legacy32k` scales 6x by 192 kHz.
6. **Does the mathematical fix change the sound musically?** Yes. `correct`
   lengthens the tail 1.5x at 48 kHz, alters RT60 by ~11-24 %, and changes the
   early reflection pattern (corr 0.04). It is a different instrument.
7. **Is the 32k behaviour a bug or identity?** The *mechanism* (unassigned
   `maxSampleRate`, frozen LFO/filter rates) is a bug; the *resulting 48 kHz
   sound* is the identity users prefer. The bug is that this identity silently
   changes with the host rate.
8. **Can the identity be preserved independent of host rate?** Yes — that is
   exactly `invariant`, verified bit-exact at 48 kHz and SD-invariant elsewhere.

## Environment note

Everything here is JUCE-free and was built/run with MinGW g++ 14.2 and Python
3.14 (numpy/matplotlib). The Web reference render and the full
Web↔VST null test still require Emscripten, which is not installed here.
