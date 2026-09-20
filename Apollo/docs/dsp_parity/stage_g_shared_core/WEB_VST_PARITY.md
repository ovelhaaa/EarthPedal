# Web ↔ VST Parity

## Current status

The shared core exists and is validated for the reverb/output path, but the two
production wrappers have **not yet been switched to it**. This is deliberate:
the migration is incremental and no production file was modified in this
milestone.

| Piece | Shared? | Notes |
| --- | --- | --- |
| Dattorro reverb | yes (`shared/Dattorro`) | bit-exact with production at 48 kHz |
| Timebase policy | yes (`EarthTimebase`) | LegacySrInvariant default |
| Parameters / enums | yes | single source of truth |
| Reverb initialisation, mix, damp, pre-delay, freeze | yes (`EarthDSPCore`) | validated P0/P1/P6/P8/P9 |
| Smoothing windows | yes | 5 ms, in the core |
| Octave engine (multirate, 48 kHz canonical, shelves) | **no** | G4 |
| Overdrive | **no** | G5 |
| Latency reporting | partial (`getLatencySamples()` returns 0) | finalised with G4 |
| WASM adapter | **no** (`src/wasm_wrapper.cpp` unchanged) | G6 |
| JUCE adapter | **no** (`PluginProcessor.cpp` unchanged, except the Stage 1 fixes) | G7 |

## Consequence

Because the octave branch is not yet in the core, `EarthDSPCore` cannot yet be
used verbatim by the wrappers. The golden tests therefore cover the reverb path
that *is* shared. The plan is to finish the core before touching the wrappers,
so that the wrapper swap (G6/G7) is a mechanical change validated by the same
tests.

## Planned octave engine (G4)

Canonical 48 kHz domain, identical in both targets:

```
host/process SR -> anti-alias/resample -> 48 kHz -> /6 -> 8 kHz
     -> OctaveGenerator -> *6 -> 48 kHz -> shelves(@48 kHz)
     -> resample -> host/process SR
```

The shelves use one shared implementation (no JUCE, no cycfi q) designed at
48 kHz: high shelf −11 dB @140 Hz, low shelf +5 dB @160 Hz.

## Latency / dry alignment

The core owns the octave-branch algorithmic latency and will report it through
`getLatencySamples()`. The policy must not delay the dry signal when
`octaveMode == Off`; the A/B/C measurement (always delay / delay only when the
octave is active / other) from Stage F item 9 is still required. Until the
octave engine exists the latency is zero and the dry path is un-delayed, which
is the Golden reference.

## Web ↔ VST null test

Not yet possible: it requires both adapters on the core and an Emscripten
toolchain (absent in this environment). The test will render the same input at
the same parameters/sample rate/block size through both and report peak error,
RMS error, null depth and correlation, aligning only proven latency.

## Answer to "do Web and VST run the same algorithm?"

Not yet. They now have a validated shared core for the reverb/output path, but
the wrappers still contain the legacy DSP until G6/G7. No sonic rule has been
de-duplicated in the wrappers yet.
