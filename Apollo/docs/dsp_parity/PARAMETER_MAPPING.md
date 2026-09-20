# Canonical Parameter Mapping

All parameters are exposed to the UI/APVTS/AudioWorklet as normalised floats in
`[0, 1]` (except EQ gains in dB and the enum choices). The conversion to DSP
values happens **once**, in the shared core. The wrappers must not contain any
of this math.

Source of truth: `earth.cpp` initialisation and `processSmoothedParameters()`.

| UI param | Normalised | DSP value | Formula | earth.cpp |
| --- | --- | --- | --- | --- |
| `preDelay` | 0..1 | seconds 0..1 | `seconds = norm` | `:291` |
| `mix` | 0..1 | dry/wet gains | see crossfade below | `:405-418` |
| `decay` | 0..1 | tank feedback | `feedback = norm` | `:321` |
| `modDepth` | 0..1 | LFO excursion | `depth = norm * 8.0` | `:295` |
| `modSpeed` | 0..1 | LFO rate | `speed = 0.3 + norm * 15.0` | `:299` |
| `damp` | 0..1 | input cut pitch | `<0.5: highCutPitch = 7*(2*damp)+3` else `lowCutPitch = 9*(2*(damp-0.5))` | `:444-457` |
| `eq1_gain` | dB | high shelf gain | `-11 dB @ 140 Hz` | `:62` |
| `eq2_gain` | dB | low shelf gain | `+5 dB @ 160 Hz` | `:63` |
| `time_scale` | choice | time scale | `Small=1, Medium=2, Large=4` | `:89-104` |
| `octaveMode` | choice | `OctaveMode` | see enum table | `:484-489` |
| `input_diffusion` | bool | diffusion enable | `enableInputDiffusion(bool)` | `:460` |
| `includeDryInOctavePath` | bool | inner dry | `mix += 0.5*dry` when true | `:499` |
| `perform`/`footswitch_mode` | choice | momentary action | Freeze / Overdrive / Effect | `:172-193` |

## mix — energy-preserving crossfade

```cpp
x2 = 1 - mix;
A  = mix * x2;
B  = A * (1 + 1.4186*A);
C  = B + mix;      wet = C*C;
D  = B + x2;       dry = D*D;
```
Output: `out = in*dry + reverbOut*wet*0.4`. The `0.4` headroom factor is part of
the sound (earth.cpp:543-544) and must be preserved.

## Defaults (canonical — hardware Earth)

| Param | Normalised default | DSP target |
| --- | --- | --- |
| preDelay | 0.0 | 0 s |
| mix | 0.5 | 50 % |
| decay | 0.877465 | `setDecay(0.877465)` |
| modDepth | 0.0625 | `0.0625 * 8 = 0.5` |
| modSpeed | 0.0466667 | `0.3 + 0.0466667*15 = 1.0` |
| damp | 0.5 | low cut pitch 0 → 13.75 Hz |
| eq1_gain | -11 dB | high shelf 140 Hz |
| eq2_gain | +5 dB | low shelf 160 Hz |
| time_scale | Large (index 2) | 4.0 |
| octaveMode | Off | — |
| input_diffusion | true | enable |
| includeDryInOctavePath | true | inner 0.5*dry |

### Derivation of the modDepth/modSpeed defaults

* Hardware calls `setTankModDepth(0.5)` (`earth.cpp:655`); the normalised path is
  `setTankModDepth(norm * 8)` (`earth.cpp:295`), therefore
  `norm = 0.5 / 8 = 0.0625`.
* Hardware calls `setTankModSpeed(1.0)` (`earth.cpp:654`); the normalised path is
  `0.3 + norm * 15` (`earth.cpp:299`), therefore
  `norm = (1.0 - 0.3) / 15 = 0.0466667`.

Note: the Web UI default `modDepth = 0.5` would yield `setTankModDepth(4.0)`,
about 8x the intended hardware excursion. This is why the defaults must be
unified before any listening test.

## OctaveMode (shared enum)

```cpp
enum class OctaveMode { Off = 0, Up = 1, Down = 2, Both = 3 };
```

Adapter rules (do **not** change stored APVTS indices / Web UI values):

| Target value | Meaning | -> OctaveMode |
| --- | --- | --- |
| Web/hardware 0 | Off | Off |
| Web/hardware 1 | Up | Up |
| Web/hardware 2 | Up + Down | Both |
| APVTS 0 | None | Off |
| APVTS 1 | Up Octave | Up |
| APVTS 2 | Down Octave | Down |
| APVTS 3 | Both Octaves | Both |

## includeDryInOctavePath (formerly `octave_dry_mix`)

Current VST code (`PluginProcessor.cpp:366`) adds the inner dry when
`!octave_dry_mix || effect_mode == 2`. Because the APVTS default is `true`, the
Up mode gets no inner dry — unlike hardware/Web.

Canonical semantic: **`includeDryInOctavePath == true` adds `0.5 * dry` to the
octave branch for every active octave mode.**

To preserve saved sessions without changing the stored polarity, the adapter
maps the legacy id as:

```cpp
includeDryInOctavePath = legacy_octave_dry_mix;   // if old default meant "include"
```

The exact polarity must be confirmed by the Up/Down/Both × on/off render matrix
(see `PARITY_RESULTS.md`), then frozen. Until then the DSP keeps the legacy
behaviour behind the adapter.
