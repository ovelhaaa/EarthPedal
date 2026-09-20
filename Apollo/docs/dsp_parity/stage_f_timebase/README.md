# Stage F — Tank Timebase Forensics

Investigation of the `Dattorro1997Tank` sample-rate conflation and the choice of
a definitive timebase for Apollo, **without changing production**.

Reference sound for this stage: the current Web / Apollo Stage B behaviour at
48 kHz (`golden_legacy_48k`).

## Documents

| File | Contents |
| --- | --- |
| `TIMEBASE_ANALYSIS.md` | All sample-rate consumers, the three models, the root cause |
| `INTERNAL_DELAY_MATRIX.md` | Delay/tap lengths in samples and ms for every model × host |
| `LFO_AUDIT.md` | `TriSawLFO` frequency and excursion across host rates |
| `DECAY_AUDIT.md` | RT20/RT30/RT60 vs host and decay, and whether decay needs remapping |
| `RESULTS.md` | Full matrix: metrics, null tests, plots |
| `RECOMMENDATION.md` | Final timebase recommendation and next-step proposal |

## Harness

```
Apollo/tools/dsp_parity/timebase_harness.cpp   # JUCE-free, builds with MinGW g++
Apollo/tools/dsp_parity/stage_f_analyze.py     # plots + null report
Apollo/docs/dsp_parity/stage_f_timebase/renders/   # CSVs, WAVs (WAVs git-ignored)
Apollo/docs/dsp_parity/stage_f_timebase/plots/     # PNGs
```

Rebuild:

```powershell
g++ -std=c++20 -O2 -IApollo/Source/DSP -IApollo/Source/DSP/Dattorro -IApollo/Source/DSP/Util `
    Apollo/tools/dsp_parity/timebase_harness.cpp `
    Apollo/Source/DSP/Dattorro/Dattorro.cpp `
    Apollo/Source/DSP/Dattorro/dsp/delays/InterpDelay.cpp `
    Apollo/Source/DSP/Dattorro/dsp/filters/OnePoleFilters.cpp `
    -o Apollo/tools/dsp_parity/timebase_harness.exe
Apollo/tools/dsp_parity/timebase_harness.exe Apollo/docs/dsp_parity/stage_f_timebase/renders
python Apollo/tools/dsp_parity/stage_f_analyze.py Apollo/docs/dsp_parity/stage_f_timebase/renders Apollo/docs/dsp_parity/stage_f_timebase/plots
```

## Models

| Model | Tank internal rate | Real delay times | LFO real Hz |
| --- | --- | --- | --- |
| `legacy32k` | `min(host, 32000)` | host-dependent | host-dependent |
| `correct` | `host` | canonical, SR-invariant | target |
| `invariant` | `host * 2/3` | = legacy @ 48 kHz, SR-invariant | = legacy @ 48 kHz, SR-invariant |

`invariant` is implemented by setting `tank.maxSampleRate = host*2/3` and
syncing the tank cut filters and the four LFOs to that rate. No production file
was modified.
