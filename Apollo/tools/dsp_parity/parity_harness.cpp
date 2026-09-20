// EarthPedal / Apollo DSP parity harness (JUCE-free).
//
// Purpose: reproduce the *reverb* and *octave-branch* divergences between the
// Web/WASM port and the Apollo VST port, isolating each suspected cause so the
// perceptual difference can be measured instead of guessed.
//
// It links only the self-contained DSP classes used on the audio path:
//   - Apollo/Source/DSP/Dattorro/{Dattorro, dsp/**}
//   - Apollo/Source/DSP/Util/{Multirate, OctaveGenerator, BandShifter}
//
// Build: see build.ps1 in this folder. Outputs WAVs + CSV into
// Apollo/docs/dsp_parity/renders.

#include "wav_writer.h"

#include "Dattorro.hpp"
#include "Multirate.h"
#include "OctaveGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Reverb configuration
// ---------------------------------------------------------------------------

struct ReverbConfig {
    std::string name;

    // Presence flags mirror the difference between "VST current" (does not call
    // these setters, so the Dattorro defaults leak through) and the original
    // earth.cpp / Web initialisation (calls them explicitly).
    bool tankDiffusionSet = false;
    float tankDiffusion = 0.7f;

    bool inputHighCutSet = false;
    float inputHighCutPitch = 10.0f; // -> 14080 Hz

    bool inputLowCutSet = true;
    float inputLowCutPitch = 0.0f; // -> 13.75 Hz

    bool tankHighCutSet = false;
    float tankHighCutPitch = 10.0f; // -> 14080 Hz

    bool tankLowCutSet = false;
    float tankLowCutPitch = 0.0f; // -> 13.75 Hz

    bool modShapeSet = false;
    float modShape = 0.5f;

    float decay = 0.877465f;
    float modDepthNorm = 0.0625f; // 0.0625 * 8 = 0.5
    float modSpeedNorm = 0.0466f; // 0.3 + 0.0466*15 ~= 1.0
    float timeScale = 4.0f;       // Large
    bool inputDiffusion = true;
    float preDelaySeconds = 0.0f;
};

constexpr float kSampleRate = 48000.0f;

// Renders a stereo impulse response of the reverb, replicating the exact order
// of operations used by earth.cpp (the canonical reference) and by the VST.
static void renderReverbIR(const ReverbConfig& cfg,
                           float seconds,
                           std::vector<float>& left,
                           std::vector<float>& right) {
    const int n = static_cast<int>(seconds * kSampleRate);
    left.assign(n, 0.0f);
    right.assign(n, 0.0f);

    Dattorro reverb(48000, 16, 4.0); // matches both ports
    reverb.setSampleRate(kSampleRate);

    // earth.cpp applies these unconditionally; the VST does not, which is the
    // divergence under test (see ANALYSIS.md hypothesis 1/2).
    reverb.setTimeScale(cfg.timeScale);
    reverb.setPreDelay(cfg.preDelaySeconds);
    if (cfg.inputLowCutSet) reverb.setInputFilterLowCutoffPitch(cfg.inputLowCutPitch);
    if (cfg.inputHighCutSet) reverb.setInputFilterHighCutoffPitch(cfg.inputHighCutPitch);
    reverb.enableInputDiffusion(cfg.inputDiffusion);
    reverb.setDecay(cfg.decay);
    if (cfg.tankDiffusionSet) reverb.setTankDiffusion(cfg.tankDiffusion);
    if (cfg.tankLowCutSet) reverb.setTankFilterLowCutFrequency(cfg.tankLowCutPitch);
    if (cfg.tankHighCutSet) reverb.setTankFilterHighCutFrequency(cfg.tankHighCutPitch);
    reverb.setTankModSpeed(0.3f + cfg.modSpeedNorm * 15.0f);
    reverb.setTankModDepth(cfg.modDepthNorm * 8.0f);
    if (cfg.modShapeSet) reverb.setTankModShape(cfg.modShape);
    reverb.clear();

    for (int i = 0; i < n; ++i) {
        const float in = (i == 0) ? 1.0f : 0.0f;
        reverb.process(in, in);
        left[i] = reverb.getLeftOutput();
        right[i] = reverb.getRightOutput();
    }
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

struct Metrics {
    double peak = 0.0;
    double rms = 0.0;
    double rt60 = 0.0;        // seconds, -5..-35 dB slope
    double tailCrest = 0.0;   // peak/rms of the tail after 100 ms
    double density0100 = 0.0; // detected reflections per second, 0..100 ms
    double density100300 = 0.0;
    double totalEnergyDb = 0.0;
};

static Metrics analyse(const std::vector<float>& x) {
    Metrics m;
    if (x.empty()) return m;

    double energy = 0.0;
    for (float v : x) {
        const double a = std::fabs(v);
        if (a > m.peak) m.peak = a;
        energy += double(v) * double(v);
    }
    m.rms = std::sqrt(energy / x.size());
    m.totalEnergyDb = 10.0 * std::log10(energy + 1e-30);

    // Energy decay curve (Schroeder backwards integration).
    std::vector<double> edc(x.size());
    double acc = 0.0;
    for (int i = static_cast<int>(x.size()) - 1; i >= 0; --i) {
        acc += double(x[i]) * double(x[i]);
        edc[i] = acc;
    }
    const double edc0 = edc[0] + 1e-30;
    const double peakDb = 10.0 * std::log10(edc0);
    int idx5 = -1, idx35 = -1;
    for (int i = 0; i < static_cast<int>(edc.size()); ++i) {
        const double db = 10.0 * std::log10(edc[i] + 1e-30) - peakDb;
        if (idx5 < 0 && db <= -5.0) idx5 = i;
        if (idx35 < 0 && db <= -35.0) { idx35 = i; break; }
    }
    if (idx5 >= 0 && idx35 > idx5) {
        const double slope = (-35.0 - (-5.0)) / ((idx35 - idx5) / double(kSampleRate));
        m.rt60 = -60.0 / slope;
    } else {
        m.rt60 = std::numeric_limits<double>::quiet_NaN();
    }

    // Tail crest factor.
    const int tailStart = std::min<int>(static_cast<int>(0.1 * kSampleRate), static_cast<int>(x.size()) - 1);
    double tailEnergy = 0.0, tailPeak = 0.0;
    for (int i = tailStart; i < static_cast<int>(x.size()); ++i) {
        const double a = std::fabs(x[i]);
        tailPeak = std::max(tailPeak, a);
        tailEnergy += double(x[i]) * double(x[i]);
    }
    const double tailRms = std::sqrt(tailEnergy / std::max(1, static_cast<int>(x.size()) - tailStart));
    m.tailCrest = tailRms > 0 ? tailPeak / tailRms : 0.0;

    // Reflection density: local maxima above (peak - 20 dB), re-armed after a
    // refractory window so a ringing sample is not counted many times.
    auto countPeaks = [&](double t0, double t1, double refractoryMs) {
        const double thr = m.peak * 0.1; // -20 dB
        const int a = static_cast<int>(t0 * kSampleRate);
        const int b = std::min<int>(static_cast<int>(t1 * kSampleRate), static_cast<int>(x.size()));
        int count = 0;
        int last = -1000000;
        const int refractory = static_cast<int>(refractoryMs * 0.001 * kSampleRate);
        for (int i = std::max(1, a); i < b - 1; ++i) {
            if (x[i] > thr && x[i] >= x[i - 1] && x[i] >= x[i + 1] && (i - last) > refractory) {
                ++count;
                last = i;
            }
        }
        return count / std::max(1e-9, (b - a) / double(kSampleRate));
    };
    m.density0100 = countPeaks(0.0, 0.1, 0.5);
    m.density100300 = countPeaks(0.1, 0.3, 0.5);

    return m;
}

// ---------------------------------------------------------------------------
// Shelf filters (RBJ, matching juce::dsp::IIR::Coefficients::makeHigh/LowShelf)
// ---------------------------------------------------------------------------

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;
    float process(float xin) {
        const double x = xin;
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return static_cast<float>(y);
    }
    void reset() { z1 = z2 = 0; }
};

static Biquad makeHighShelf(double sr, double f0, double Q, double gainDb) {
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * f0 / sr;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * Q);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) - (A - 1.0) * cw + beta;
    Biquad q;
    q.b0 = A * ((A + 1.0) + (A - 1.0) * cw + beta) / a0;
    q.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0;
    q.b2 = A * ((A + 1.0) + (A - 1.0) * cw - beta) / a0;
    q.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0;
    q.a2 = ((A + 1.0) - (A - 1.0) * cw - beta) / a0;
    return q;
}

static Biquad makeLowShelf(double sr, double f0, double Q, double gainDb) {
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * f0 / sr;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * Q);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    const double a0 = (A + 1.0) + (A - 1.0) * cw + beta;
    Biquad q;
    q.b0 = A * ((A + 1.0) - (A - 1.0) * cw + beta) / a0;
    q.b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0;
    q.b2 = A * ((A + 1.0) - (A - 1.0) * cw - beta) / a0;
    q.a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0;
    q.a2 = ((A + 1.0) + (A - 1.0) * cw - beta) / a0;
    return q;
}

// Magnitude of a biquad at frequency f.
static double biquadMagDb(const Biquad& q, double sr, double f) {
    const double w = 2.0 * M_PI * f / sr;
    const double cw = std::cos(w), sw = std::sin(w);
    const double cw2 = std::cos(2 * w), sw2 = std::sin(2 * w);
    const double nr = q.b0 + q.b1 * cw + q.b2 * cw2;
    const double ni = -(q.b1 * sw + q.b2 * sw2);
    const double dr = 1.0 + q.a1 * cw + q.a2 * cw2;
    const double di = -(q.a1 * sw + q.a2 * sw2);
    const double num = std::sqrt(nr * nr + ni * ni);
    const double den = std::sqrt(dr * dr + di * di);
    return 20.0 * std::log10((num / den) + 1e-30);
}

// ---------------------------------------------------------------------------
// Octave branch: shelves designed at 8 kHz (VST bug) vs 48 kHz (Web)
// ---------------------------------------------------------------------------

static void runOctaveShelfDemo(const fs::path& outDir) {
    const int n = static_cast<int>(3.0 * kSampleRate);
    std::vector<float> in(n, 0.0f);
    // Log sweep 40 Hz -> 12 kHz, 0.2..2.2 s
    double phase = 0.0;
    const double t0 = 0.2, t1 = 2.2;
    const double f0 = 40.0, f1 = 12000.0;
    for (int i = 0; i < n; ++i) {
        const double t = i / double(kSampleRate);
        if (t < t0 || t > t1) continue;
        const double u = (t - t0) / (t1 - t0);
        const double f = f0 * std::pow(f1 / f0, u);
        phase += 2.0 * M_PI * f / kSampleRate;
        in[i] = 0.3f * static_cast<float>(std::sin(phase));
    }

    auto renderWithShelves = [&](double designSr, std::vector<float>& out, Biquad& hb, Biquad& lb) {
        hb = makeHighShelf(designSr, 140.0, 0.707, -11.0);
        lb = makeLowShelf(designSr, 160.0, 0.707, +5.0);
        hb.reset();
        lb.reset();
        out.assign(n, 0.0f);
        for (int i = 0; i < n; ++i) out[i] = lb.process(hb.process(in[i]));
    };

    std::vector<float> vstOut, webOut;
    Biquad hb1, lb1, hb2, lb2;
    renderWithShelves(48000.0 / 6.0, vstOut, hb1, lb1); // VST: 8 kHz design, 48 kHz signal
    renderWithShelves(48000.0, webOut, hb2, lb2);       // Web: 48 kHz design, 48 kHz signal

    parity::writeWav32f((outDir / "octave_shelf_vst8k.wav").string(), {vstOut}, 48000);
    parity::writeWav32f((outDir / "octave_shelf_web48k.wav").string(), {webOut}, 48000);

    std::ofstream csv((outDir / "shelf_response.csv").string());
    csv << "freq_hz,vst_design8k_db,web_design48k_db\n";
    for (double f = 20.0; f <= 20000.0; f *= 1.05) {
        Biquad hA = makeHighShelf(48000.0 / 6.0, 140.0, 0.707, -11.0);
        Biquad lA = makeLowShelf(48000.0 / 6.0, 160.0, 0.707, +5.0);
        Biquad hB = makeHighShelf(48000.0, 140.0, 0.707, -11.0);
        Biquad lB = makeLowShelf(48000.0, 160.0, 0.707, +5.0);
        csv << f << "," << (biquadMagDb(hA, 48000.0, f) + biquadMagDb(lA, 48000.0, f)) << ","
            << (biquadMagDb(hB, 48000.0, f) + biquadMagDb(lB, 48000.0, f)) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Octave branch dry-routing matrix (hypothesis 5)
//
// Reproduces the Web/hardware branch at 48 kHz:
//   decimate -> OctaveGenerator -> interpolate -> shelves -> [+0.5*dry?]
// and renders Up / Down / Both with and without the inner dry, so the meaning
// of `octave_dry_mix` can be pinned down before it is frozen.
// ---------------------------------------------------------------------------

static void runOctaveRoutingMatrix(const fs::path& outDir) {
    const float sr = 48000.0f;
    const int n = static_cast<int>(2.0 * sr);
    std::vector<float> input(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const double t = i / double(sr);
        const double env = std::exp(-t * 1.2) * (1.0 - std::exp(-t * 200.0));
        const double s = std::sin(2 * M_PI * 220.0 * t) + 0.7 * std::sin(2 * M_PI * 330.0 * t) +
                         0.5 * std::sin(2 * M_PI * 440.0 * t);
        input[i] = static_cast<float>(0.25 * env * s);
    }
    parity::writeWav32f((outDir / "octave_input.wav").string(), {input}, 48000);

    std::ofstream csv((outDir / "octave_routing.csv").string());
    csv << "mode,include_dry,rms,temporal_centroid_s,corr_with_dry\n";

    // Canonical modes (Off is a bypass and needs no render).
    const std::array<std::pair<const char*, int>, 3> modes = {
        {{"up", 1}, {"down", 2}, {"both", 3}}};

    for (const auto& [modeName, mode] : modes) {
        for (int includeDry = 0; includeDry <= 1; ++includeDry) {
            Decimator2 dec;
            Interpolator interp;
            OctaveGenerator oct(sr / resample_factor);
            Biquad hb = makeHighShelf(sr, 140.0, 0.707, -11.0);
            Biquad lb = makeLowShelf(sr, 160.0, 0.707, +5.0);

            std::array<float, resample_factor> buff{};
            std::array<float, resample_factor> buffOut{};
            int bin = 0;
            std::vector<float> out(n, 0.0f);

            for (int i = 0; i < n; ++i) {
                buff[bin] = input[i];
                if (bin > 4) {
                    std::span<const float, resample_factor> chunk(&buff[0], resample_factor);
                    const float sample = dec(chunk);
                    oct.update(sample);
                    float mix = 0.0f;
                    if (mode == 1 || mode == 3) mix += oct.up1() * 2.0f;
                    if (mode == 2 || mode == 3) {
                        mix += oct.down1() * 2.0f;
                        mix += oct.down2() * 2.0f;
                    }
                    const auto outChunk = interp(mix);
                    for (size_t j = 0; j < outChunk.size(); ++j) {
                        float m = lb.process(hb.process(outChunk[j]));
                        if (includeDry) m += 0.5f * buff[j];
                        buffOut[j] = m;
                    }
                }
                bin += 1;
                if (bin > 5) bin = 0;
                out[i] = buffOut[bin];
            }

            const std::string name = std::string("octave_") + modeName + (includeDry ? "_dry" : "_nodry");
            parity::writeWav32f((outDir / (name + ".wav")).string(), {out}, 48000);

            // Metrics: rms, naive spectral centroid, correlation with the dry input.
            double energy = 0.0, num = 0.0, den = 0.0;
            for (int i = 0; i < n; ++i) {
                energy += double(out[i]) * double(out[i]);
                num += double(std::fabs(out[i])) * (i / double(sr));
                den += std::fabs(out[i]);
            }
            const double rms = std::sqrt(energy / n);
            double sa = 0, sb = 0, sab = 0;
            for (int i = 0; i < n; ++i) {
                sa += double(out[i]) * out[i];
                sb += double(input[i]) * input[i];
                sab += double(out[i]) * input[i];
            }
            const double corr = sab / (std::sqrt(sa * sb) + 1e-30);
            csv << modeName << "," << includeDry << "," << rms << ","
                << (den > 1e-9 ? num / den : 0.0) << "," << corr << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    fs::path outDir = (argc > 1) ? fs::path(argv[1]) : fs::path(".");
    fs::create_directories(outDir);

    std::vector<ReverbConfig> configs;

    // Baseline: exactly what the Apollo VST does today for the reverb
    // (no setTankDiffusion / no tank filter init / no explicit mod shape).
    ReverbConfig current;
    current.name = "vst_current";
    configs.push_back(current);

    // Stage A: + original tank diffusion.
    ReverbConfig a = current;
    a.name = "vst_A_tank_diffusion";
    a.tankDiffusionSet = true;
    configs.push_back(a);

    // Stage B: A + complete original Dattorro initialisation (input/tank cuts).
    ReverbConfig b = a;
    b.name = "vst_B_tank_init";
    b.inputHighCutSet = true;
    b.inputHighCutPitch = 10.0f;
    b.tankHighCutSet = true;
    b.tankHighCutPitch = 10.0f;
    b.tankLowCutSet = true;
    b.tankLowCutPitch = 0.0f;
    configs.push_back(b);

    // Stage C: B + explicit mod shape (no-op today: default is already 0.5).
    ReverbConfig c = b;
    c.name = "vst_C_modshape";
    c.modShapeSet = true;
    c.modShape = 0.5f;
    configs.push_back(c);

    // Full canonical earth.cpp / Web init.
    ReverbConfig canon = b;
    canon.name = "web_reference_default";
    configs.push_back(canon);

    // Tank-diffusion sweep to quantify density (hypothesis 1).
    for (float d : {0.0f, 0.7f}) {
        ReverbConfig s = b;
        s.name = std::string("diffusion_") + (d == 0.0f ? "0.0" : "0.7");
        s.tankDiffusionSet = true;
        s.tankDiffusion = d;
        configs.push_back(s);
    }

    // Decay sweep (isolating reverb tail behaviour).
    for (float dec : {0.25f, 0.5f, 0.877465f, 1.0f}) {
        ReverbConfig s = b;
        s.name = "decay_" + std::to_string(dec);
        s.decay = dec;
        configs.push_back(s);
    }

    std::ofstream metrics(outDir / "metrics.csv");
    metrics << "name,rt60_s,peak,rms,tail_crest,density_0_100ms,density_100_300ms,total_energy_db\n";

    for (const auto& cfg : configs) {
        std::vector<float> l, r;
        renderReverbIR(cfg, 4.0f, l, r);
        parity::writeWav32f((outDir / (cfg.name + "_L.wav")).string(), {l}, 48000);
        parity::writeWav32f((outDir / (cfg.name + "_stereo.wav")).string(), {l, r}, 48000);
        const Metrics m = analyse(l);
        metrics << cfg.name << "," << m.rt60 << "," << m.peak << "," << m.rms << ","
                << m.tailCrest << "," << m.density0100 << "," << m.density100300 << ","
                << m.totalEnergyDb << "\n";
        std::printf("%-24s RT60=%7.3fs  crest=%6.2f  density[0-100ms]=%8.1f/s  energy=%.2f dB\n",
                    cfg.name.c_str(), m.rt60, m.tailCrest, m.density0100, m.totalEnergyDb);
    }

    runOctaveRoutingMatrix(outDir);
    runOctaveShelfDemo(outDir);

    std::printf("\nWrote renders + CSVs to %s\n", outDir.string().c_str());
    return 0;
}
