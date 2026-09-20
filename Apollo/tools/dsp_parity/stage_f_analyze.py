#!/usr/bin/env python3
"""Stage F timebase forensics: plots + null report.

Usage: python stage_f_analyze.py <renders_dir> <plots_dir>
"""
import sys
import os
import csv
import numpy as np


def read_wav32(path):
    with open(path, "rb") as f:
        data = f.read()
    pos = 12
    fmt = None
    body = b""
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = int.from_bytes(data[pos + 4:pos + 8], "little")
        chunk = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = chunk
        elif cid == b"data":
            body = chunk
            break
        pos += 8 + size + (size & 1)
    channels = int.from_bytes(fmt[2:4], "little")
    sr = int.from_bytes(fmt[4:8], "little")
    x = np.frombuffer(body, dtype="<f4").astype(np.float64).reshape(-1, channels)
    return x, sr


def edc_db(x):
    e = np.cumsum(x[::-1] ** 2)[::-1]
    e = np.maximum(e, 1e-30)
    return 10.0 * np.log10(e / e[0])


def mag_db(x, sr):
    n = 1 << int(np.ceil(np.log2(max(1024, len(x)))))
    X = np.fft.rfft(x * np.hanning(len(x)), n=n)
    return np.fft.rfftfreq(n, 1.0 / sr), 20.0 * np.log10(np.abs(X) + 1e-12)


def null_metrics(a, b):
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    best = None
    for lag in range(-256, 257):
        if lag >= 0:
            aa, bb = a[lag:], b[:n - lag]
        else:
            aa, bb = a[:n + lag], b[-lag:]
        rms = float(np.sqrt(np.mean((aa - bb) ** 2)))
        if best is None or rms < best[0]:
            best = (rms, lag)
    rms, lag = best
    if lag >= 0:
        aa, bb = a[lag:], b[:n - lag]
    else:
        aa, bb = a[:n + lag], b[-lag:]
    d = aa - bb
    ref = float(np.sqrt(np.mean(a ** 2))) + 1e-30
    return dict(lag=lag, null_db=20.0 * np.log10((rms + 1e-30) / ref),
                corr=float(np.corrcoef(aa, bb)[0, 1]) if len(aa) > 1 else 0.0)


def load_csv(path):
    with open(path) as f:
        return list(csv.DictReader(f))


def main():
    renders = sys.argv[1] if len(sys.argv) > 1 else "."
    plots = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(plots, exist_ok=True)
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    models = ["legacy32k", "correct", "invariant"]
    colors = {"legacy32k": "tab:red", "correct": "tab:blue", "invariant": "tab:green"}
    hosts = [32000, 44100, 48000, 88200, 96000, 192000]

    delays = load_csv(os.path.join(renders, "internal_delays.csv"))
    lfo = load_csv(os.path.join(renders, "lfo_audit.csv"))
    metrics = load_csv(os.path.join(renders, "metrics.csv"))

    def dval(rows, **kw):
        for r in rows:
            if all(str(r[k]) == str(v) for k, v in kw.items()):
                return r
        return None

    # 1. delay time vs host
    plt.figure(figsize=(10, 4.5))
    for m in models:
        ms = [float(dval(delays, model=m, host=h, size=2, kind="delay", name="leftDelay1")["ms"]) for h in hosts]
        plt.plot(hosts, ms, "o-", color=colors[m], label=m)
    plt.axhline(399.0, color="gray", ls="--", lw=0.8)
    plt.title("Tank delay time vs host rate (leftDelay1, Size Large)")
    plt.xlabel("host sample rate (Hz)")
    plt.ylabel("delay (ms)")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "delay_time_vs_host.png"), dpi=120)
    plt.close()

    # 2. LFO frequency vs host (lfo1 default)
    plt.figure(figsize=(10, 4.5))
    for m in models:
        hz = [float(dval(lfo, model=m, host=h, lfo="lfo1_default")["measured_hz"]) for h in [44100, 48000, 96000, 192000]]
        plt.plot([44100, 48000, 96000, 192000], hz, "o-", color=colors[m], label=m)
    plt.axhline(0.0999, color="gray", ls="--", lw=0.8, label="target 0.0999 Hz")
    plt.title("LFO1 real frequency vs host rate (default mod speed)")
    plt.xlabel("host sample rate (Hz)")
    plt.ylabel("Hz")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "lfo_freq_vs_host.png"), dpi=120)
    plt.close()

    # 3. RT30 vs host (Large, decay 0.877465)
    plt.figure(figsize=(10, 4.5))
    for m in models:
        rt = [float(dval(metrics, model=m, host=h, size=2, decay=0.877465)["rt30"]) for h in hosts]
        plt.plot(hosts, rt, "o-", color=colors[m], label=m)
    plt.title("RT30 vs host rate (Size Large, decay 0.877465)")
    plt.xlabel("host sample rate (Hz)")
    plt.ylabel("RT30 (s)")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "rt30_vs_host.png"), dpi=120)
    plt.close()

    # 4. RT30 decay matrix at 44.1/48/96
    fig, axes = plt.subplots(1, 3, figsize=(13, 4), sharey=True)
    decays = [0.5, 0.877465, 0.95]
    for ax, m in zip(axes, models):
        for d in decays:
            rt = [float(dval(metrics, model=m, host=h, size=2, decay=d)["rt30"]) for h in [44100, 48000, 96000]]
            ax.plot([44100, 48000, 96000], rt, "o-", label="decay %.3f" % d)
        ax.set_title(m)
        ax.set_xlabel("host (Hz)")
    axes[0].set_ylabel("RT30 (s)")
    axes[0].legend(fontsize=8)
    fig.suptitle("RT30 vs host and decay (Size Large)")
    fig.tight_layout()
    plt.savefig(os.path.join(plots, "rt30_decay_matrix.png"), dpi=120)
    plt.close()

    # 5. density vs host
    plt.figure(figsize=(10, 4.5))
    for m in models:
        de = [float(dval(metrics, model=m, host=h, size=2, decay=0.877465)["density_100_500"]) for h in hosts]
        plt.plot(hosts, de, "o-", color=colors[m], label=m)
    plt.title("Reflection density 100-500 ms vs host (Size Large)")
    plt.xlabel("host (Hz)")
    plt.ylabel("reflections/s")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "density_vs_host.png"), dpi=120)
    plt.close()

    # WAV-based plots
    def w(name):
        x, sr = read_wav32(os.path.join(renders, name))
        return x[:, 0], sr

    # 6. EDC overlays at 48k and 96k
    for h in [48000, 96000]:
        plt.figure(figsize=(10, 4.5))
        for m in models:
            x, sr = w("%s_%d_ir.wav" % (m, h))
            e = edc_db(x)
            t = np.arange(len(e)) / sr
            idx = np.argmax(e < -1.0)
            plt.plot(t[idx:], e[idx:], color=colors[m], label=m)
        plt.ylim(-70, 0)
        plt.xlim(0, 8)
        plt.title("Energy decay curve @ %d Hz (Size Large)" % h)
        plt.xlabel("s")
        plt.ylabel("dB")
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(plots, "edc_%d.png" % h), dpi=120)
        plt.close()

    # 7. early reflection zoom @48k
    plt.figure(figsize=(10, 4.5))
    for m in models:
        x, sr = w("%s_48000_ir.wav" % m)
        n = int(0.06 * sr)
        plt.plot(np.arange(n) / sr * 1000, x[:n], color=colors[m], label=m, alpha=0.8)
    plt.title("Early reflections zoom @ 48 kHz (first 60 ms)")
    plt.xlabel("ms")
    plt.ylabel("amplitude")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "ir_early_zoom_48k.png"), dpi=120)
    plt.close()

    # 8. magnitude spectra @48k
    plt.figure(figsize=(10, 4.5))
    for m in models:
        x, sr = w("%s_48000_ir.wav" % m)
        f, mag = mag_db(x, sr)
        plt.semilogx(f[1:], mag[1:], color=colors[m], label=m, alpha=0.8)
    plt.xlim(20, 20000)
    plt.title("IR magnitude spectrum @ 48 kHz")
    plt.xlabel("Hz")
    plt.ylabel("dB")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "magnitude_spectra_48k.png"), dpi=120)
    plt.close()

    # 9. spectrograms @48k
    fig, axes = plt.subplots(3, 1, figsize=(10, 7), sharex=True)
    for ax, m in zip(axes, models):
        x, sr = w("%s_48000_ir.wav" % m)
        ax.specgram(x[:int(2.0 * sr)], NFFT=1024, Fs=sr, noverlap=512, cmap="magma", vmin=-130)
        ax.set_title(m)
        ax.set_ylabel("Hz")
    axes[-1].set_xlabel("s")
    fig.suptitle("IR spectrogram @ 48 kHz")
    fig.tight_layout()
    plt.savefig(os.path.join(plots, "spectrogram_48k.png"), dpi=120)
    plt.close()

    # 10. musical waveform overlay @96k
    plt.figure(figsize=(10, 4.5))
    for m in models:
        x, sr = w("%s_96000_music_wet.wav" % m)
        t = np.arange(len(x)) / sr
        plt.plot(t[:int(1.5 * sr)], x[:int(1.5 * sr)], color=colors[m], label=m, alpha=0.7)
    plt.title("100% wet musical render @ 96 kHz (first 1.5 s)")
    plt.xlabel("s")
    plt.ylabel("amplitude")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(plots, "waveform_overlay_96k.png"), dpi=120)
    plt.close()

    # 11. LFO waveforms
    for h in [48000, 96000]:
        plt.figure(figsize=(10, 4.5))
        for m in models:
            x, sr = w("lfo1_%s_%d.wav" % (m, h))
            t = np.arange(len(x)) / sr
            plt.plot(t, x, color=colors[m], label="%s (%.4f Hz)" % (m, float(dval(
                lfo, model=m, host=h, lfo="lfo1_default")["measured_hz"])), alpha=0.8)
        plt.xlim(0, 30)
        plt.title("LFO1 waveform @ %d Hz host (30 s)" % h)
        plt.xlabel("s")
        plt.ylabel("output")
        plt.legend(fontsize=8)
        plt.tight_layout()
        plt.savefig(os.path.join(plots, "lfo_waveform_%d.png" % h), dpi=120)
        plt.close()

    # 12. null / correlation report
    report = {}
    report["invariant_48k_vs_legacy_48k"] = null_metrics(w("invariant_48000_ir.wav")[0], w("legacy32k_48000_ir.wav")[0])
    report["invariant_96k_vs_legacy_96k"] = null_metrics(w("invariant_96000_ir.wav")[0], w("legacy32k_96000_ir.wav")[0])
    report["correct_48k_vs_legacy_48k"] = null_metrics(w("correct_48000_ir.wav")[0], w("legacy32k_48000_ir.wav")[0])
    report["invariant_44k1_vs_legacy_48k"] = null_metrics(w("invariant_44100_ir.wav")[0], w("legacy32k_48000_ir.wav")[0])
    with open(os.path.join(renders, "NULL_REPORT.txt"), "w") as fh:
        for k, v in report.items():
            fh.write("%s\n" % k)
            for kk, vv in v.items():
                fh.write("  %-10s %s\n" % (kk, vv))
            fh.write("\n")
    for k, v in report.items():
        print("%-30s null=%7.1f dB  lag=%d  corr=%.5f" % (k, v["null_db"], v["lag"], v["corr"]))


if __name__ == "__main__":
    main()
