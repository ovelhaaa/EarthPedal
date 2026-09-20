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
| Reverb initialisation, mix, damp, pre-delay, freeze | yes (`EarthDSPCore`) | validated P0/P1/P7 |
| Smoothing windows | yes | 5 ms, in the core |
| Octave engine (multirate, 48 kHz canonical, shared shelves) | yes | validated P3/P4/P5 |
| Overdrive | yes (`shared/Effects`) | validated P7 |
| Latency reporting | partial (`getLatencySamples()` returns 0) | finalised with G6/G7 |
| WASM adapter | yes (`src/wasm_wrapper.cpp` -> core) | not build-verified (no Emscripten) |
| JUCE adapter | yes (`PluginProcessor.{h,cpp}` -> core) | not build-verified (no JUCE) |

## Consequence

Both wrappers now call `EarthDSPCore` and contain no DSP. The remaining work is
verification (G8): build the Web module with Emscripten and the plugin with
JUCE, then run the Web↔VST null test. Until then the wrapper changes are
unverified at compile time, although they use only the core API covered by the
golden tests.

Two deliberate behaviour changes are documented in `MIGRATION_LOG.md`: the
`octave_dry_mix` polarity now follows the canonical positive semantic, and the
dry signal is no longer delayed (the octave only excites the reverb).

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

Yes at the source level: both adapters call `EarthDSPCore` and contain no DSP
code. Whether the built artifacts are numerically identical is unverified here
because neither toolchain is available; that is the G8 null test.
