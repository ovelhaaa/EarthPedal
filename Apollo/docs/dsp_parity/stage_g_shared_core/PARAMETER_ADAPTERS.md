# Parameter Adapters

Both wrappers translate their native values into the canonical struct below.
This is the only allowed place for parameter translation; no sonic mapping
belongs in the wrappers.

## Canonical struct

```cpp
struct EarthParameters {
    float preDelaySeconds;       // seconds, 0 .. 1
    float mix;                   // 0 .. 1
    float decay;                 // 0 .. 1
    float modulationDepth;       // 0 .. 1  -> excursion = value * 8
    float modulationSpeed;       // 0 .. 1  -> Hz = 0.3 + value * 15
    float damp;                  // 0 .. 1
    float octaveHighShelfDb;     // dB, default -11 @140 Hz (designed @48 kHz)
    float octaveLowShelfDb;      // dB, default  +5 @160 Hz (designed @48 kHz)
    ReverbSize reverbSize;       // Small / Medium / Large
    OctaveMode octaveMode;       // Off / Up / Down / Both
    PerformanceMode performanceMode; // Freeze / Overdrive / Octave
    bool inputDiffusion;
    bool includeDryInOctavePath;
    bool performanceActive;
    bool bypass;
};
```

## JUCE / APVTS -> EarthParameters

All ids preserved. Values are normalised 0..1 unless noted.

| APVTS id | type | -> core field | mapping |
| --- | --- | --- | --- |
| `predelay` | float 0..1 | `preDelaySeconds` | direct (seconds) |
| `mix` | float 0..1 | `mix` | direct |
| `decay` | float 0..1 | `decay` | direct |
| `moddepth` | float 0..1 | `modulationDepth` | direct |
| `modspeed` | float 0..1 | `modulationSpeed` | direct |
| `damp` | float 0..1 | `damp` | direct |
| `eq1_gain` | float dB | `octaveHighShelfDb` | direct |
| `eq2_gain` | float dB | `octaveLowShelfDb` | direct |
| `time_scale` | choice 0/1/2 | `reverbSize` | Small/Medium/Large direct |
| `effect_mode` | choice 0/1/2/3 | `octaveMode` | 0→Off, 1→Up, 2→**Down**, 3→Both |
| `footswitch_mode` | choice 0/1/2 | `performanceMode` | Freeze/Overdrive/Octave direct |
| `input_diffusion` | bool | `inputDiffusion` | direct |
| `octave_dry_mix` | bool | `includeDryInOctavePath` | **see below** |
| `bypass` | bool | `bypass` | direct |
| `momentary_effect` | bool | `performanceActive` | direct |

### `octave_dry_mix` polarity (legacy -> canonical)

The legacy VST code added the inner dry when `!octave_dry_mix || effect_mode == 2`
— a double negation with a mode exception. The canonical semantic is positive:
`includeDryInOctavePath == true` adds `0.5 * dry` to the octave branch for every
active octave mode, which is what the Web/earth reference does and what the
Stage F octave-routing matrix measured.

**Decision (G7):** the adapter maps `includeDryInOctavePath = octave_dry_mix`
directly. The id and the stored boolean are preserved, but the *effective*
meaning is inverted relative to the legacy double negation, so the default
(`octave_dry_mix = true`) now includes the inner dry and matches the Web
reference. This is a deliberate, documented compatibility change; it is the
single intended polarity change and is listed in `MIGRATION_LOG.md`.

## Web / AudioWorklet -> EarthParameters

The worklet exposes normalised parameters by name. Same canonical units.

| Worklet param | -> core field | mapping |
| --- | --- | --- |
| `preDelay` | `preDelaySeconds` | direct (seconds) |
| `mix` | `mix` | direct |
| `decay` | `decay` | direct |
| `modDepth` | `modulationDepth` | direct |
| `modSpeed` | `modulationSpeed` | direct |
| `filter` | `damp` | direct |
| `eq1Gain` | `octaveHighShelfDb` | direct |
| `eq2Gain` | `octaveLowShelfDb` | direct |
| `reverbSize` | `reverbSize` | 0/1/2 direct |
| `octaveMode` | `octaveMode` | 0→Off, 1→Up, 2→**Both** (legacy Web); UI to expose Down separately |
| `disableInputDiffusion` | `inputDiffusion` | negation |

### Octave enum divergence

The Web UI historically maps 0=Off, 1=Up, 2=Up+Down (no separate Down). The
canonical enum has four states. The Web adapter maps its values into
`OctaveMode`; the Web UI will offer `Down` separately in G6. The DSP never sees
raw integers.

## Defaults

`EarthParameters::defaults()` is the single source of truth and reproduces the
Golden: `decay 0.877465`, `modulationDepth 0.0625`, `modulationSpeed 0.0466667`,
`preDelaySeconds 0`, `mix 0.5`, `damp 0.5`, `reverbSize Large`,
`octaveMode Off`, `inputDiffusion true`, `includeDryInOctavePath true`.
The Web page, the JS defaults and the APVTS defaults must eventually be derived
from (or checked against) this struct rather than duplicated literals.
