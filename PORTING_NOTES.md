# Apollo DSP Porting Notes

## Resampling Architecture & Aliasing

The original Apollo architecture relied on a block-by-block `LagrangeInterpolator` (via JUCE) without an explicit anti-aliasing filter. This caused two significant issues when ported to variable host sample rates:

1. **High-Frequency Aliasing (96kHz to 48kHz)**: The `LagrangeInterpolator` drops samples when downsampling but does not low-pass filter the signal first. This caused any high-frequency energy (above 24kHz) to fold back (alias) into the audible spectrum.
   - **Solution**: We added a dedicated 8th-order Butterworth Low-Pass Filter (`juce::dsp::IIR::Filter`) running at the host's native rate, *before* the resampler. The cutoff is dynamically calculated as `min(23000.0, HostSampleRate * 0.45)` to ensure that no energy above 24kHz ever reaches the downsampler, eliminating aliasing at 96kHz.

2. **Fractional Block Jitter & Phase Resets (44.1kHz to 48kHz)**: When upsampling from 44.1kHz to 48kHz, the block size ratio is `48000 / 44100 = 1.0884`. The interpolator consumes a fractional number of input samples (e.g., 255.4). Because the host plugin architecture provides discrete audio blocks (e.g., 256 samples), discarding the unconsumed fractional remainder (0.6 samples) caused the interpolator's internal phase to jump at every block boundary. This resulted in severe phase cancellation of the 440Hz fundamental at 44.1kHz (-48dB) compared to 96kHz (-35.9dB, where the ratio is an exact integer 2.0).
   - **Solution**: We implemented a **Sliding Window FIFO** architecture for both the input and output resampling stages. Unconsumed input samples are now shifted to the front of the FIFO and prepended to the next host block, ensuring continuous, unbroken phase alignment. We also pre-filled the output downsampling FIFO with 64 samples of latency (~1.3ms at 48kHz) to absorb the fractional block jitter and prevent underruns.

## Results of DSP Architecture Fix

After applying the 8th-order Butterworth anti-aliasing filter and the Sliding Window FIFOs, the 440Hz leakage metrics (relative to the 880Hz peak) stabilized across all sample rates. By eliminating the phase jitter at 44.1kHz and the aliasing at 96kHz, the relative energy levels are now mathematically sound and perceptually consistent.

(See GH Actions logs for the exact post-fix measurements).
