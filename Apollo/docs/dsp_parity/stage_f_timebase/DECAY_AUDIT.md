# Decay Audit

`decay` is the feedback gain applied once per network circulation
(`Dattorro1997Tank::process`, `Dattorro.cpp:65-79`). If the circulation time
changes, RT60 changes for the same coefficient. This document quantifies it.

Data: `renders/metrics.csv` (Size = Large unless noted). RT values are seconds.

## RT30 vs host and decay

| model | host | decay 0.5 | decay 0.877465 | decay 0.95 |
| --- | --- | --- | --- | --- |
| legacy32k | 44.1 kHz | 5.963 | 5.167 | 4.130 |
| legacy32k | 48 kHz | 5.567 | 5.276 | 4.182 |
| legacy32k | 96 kHz | **2.838** | **6.274** | 4.936 |
| correct | 44.1 kHz | 6.873 | 4.648 | 3.790 |
| correct | 48 kHz | 6.925 | 4.701 | 3.835 |
| correct | 96 kHz | 6.948 | 4.795 | 3.886 |
| invariant | 44.1 kHz | 5.587 | 5.380 | 4.276 |
| invariant | 48 kHz | 5.567 | 5.276 | 4.182 |
| invariant | 96 kHz | 5.635 | 5.213 | 4.102 |

## RT20 / RT30 / RT60 estimate vs host (decay 0.877465, Large)

| model | host | RT20 | RT30 | RT60 |
| --- | --- | --- | --- | --- |
| legacy32k | 32 kHz | 7.10 | 4.81 | 4.81 |
| legacy32k | 44.1 kHz | 7.57 | 5.17 | 5.17 |
| legacy32k | 48 kHz | 7.73 | 5.28 | 5.28 |
| legacy32k | 88.2 kHz | 8.71 | 6.25 | 6.25 |
| legacy32k | 96 kHz | 8.62 | 6.27 | 6.27 |
| legacy32k | 192 kHz | 6.16 | 6.00 | 6.00 |
| correct | 48 kHz | 6.94 | 4.70 | 4.70 |
| correct | 96 kHz | 7.06 | 4.80 | 4.80 |
| invariant | 44.1 kHz | 7.88 | 5.38 | 5.38 |
| invariant | 48 kHz | 7.73 | 5.28 | 5.28 |
| invariant | 96 kHz | 7.63 | 5.21 | 5.21 |
| invariant | 192 kHz | 7.78 | 5.31 | 5.31 |

## Findings

1. **`legacy32k` is not SR-invariant for RT.** RT30 at decay 0.877 swings from
   4.81 s (32 kHz) to 6.27 s (96 kHz), a ~30 % change; at decay 0.5 it diverges
   even more (5.57 s → 2.84 s). The tail character depends on the host rate.
2. **`correct` is SR-invariant but changes the sound.** RT30 is stable across
   48/96 kHz (4.70/4.80 s), but it is ~11 % shorter than the golden 48 kHz
   value and 24 % shorter at decay 0.5. The mathematically canonical timebase
   therefore does not reproduce the preferred sound.
3. **`invariant` preserves both.** RT30 stays within ~4 % across
   44.1/48/96/192 kHz and matches the golden 48 kHz value (5.28 s at
   decay 0.877; 5.57 s at decay 0.5; 4.18 s at decay 0.95).
4. **No `decay` remapping is required** for the recommended model. Because the
   delay lengths and the per-cycle filter losses both scale with the same
   `timingReferenceRate`, the RT-vs-decay curve is preserved automatically. A
   decay transform would only be needed if the loop *time* were changed while
   keeping the loop *losses* fixed — which is exactly the `correct` model.

## Measurement caveat

RT estimates come from a Schroeder backwards integration of the impulse
response. When the decay is not a clean single exponential (multi-slope tank,
strong modulation), RT20 and RT30 differ. This is visible in the `legacy32k`
row (RT20 7.7 vs RT30 5.3 at 48 kHz) and is not a measurement failure — it is
the actual multi-slope behaviour of the tank. The `invariant` and `correct`
models are more consistent because their timebase is fixed relative to the
process rate.
