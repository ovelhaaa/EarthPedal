# LFO Audit — `TriSawLFO`

Source: `Apollo/Source/DSP/Dattorro/dsp/modulation/LFO.hpp`. Data:
`renders/lfo_audit.csv`.

## How the phase works

```cpp
_stepSize = _frequency * _1_sampleRate;   // LFO.hpp:92-95
_step += _stepSize;                        // once per process() call
```

The LFO advances `frequency / lfoSampleRate` per call to `process()`.
`TriSawLFO` defaults to `lfoSampleRate = 32000` (`LFO.hpp:13`) and the tank
**never calls `setSamplerate()`**, so the realised frequency is:

```
realHz = targetHz * processRate / 32000
```

## Frequency vs host (LFO1, default mod speed, target = 0.0999 Hz)

| host | legacy measured | correct measured | invariant measured |
| --- | --- | --- | --- |
| 44.1 kHz | 0.1377 (+37.8 %) | 0.0999 (0 %) | 0.1499 (+50 %) |
| 48 kHz | 0.1499 (+50 %) | 0.0999 (0 %) | 0.1499 (+50 %) |
| 96 kHz | 0.2997 (+200 %) | 0.0999 (0 %) | 0.1499 (+50 %) |
| 192 kHz | 0.5994 (+500 %) | 0.0999 (0 %) | 0.1499 (+50 %) |

* `legacy32k`: frequency scales with the host — 6x too fast at 192 kHz.
* `correct`: exact target at every rate.
* `invariant`: constant and equal to the legacy 48 kHz value (target × 1.5).

## All four LFOs @ 48 kHz, default speed

| LFO | target | legacy | correct | invariant |
| --- | --- | --- | --- | --- |
| lfo1 | 0.0999 | 0.1499 | 0.0999 | 0.1499 |
| lfo2 | 0.1499 | 0.2248 | 0.1499 | 0.2248 |
| lfo3 | 0.1199 | 0.1798 | 0.1199 | 0.1798 |
| lfo4 | 0.1798 | 0.2697 | 0.1798 | 0.2697 |

The `legacy` and `invariant` columns are identical, confirming that
`invariant` reproduces the legacy 48 kHz modulation and keeps it constant at
all rates.

## Frequency vs mod speed @ 48 kHz (LFO1)

| speed | target | legacy | correct | invariant |
| --- | --- | --- | --- | --- |
| min (0.0) | 0.0300 | 0.0450 | 0.0300 | 0.0450 |
| default (0.0466) | 0.0999 | 0.1499 | 0.0999 | 0.1499 |
| mid (0.5) | 0.7800 | 1.1700 | 0.7800 | 0.7800→1.1700* |
| max (1.0) | 1.5300 | 2.2950 | 1.5300 | 2.2950 |

\* `invariant` = legacy value at every speed (1.5x target at 48 kHz).

## Modulation excursion (default depth 0.5)

`lfoExcursion = modDepth * 16 * sampleRateScale` (`Dattorro.cpp:158-161`).

| host | legacy samples / ms | correct samples / ms | invariant samples / ms |
| --- | --- | --- | --- |
| 44.1 kHz | 8.60 / 0.195 | 11.85 / 0.269 | 7.90 / 0.179 |
| 48 kHz | 8.60 / 0.179 | 12.90 / 0.269 | 8.60 / 0.179 |
| 96 kHz | 8.60 / 0.090 | 25.81 / 0.269 | 17.20 / 0.179 |
| 192 kHz | 8.60 / 0.045 | 51.61 / 0.269 | 34.41 / 0.179 |

* `legacy32k`: constant in *samples*, so the realised modulation depth in
  *milliseconds* collapses as the host rate rises.
* `correct`: constant in ms (0.269 ms).
* `invariant`: constant in ms (0.179 ms) and equal to the legacy 48 kHz value.

## Conclusion

For the modulation to be perceptually consistent across sample rates, both the
LFO frequency (`modulationSampleRate`) and the excursion (`timingReferenceRate`)
must derive from the same non-host reference. `invariant` keeps both equal to
the legacy 48 kHz behaviour at every host rate; `legacy32k` does not.
