# Internal Delay Matrix

All values from `renders/internal_delays.csv` for **Size = Large**
(`timeScale = 4`). Left/right tank delays are in ms as heard at the host rate.

## Tank delays (ms)

| delay | legacy 44.1 | correct 44.1 | invariant 44.1 | legacy 48 | correct 48 | invariant 48 | legacy 96 | correct 96 | invariant 96 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| leftApf1 | 65.54 | 90.32 | 60.21 | 60.21 | 90.32 | 60.21 | 30.11 | 90.32 | 60.21 |
| leftDelay1 | 434.29 | 598.50 | 399.00 | 399.00 | 598.50 | 399.00 | 199.50 | 598.50 | 399.00 |
| leftApf2 | 175.55 | 241.93 | 161.28 | 161.28 | 241.93 | 161.28 | 80.64 | 241.93 | 161.28 |
| leftDelay2 | 362.80 | 499.98 | 333.32 | 333.32 | 499.98 | 333.32 | 166.66 | 499.98 | 333.32 |
| rightApf1 | 88.55 | 122.04 | 81.36 | 81.36 | 122.04 | 81.36 | 40.68 | 122.04 | 81.36 |
| rightDelay1 | 411.27 | 566.78 | 377.86 | 377.86 | 566.78 | 377.86 | 188.93 | 566.78 | 377.86 |
| rightApf2 | 259.03 | 356.98 | 237.99 | 237.99 | 356.98 | 237.99 | 118.99 | 356.98 | 237.99 |
| rightDelay2 | 308.48 | 425.12 | 283.41 | 283.41 | 425.12 | 283.41 | 141.71 | 425.12 | 283.41 |

## Input all-passes (ms) — outer Dattorro, host rate in all models

| delay | 44.1 | 48 | 96 |
| --- | --- | --- | --- |
| inApf1 | 4.74 | 4.74 | 4.74 |
| inApf2 | 3.60 | 3.60 | 3.60 |
| inApf3 | 12.73 | 12.73 | 12.73 |
| inApf4 | 9.31 | 9.31 | 9.31 |

The input APFs and pre-delay already use the host rate
(`Dattorro::setSampleRate`, `Dattorro.cpp:402-421`) in every model, so they are
SR-invariant by construction. Only the tank was compressed.

## Output taps (ms, first tap shown; full table in CSV)

| tap | legacy 44.1 | correct 44.1 | invariant 44.1 | legacy 48 | correct 48 | invariant 48 | legacy 96 | correct 96 | invariant 96 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| L_DELAY1_T1 | 6.49 | 8.93 | 5.94 | 5.96 | 8.94 | 5.96 | 2.98 | 8.94 | 5.96 |
| L_DELAY1_T2 | 72.49 | 99.79 | 66.52 | 66.53 | 99.80 | 66.53 | 33.27 | 99.80 | 66.53 |
| R_DELAY2_T | 25.98 | 35.76 | 23.84 | 23.84 | 35.75 | 23.84 | 11.92 | 35.75 | 23.84 |

## Reading the table

* **`legacy32k`**: the *sample* length is constant (e.g. `leftDelay1 = 19152`
  samples at every rate) so the *time in ms* collapses as the host rate rises
  (399 → 199.5 ms from 48 → 96 kHz).
* **`correct`**: the *time in ms* is constant (598.50 ms) at every rate; it is
  **1.5x** the legacy 48 kHz value.
* **`invariant`**: the *time in ms* is constant (399.00 ms) and equal to the
  legacy 48 kHz value.

Sample-domain view for `leftDelay1`:

| host | legacy samples | correct samples | invariant samples |
| --- | --- | --- | --- |
| 48 kHz | 19152 | 28728 | 19152 |
| 96 kHz | 19152 | 57456 | 38304 |

Only `invariant` keeps both a constant *physical* delay and the legacy 48 kHz
value across every sample rate.
