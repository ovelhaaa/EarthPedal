# EarthDSPCore — Shared DSP Core

Stage G goal: one portable DSP core used by both the Web/WASM build and the
Apollo JUCE plugin, so no sonic rule lives in the wrappers.

```
        Web/WASM adapter \                 / JUCE/APVTS adapter
                         > EarthDSPCore <
```

## Status of this milestone

| Step | Description | Status |
| --- | --- | --- |
| G0 | Commit/push Stage F checkpoint | done (`6df0b21`) |
| G1 | `EarthEnums`, `EarthParameters`, `EarthRateContext`, `EarthTimebase` | done |
| G2 | Consolidated shared Dattorro with explicit rate context (LegacySrInvariant) | done |
| G3 | `EarthDSPCore` reverb / output / mix / pre-delay / damp / freeze path | done |
| G3 tests | Golden @48k bit-exact, SR invariance, block invariance | done, passing |
| G4 | Shared octave engine (multirate, shared shelves, 48k canonical domain) | done |
| G5 | Shared overdrive + freeze routing | done |
| G9 (partial) | CI gate building/running the core tests on Linux | done |
| G6 | Web/WASM adapter on the core | planned |
| G7 | JUCE adapter on the core | planned |
| G8 | Remove duplicated legacy DSP after validation | planned |

The core now covers the reverb, the octave branch (canonical 48 kHz domain),
the overdrive and freeze. The Golden set includes P0/P1 (reverb),
P3/P4/P5 (octave) and P7 (overdrive), all bit-exact at 48 kHz. The wrappers are
still not migrated; that is G6/G7.

## Layout

```
shared/
    EarthDSPCore.h/.cpp     # the core: parameters -> DSP, no UI, no host
    EarthParameters.h       # canonical parameter struct + defaults
    EarthEnums.h            # OctaveMode / ReverbSize / PerformanceMode
    EarthRateContext.h      # explicit sample-rate roles
    EarthTimebase.h         # TimebaseModel + makeRateContext
    Dattorro/               # consolidated reverb (rate-context aware)
    Multirate/              # Decimator2 + Interpolator (48k canonical)
    Octave/                 # OctaveGenerator + BandShifter
    Filters/                # shared RBJ shelf filters
    Resampling/             # host <-> 48 kHz for the octave branch
    Effects/                # shared DaisySP-derived overdrive
    CMakeLists.txt
    tests/
        make_golden.cpp     # builds the golden from the production reference
        golden_test.cpp     # golden + SR + block invariance
        golden/*.f32        # frozen 48 kHz references (committed)
```

No dependency on JUCE, Emscripten, WebAudio or Daisy hardware exists inside
`shared/`.

## Build and test

```bash
cmake -S shared -B shared/build -DCMAKE_BUILD_TYPE=Release
cmake --build shared/build -j
ctest --test-dir shared/build --output-on-failure
```

See `GOLDEN_TESTS.md` for the results and `RATE_CONTEXT.md` for the single most
important design decision (why the tank timing rate is `host * 2/3`).
