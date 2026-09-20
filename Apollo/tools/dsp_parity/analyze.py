#!/usr/bin/env python3
"""Post-process the DSP parity renders.

Reads the 32-bit float WAVs produced by parity_harness.cpp and writes
comparative plots + a null-test report.

Usage:  python analyze.py <renders_dir> <plots_dir>
"""
import sys
import os
import csv
import numpy as np


def read_wav32(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a WAV: " + path)
    pos = 12
    fmt = None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = int.from_bytes(data[pos + 4:pos + 8], "little")
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = body
        elif cid == b"data":
            break
        pos += 8 + size + (size & 1)
    if fmt is None:
        raise ValueError("no fmt chunk")
    audio_format = int.from_bytes(fmt[0:2], "little")
    channels = int.from_bytes(fmt[2:4], "little")
    sample_rate = int.from_bytes(fmt[4:8], "little")
    bits = int.from_bytes(fmt[14:16], "little")
    if audio_format != 3 or bits != 32:
        raise ValueError("expected 32-bit float WAV, got fmt=%d bits=%d" % (audio_format, bits))
    x = np.frombuffer(body, dtype="<f4").astype(np.float64)
    x = x.reshape(-1, channels)
    return x, sample_rate


def edc_db(x):
    e = np.cumsum(x[::-1] ** 2)[::-1]
    e = np.maximum(e, 1e-30)
    return 10.0 * np.log10(e / e[0])


def magnitude_db(x, sr):
    n = 1 << int(np.ceil(np.log2(max(1024, len(x)))))
    X = np.fft.rfft(x * np.hanning(len(x)), n=n)
    f = np.fft.rfftfreq(n, 1.0 / sr)
    mag = 20.0 * np.log10(np.abs(X) + 1e-12)
    return f, mag


def null_metrics(a, b):
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    # best integer lag over +/- 64 samples
    best = None
    for lag in range(-64, 65):
        if lag >= 0:
            aa, bb = a[lag:], b[:n - lag]
        else:
            aa, bb = a[:n + lag], b[-lag:]
        d = aa - bb
        rms = float(np.sqrt(np.mean(d ** 2)))
        if best is None or rms < best[0]:
            best = (rms, lag)
    rms, lag = best
    if lag >= 0:
        aa, bb = a[lag:], b[:n - lag]
    else:
        aa, bb = a[:n + lag], b[-lag:]
    d = aa - bb
    ref = float(np.sqrt(np.mean(a ** 2))) + 1e-30
    depth_db = 20.0 * np.log10((rms + 1e-30) / ref)
    corr = float(np.corrcoef(aa, bb)[0, 1]) if len(aa) > 1 else 0.0
    return dict(lag=lag, null_depth_db=depth_db, diff_rms=rms,
                peak_diff=float(np.max(np.abs(d))), corr=corr)


def main():
    renders = sys.argv[1] if len(sys.argv) > 1 else "."
    plots = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(plots, exist_ok=True)

    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    def load(name):
        x, sr = read_wav32(os.path.join(renders, name))
        return x[:, 0], sr

    current, sr = load("vst_current_L.wav")
    a, _ = load("vst_A_tank_diffusion_L.wav")
    b, _ = load("vst_B_tank_init_L.wav")
    web, _ = load("web_reference_default_L.wav")
    d0, _ = load("diffusion_0.0_L.wav")
    d7, _ = load("diffusion_0.7_L.wav")
    t = np.arange(len(current)) / sr

    # 1. impulse response overlay (first 150 ms)
    m = int(0.15 * sr)
    plt.figure(figsize=(11, 4))
    plt.plot(t[:m] * 1000, current[:m], label="VST current (no tank diffusion)", alpha=0.8)
    plt.plot(t[:m] * 1000, a[:m], label="VST + tank diffusion 0.7", alpha=0.8)
    plt.title("Impulse response, first 150 ms")
    plt.xlabel("ms")
    plt.ylabel("amplitude")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "ir_overlay_150ms.png"), dpi=120)
    plt.close()

    # 2. energy decay curve
    plt.figure(figsize=(11, 4))
    for x, label in [(current, "VST current"), (a, "VST + tank diffusion"), (b, "VST full init / Web ref")]:
        e = edc_db(x)
        # start where the EDC has fallen 1 dB (skip direct impulse)
        idx = np.argmax(e < -1.0)
        plt.plot(t[idx:] , e[idx:], label=label)
    plt.ylim(-70, 0)
    plt.xlim(0, 3)
    plt.title("Energy decay curve (Schroeder)")
    plt.xlabel("s")
    plt.ylabel("dB")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "energy_decay_curve.png"), dpi=120)
    plt.close()

    # 3. magnitude spectrum
    plt.figure(figsize=(11, 4))
    for x, label in [(current, "VST current"), (a, "VST + tank diffusion")]:
        f, mag = magnitude_db(x, sr)
        plt.semilogx(f[1:], mag[1:], label=label, alpha=0.8)
    plt.xlim(20, 20000)
    plt.title("Impulse response magnitude spectrum")
    plt.xlabel("Hz")
    plt.ylabel("dB")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "magnitude_spectrum.png"), dpi=120)
    plt.close()

    # 4. spectrogram (tank diffusion off vs on)
    fig, axes = plt.subplots(2, 1, figsize=(11, 6), sharex=True)
    for ax, x, label in [(axes[0], current, "VST current"), (axes[1], a, "VST + tank diffusion 0.7")]:
        ax.specgram(x[:int(2.0 * sr)], NFFT=1024, Fs=sr, noverlap=512, cmap="magma", vmin=-120)
        ax.set_title(label)
        ax.set_ylabel("Hz")
    axes[1].set_xlabel("s")
    fig.suptitle("Spectrogram comparison")
    fig.tight_layout()
    plt.savefig(os.path.join(plots, "spectrogram.png"), dpi=120)
    plt.close()

    # 5. null comparison, tank diffusion 0 vs 0.7 (same length, same latency)
    n = min(len(d0), len(d7))
    plt.figure(figsize=(11, 4))
    plt.plot(t[:int(0.05 * sr)] * 1000, (d0 - d7)[:int(0.05 * sr)], label="diff 0.0 - diff 0.7")
    plt.title("Null difference, tank diffusion 0.0 vs 0.7 (first 50 ms)")
    plt.xlabel("ms")
    plt.ylabel("amplitude")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "null_diffusion.png"), dpi=120)
    plt.close()

    # 6. shelf response
    shelf_path = os.path.join(renders, "shelf_response.csv")
    if os.path.exists(shelf_path):
        fr, vst_db, web_db = [], [], []
        with open(shelf_path) as fh:
            rd = csv.DictReader(fh)
            for row in rd:
                fr.append(float(row["freq_hz"]))
                vst_db.append(float(row["vst_design8k_db"]))
                web_db.append(float(row["web_design48k_db"]))
        fr = np.array(fr); vst_db = np.array(vst_db); web_db = np.array(web_db)
        plt.figure(figsize=(11, 4))
        plt.semilogx(fr, vst_db, label="VST: shelves designed @ 8 kHz, applied @ 48 kHz")
        plt.semilogx(fr, web_db, label="Web: shelves designed @ 48 kHz, applied @ 48 kHz")
        plt.xlim(20, 20000)
        plt.axvline(140, color="gray", ls="--", lw=0.8)
        plt.axvline(160, color="gray", ls=":", lw=0.8)
        plt.title("Octave shelf response: EQ1 -11 dB @140 Hz, EQ2 +5 dB @160 Hz")
        plt.xlabel("Hz")
        plt.ylabel("dB")
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(plots, "shelf_response.png"), dpi=120)
        plt.close()

    # null report
    report = {
        "current_vs_A_tank_diffusion": null_metrics(current, a),
        "A_vs_B_full_init": null_metrics(a, b),
        "B_vs_web_reference": null_metrics(b, web),
        "diffusion_0.0_vs_0.7": null_metrics(d0, d7),
    }
    with open(os.path.join(renders, "NULL_REPORT.txt"), "w") as fh:
        for k, v in report.items():
            fh.write("%s\n" % k)
            for kk, vv in v.items():
                fh.write("  %-14s %s\n" % (kk, vv))
            fh.write("\n")
    for k, v in report.items():
        print(k, "-> null depth %.1f dB, lag %d, corr %.4f"
              % (v["null_depth_db"], v["lag"], v["corr"]))


if __name__ == "__main__":
    main()
