# DSP Difference Matrix — earth.cpp (hardware) vs Web/WASM vs Apollo VST

Legend: `=` equivalent, `≠` divergent, `?` needs listening. Line references are
to the current tree.

## Initialisation

| Concern | earth.cpp (hardware) | Web (`wasm_wrapper.cpp`) | VST (`PluginProcessor.cpp`) | Verdict |
| --- | --- | --- | --- | --- |
| Construction | `reverb(48000,16,4.0)` `earth.cpp:51` | `reverb_(sampleRate,16,4.0)` `:19` | `reverb(48000,16,4.0)` `:14` | Web uses host rate as ctor max; VST fixed 48000 |
| setSampleRate | `:642` | `:24` | `prepareToPlay :72` | = |
| setTimeScale | 4.0 `:644` | 4.0 `:25` | per-block from `time_scale` `:211` | = at Large |
| setPreDelay | 0.0 `:645` | 0.0 `:26` | per-sample, 0..1 s `:437` | ≠ unit (Web bug item 3) |
| input low cut | pitch 0 `:647` | pitch 0 `:28` | set by damp branch `:241` | ≈ |
| input high cut | pitch 10 `:648` | pitch 10 `:29` | **never called** → 10000 Hz | ≠ |
| enableInputDiffusion | true `:649` | true `:30` | per-block `:255` | = |
| decay init | 0.877465 `:650` | 0.877465 `:31` | param default 0.877 `:37` | ≈ |
| **tank diffusion** | **0.7 `:651`** | **0.7 `:32`** | **never called** → 0.0 | **≠ major** |
| tank low cut | pitch 0 `:652` | pitch 0 `:33` | never called → 10 Hz | ≠ small |
| tank high cut | pitch 10 `:653` | pitch 10 `:34` | never called → 22049 Hz | ≠ |
| tank mod speed | 1.0 `:654` | 1.0 `:35` | params → 1.0 default | = |
| tank mod depth | 0.5 `:655` | 0.5 `:36` | params → 0.5 default | = |
| tank mod shape | 0.5 `:656` | 0.5 `:37` | never called → 0.5 (ctor) | = |
| clear | `:657` | `:38` | `:74` | = |

## Octave branch

| Concern | earth.cpp | Web | VST | Verdict |
| --- | --- | --- | --- | --- |
| Domain | fixed 48 kHz | AudioContext rate | resampled to 48 kHz | Web ≠ (item 8) |
| Decimator/Interp | `Multirate.h` | same | same | = |
| OctaveGenerator rate | 8000 `:61` | `sampleRate/6` `:20` | 8000 `:78` | Web ≠ |
| Up gain | `up1*2` `:485` | `up1*2` `:118` | `up1*2` `:352` | = |
| Down gain | `down1*2+down2*2` `:487-488` | `:120-121` | `:355-356` | = |
| Shelves | 140/160 Hz @48k `:62-63` | @stream rate `:21-22` | @8000 Hz, applied @48k `:82-83,362` | **VST ≠ (item 7)** |
| Inner dry | `!dip2 || mode==2` `:499` | always (mode 2 or not-only) `:135` | `!octave_dry_mix || mode==2` `:366` | **≠ (item 5)** |
| Enum | 0/1/2 `:484-489` | 0/1/2 `:117-121` | 0/1/2/3 `:46,351-357` | **≠ (item 6)** |
| Gate to reverb | footswitch logic `:516-522` | mode!=0 `:155-157` | footswitch + momentary `:461-465` | ≈ |
| NaN clamp | none | yes `:124-128,144-146,159-161` | none | Web extra guard |

## Output / utility

| Concern | earth.cpp | Web | VST | Verdict |
| --- | --- | --- | --- | --- |
| dry/wet law | energy crossfade `:405-418` | same `:191-197` | same `:223-232` | = |
| wet gain | `*0.4` `:543` | `*0.4` `:168` | `*0.4` `:488` | = |
| dry delay | none | none | yes `:483-486` | ≠ (item 9) |
| bypass | physical FS1 | engine suspend | `bypassFade` crossfade `:257,434,492` | different scope |
| overdrive | `overdrive1/2` `:533-541` | not exposed | `overdriveLeft/Right` `:475-477` | VST added |
| freeze | `current_freezeDecay→1` `:316-321` | not exposed | same `:415-419` | VST added |

## Dattorro copies

`Dattorro/` (root, used by Web/WASM) and `Apollo/Source/DSP/Dattorro/` are
near-identical. The only functional differences:

| File | Root | Apollo | Consequence |
| --- | --- | --- | --- |
| `Dattorro.cpp` pre-delay alloc | fixed `InterpDelay(37000,0.)` `:303`, not resized `:395-411` | `ceil(sr*1s)+1`, resized in `setSampleRate` `:406` | root unsafe >48 kHz (item 10) |
| `Dattorro.cpp` include | no `<cmath>` | `<cmath>` | – |

Everything else (tank scale bug, `maxSampleRate=32000`) is identical.

## Files that must become the single source of truth

```
shared/
  EarthDSPCore.{h,cpp}     # parameter -> DSP, no UI, no host
  EarthParameters.h        # canonical struct + enums
  Dattorro/                # merged from both copies (dynamic pre-delay)
  Octave/                  # OctaveGenerator + BandShifter
  Multirate/               # Decimator2 + Interpolator
  Filters/                 # one shelf implementation
  Resampling/              # host <-> 48 kHz
Apollo -> thin APVTS adapter -> EarthDSPCore
Web    -> thin AudioWorklet adapter -> EarthDSPCore
```
