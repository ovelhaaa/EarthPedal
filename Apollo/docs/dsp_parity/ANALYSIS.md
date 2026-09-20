# EarthPedal / Apollo — DSP Parity Analysis

Audit of the DSP divergence between the Web/WASM port and the Apollo JUCE/VST
port. Reference order used here:

1. `earth.cpp` — historical hardware reference (Daisy Petal, fixed 48 kHz).
2. `src/wasm_wrapper.cpp` + `web/` — the **provisionally preferred** sound.
3. `Apollo/Source/PluginProcessor.cpp` + `Apollo/Source/DSP/**` — current VST.

No port is assumed correct. Each item below is either **confirmed** (with
`file:line`), **measured** (see `PARITY_RESULTS.md` / `renders/`), or
**open** (needs a product/listening decision).

The single largest finding: the VST never calls `setTankDiffusion()`, so the
Dattorro tank's four internal all-pass diffusers start with gain `0`.
Measured effect: reflection density in the first 100 ms rises from **280/s to
920/s** and tail crest factor drops from **37.7 to 22.6** the moment diffusion
is restored. That is the "denser, smoother, more musical" trait attributed to
the Web build.

---

## 1. Tank diffusion absent in the VST — CONFIRMED / MEASURED (dominant)

* Original: `earth.cpp:651` → `reverb.setTankDiffusion(diffusion * 0.7);`
* Web: `src/wasm_wrapper.cpp:32` → `reverb_.setTankDiffusion(1.0 * 0.7);`
* VST: **no call anywhere**. `grep setTankDiffusion Apollo/Source` returns only
  the definition/declaration, never a call site.
* Consequence: `Dattorro1997Tank::setDiffusion()` is never invoked, so
  `leftApf1/2`, `rightApf1/2` keep the constructor gain of `0`
  (`AllpassFilter.hpp:9`, `AllpassFilter.hpp:11`). With gain `0` the tank stage
  degenerates from an all-pass diffuser into a bare delay line, producing a
  sparse, spiky, echoey tail.

Measured (irreducible input diffusion/decay): density 280 → 920 /s, crest 37.7 →
22.6, correlation between the two IRs only 0.26. See `PARITY_RESULTS.md`.

**Action (Stage A):** call `setTankDiffusion(0.7)` in the VST initialisation.

## 2. VST Dattorro initialisation incomplete — CONFIRMED / MEASURED

`earth.cpp:642-657` performs a full explicit initialisation. `PluginProcessor.cpp`
only calls `setSampleRate()` + `clear()` in `prepareToPlay`
(`PluginProcessor.cpp:72-74`); every other tank setting relies on class defaults.

| Setting | earth.cpp / Web | VST default (never set) | Divergence |
| --- | --- | --- | --- |
| tank diffusion | `0.7` (`earth.cpp:651`) | `0.0` | **major** (item 1) |
| input high cut | pitch 10 → 14080 Hz (`earth.cpp:648`) | `inputHighCut = 10000` (`Dattorro.hpp:208`) | moderate |
| input low cut | pitch 0 → 13.75 Hz (`earth.cpp:647`) | 13.75 Hz (set later by damp branch) | none |
| tank high cut | pitch 10 → 14080 Hz (`earth.cpp:653`) | filter default 22049 Hz (`OnePoleFilters.hpp:19`, `Dattorro.hpp:139`) | moderate |
| tank low cut | pitch 0 → 13.75 Hz (`earth.cpp:652`) | filter default 10 Hz (`OnePoleFilters.hpp:96`) | small |
| mod shape | `0.5` (`earth.cpp:656`) | `0.5` (ctor, `Dattorro.cpp:44-47`) | none (see Stage C) |
| mod speed | `1.0` (`earth.cpp:654`) | driven each block by params | none at default |
| mod depth | `0.5` (`earth.cpp:655`) | driven each block by params | none at default |

The Web wrapper replicates the earth.cpp sequence exactly in its constructor
(`wasm_wrapper.cpp:24-38`).

**Action (Stage B):** add an explicit initialisation helper for the shared core
(mirroring `earth.cpp`), including input high cut, tank low/high cut, mod shape.

## 3. Web pre-delay unit bug — CONFIRMED

`src/wasm_wrapper.cpp:187`:

```cpp
reverb_.setPreDelay(predelay_ * 1000.0f * 2.0f > 700.0f ? 700.0f : predelay_ * 1000.0f * 2.0f);
```

`Dattorro::setPreDelay(float t)` already multiplies by the sample rate
(`Dattorro.cpp:393` in Apollo, `Dattorro.cpp:386` at repo root). The wrapper
therefore passes up to `700` **seconds**, which `InterpDelay::setDelayTime`
clamps to the buffer length. Any non-zero Web pre-delay saturates the delay line
(≈0.77 s at the old fixed 37000-sample buffer). Even `predelay_ = 0.0001`
requests 200 ms.

The VST uses `setPreDelay(current_predelay)` with the raw normalized 0..1
(`PluginProcessor.cpp:437`), i.e. 0..1 s — different unit again.

**Action:** one canonical mapping `normalized 0..1 -> seconds 0..1.0`
(0 ms / 500 ms / 1000 ms), applied identically in both wrappers.

## 4. Web/VST defaults do not match — CONFIRMED

Current defaults:

| Parameter | Web (`web/earth-worklet-processor.js:126-138`) | VST (`PluginProcessor.cpp:35-42`) | Canonical (earth.cpp) |
| --- | --- | --- | --- |
| decay | 0.5 | 0.877 | `0.877465` (`earth.cpp:650`) |
| modDepth (norm) | 0.5 | 0.0625 | `0.0625` (0.5/8) |
| modSpeed (norm) | 0.5 | 0.0466 | `0.0466` ((1.0-0.3)/15) |
| size | 1 = Medium | 2 = Large | Large (`earth.cpp:644`) |
| mix | 0.5 | 0.5 | — |

Any subjective A/B done before this is fixed is invalid. See
`PARAMETER_MAPPING.md` for the exact conversions.

## 5. Octave dry routing diverges — CONFIRMED

* Web (and earth.cpp): always adds `0.5 * dry` into the octave branch before the
  reverb. `wasm_wrapper.cpp:135` uses `effect_mode_==2 || octave_only_mode_==false`
  where `octave_only_mode_` is always false, so the dry is added in **all**
  modes. `earth.cpp:499` uses `!dipValues[1] || effect_mode==2` (dip 2 off by
  default).
* VST: `PluginProcessor.cpp:366` adds dry only when
  `!octave_dry_mix || effect_mode == 2`. With the default `octave_dry_mix=true`
  and Up mode, **no inner dry is added**, so the reverb is excited by a very
  different signal.

`octave_dry_mix` is currently a double negation with a mode exception — see
`docs/ui-rack-redesign/DSP_NOTES.md` item 1.

**Action:** define one explicit, named semantic (`includeDryInOctavePath`),
keep the APVTS id/polarity, adapt at the wrapper. Add an Up/Down/Both × dry
on/off render matrix.

## 6. Octave enum not equivalent — CONFIRMED

| Value | Web (`wasm_wrapper.cpp:117-121`, hardware) | VST (`PluginProcessor.cpp:351-357`, `:46`) |
| --- | --- | --- |
| 0 | Off | None |
| 1 | Up | Up Octave |
| 2 | **Up + Down** | Down Octave |
| 3 | (n/a) | Both Octaves |

The same integer means different things. Internally a shared `enum class
OctaveMode { Off, Up, Down, Both }` must be used; wrappers translate their own
UI/APVTS values.

## 7. VST octave shelves designed at the wrong sample rate — CONFIRMED

`PluginProcessor.cpp:82-83` designs the shelves at
`48000 / resample_factor = 8000 Hz`, but `eq1/eQ2.processSample()` run on
`out_chunk[j]` (`PluginProcessor.cpp:362-363`), which is the **48 kHz**
reconstructed stream. The shelf corner frequencies therefore land ~6x higher
than intended.

The Web wrapper designs them at the stream rate
(`wasm_wrapper.cpp:21-22`), so at 48 kHz it is correct.

**Action:** design at 48 kHz (or, better, use one shared shelf implementation
in both targets). See the `shelf_response.png` plot.

## 8. Octave branch is not sample-rate canonical — CONFIRMED (Web) / handled by
resampling (VST)

`Util/Multirate.h` FIRs assume `48k -> /6 -> 8k -> OctaveGenerator -> *6 -> 48k`.
The VST resamples the branch to 48 kHz (`PluginProcessor.cpp:78`,
`:297-392`); the Web runs the FIRs directly at the AudioContext rate
(`wasm_wrapper.cpp:20-22`), so 44.1/48/96 kHz give different octave responses.

**Action:** adopt the canonical 48 kHz domain in the shared core for both.

## 9. Dry/wet alignment — CONFIRMED divergence, open decision

* VST delays the dry signal to align with the octave branch latency
  (`PluginProcessor.cpp:149-165`, `:483-486`) and reports
  `setLatencySamples(≈97)`. `PORTING_NOTES.md` documents this as an intentional
  fix for comb filtering.
* earth.cpp / Web do **not** delay the dry.

This is a legitimate technical improvement but changes the phase relationship.
It must be measured (comb depth vs. sample rate) before keeping: the current
fixed `89*(sr/48000)+8` estimate is not exact
(`docs/ui-rack-redesign/DSP_NOTES.md` item 2). The architectural preference is
to bypass the dry delay when the octave path is Off.

## 10. Sample-rate safety / tank time consistency — CONFIRMED

* Root `Dattorro/Dattorro.cpp:303` hard-codes `preDelay = InterpDelay(37000, 0.)`
  and **never resizes it** in `setSampleRate` (`Dattorro.cpp:395-411`). At
  96 kHz a 1 s pre-delay needs 96000 samples, so the buffer is too small. The
  Apollo copy already resizes dynamically (`Dattorro.cpp:406`); port that to the
  shared core.
* `Dattorro1997Tank` has `maxSampleRate = 32000.0` (`Dattorro.hpp:109`) and
  `setSampleRate()` clamps to it (`Dattorro.cpp:118-121`). The constructor
  parameter is even named `initMaxSampleRate` but is **never assigned** to the
  member. Both ports therefore run the tank at a 32000 Hz scale while being
  clocked at the host rate. Real-time reverb time is then `(32000/Fs)` of the
  intended Dattorro time — 2/3 at 48 kHz, 1/3 at 96 kHz, and rate-dependent.
  This is a *shared* latent bug, not a Web/VST divergence, so fixing it changes
  the preferred 48 kHz sound and must be A/B'd (see `SAMPLE_RATE_AUDIT.md`).

---

## Priority summary

| Item | Impact | Status |
| --- | --- | --- |
| 1 tank diffusion | **dominant** | fix, measured |
| 2 full init (input/tank cuts) | moderate | fix, measured |
| 7 shelf sample rate | moderate | fix, measured |
| 4 defaults | invalidates A/B | fix |
| 3 pre-delay unit | bug | fix |
| 5 octave dry routing | significant | fix + test matrix |
| 6 octave enum | correctness/API | fix with adapter |
| 8 canonical 48 kHz octave | consistency | shared core |
| 9 dry alignment | product | measure first |
| 10 SR safety / tank time | correctness | measure, flag |

The immediate win is items 1, 2 and 7: they make the VST match the Web at
48 kHz without changing any Web behaviour. `PARITY_RESULTS.md` quantifies them.
