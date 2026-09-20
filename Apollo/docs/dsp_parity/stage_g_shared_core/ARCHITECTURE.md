# Architecture

## Principle

All sonic rules live in `shared/EarthDSPCore`. Wrappers do four things only:

1. read their host's parameter representation;
2. translate it into `earth::EarthParameters`;
3. call `prepare()` / `setParameters()` / `process()`;
4. report latency to the host.

No wrapper may implement the mix law, octave gains, dry routing, Dattorro
initialisation, time scale, modulation mapping, shelf behaviour, overdrive
routing, freeze routing, internal latency or sample-rate policy.

## Core API

```cpp
namespace earth {

class EarthDSPCore {
public:
    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void setParameters(const EarthParameters& parameters);
    void snapParameters();                 // apply targets immediately (offline)
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, int numSamples);
    int  getLatencySamples() const;

    // research / diagnostics only
    void setTimebaseModel(TimebaseModel);
    const EarthRateContext& rateContext() const;
    const Dattorro& reverb() const;
};

}
```

`prepare()` allocates and applies the fixed historical initialisation
(tank diffusion 0.7, input/tank cut filters, mod shape 0.5) and the timebase.
`setParameters()` sets targets; a per-sample linear smoother ramps toward them.
Smoothing windows live in the core so Web and JUCE ramp identically.

## Dependency direction

```
shared/EarthDSPCore
    depends on -> shared/Dattorro (rate-context aware)
    depends on -> EarthParameters / EarthEnums / EarthRateContext / EarthTimebase
    must NOT depend on JUCE / Emscripten / WebAudio / Daisy
```

The JUCE adapter (planned, G7) keeps all APVTS ids and translates to
`EarthParameters`. The WASM adapter (planned, G6) keeps the AudioWorklet
parameter names and translates to `EarthParameters`.

## Timebase

See `RATE_CONTEXT.md`. Summary: `processSampleRate = host`,
`timingReferenceRate = filterSampleRate = modulationSampleRate = host * 2/3`.
This is `TimebaseModel::LegacySrInvariant`, the production default.
`TimebaseModel::Correct` (tank = host) exists for research only and is not
exposed in the UI.

## Consolidation status

`shared/Dattorro` is the single reverb implementation. At the file level it is
the Apollo copy plus the new `setRateContext()` methods; the algorithm is
otherwise byte-identical to both existing copies (verified by diff, see
`MIGRATION_LOG.md`). The three duplicates
(`/Dattorro`, `/Apollo/Source/DSP/Dattorro`, `/shared/Dattorro`) will collapse to
`/shared/Dattorro` in G8 after the adapters are validated.
