// Stage F — Tank Timebase Forensics (JUCE-free harness).
//
// Compares three interpretations of the Dattorro tank sample rate:
//
//   LEGACY_32K             tank internal rate = min(host, 32000)   (current)
//   DATTORRO_CORRECT       tank internal rate = host
//   LEGACY_SOUND_SR_INVARIANT
//                          tank internal rate = host * 2/3, so real times
//                          match LEGACY_32K at 48 kHz at every host rate.
//
// Production is NOT modified: the models are built by overriding public tank
// members (all of Dattorro1997Tank's fields are public) before setSampleRate().
//
// Outputs CSVs + WAVs into the directory given as argv[1].

#include "wav_writer.h"

#include "Dattorro.hpp"
#include "dsp/modulation/LFO.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class Model { Legacy32k = 0, Correct = 1, SrInvariant = 2 };
static const char* modelName(Model m) {
    switch (m) {
        case Model::Legacy32k: return "legacy32k";
        case Model::Correct: return "correct";
        case Model::SrInvariant: return "invariant";
    }
    return "?";
}

static constexpr float kDattorroRate = 29761.0f;

struct ReverbConfig {
    bool inputDiffusion = true;
    float preDelay = 0.0f;
};

// ---------------------------------------------------------------------------
// Timebase configuration (experimental)
// ---------------------------------------------------------------------------

static void configureTankTimebase(Dattorro& rev, Model m, float host) {
    float maxRate = host;
    switch (m) {
        case Model::Legacy32k: maxRate = 32000.0f; break;
        case Model::Correct: maxRate = host; break;
        case Model::SrInvariant: maxRate = host * (2.0f / 3.0f); break;
    }
    rev.tank.maxSampleRate = maxRate;
    rev.setSampleRate(host);

    if (m != Model::Legacy32k) {
        const float eff = rev.tank.sampleRate;
        rev.tank.leftHighCutFilter.setSampleRate(eff);
        rev.tank.rightHighCutFilter.setSampleRate(eff);
        rev.tank.leftLowCutFilter.setSampleRate(eff);
        rev.tank.rightLowCutFilter.setSampleRate(eff);
        rev.tank.lfo1.setSamplerate(eff);
        rev.tank.lfo2.setSamplerate(eff);
        rev.tank.lfo3.setSamplerate(eff);
        rev.tank.lfo4.setSamplerate(eff);
    }
}

static float sizeToTimeScale(int sizeIdx) {
    return sizeIdx == 0 ? 1.0f : (sizeIdx == 2 ? 4.0f : 2.0f);
}

static std::unique_ptr<Dattorro> makeReverb(Model m, float host, int sizeIdx,
                                            float decay, const ReverbConfig& rc) {
    auto rev = std::make_unique<Dattorro>(48000, 16, 4.0);
    configureTankTimebase(*rev, m, host);
    rev->setTimeScale(sizeToTimeScale(sizeIdx));
    rev->setPreDelay(rc.preDelay);
    rev->setInputFilterLowCutoffPitch(0.0f);   // 13.75 Hz
    rev->setInputFilterHighCutoffPitch(10.0f); // 14080 Hz
    rev->enableInputDiffusion(rc.inputDiffusion);
    rev->setDecay(decay);
    rev->setTankDiffusion(0.7f);
    rev->setTankFilterLowCutFrequency(0.0f);
    rev->setTankFilterHighCutFrequency(10.0f);
    rev->setTankModSpeed(0.3f + 0.0466f * 15.0f); // ~1.0
    rev->setTankModDepth(0.0625f * 8.0f);         // 0.5
    rev->setTankModShape(0.5f);
    rev->clear();
    return rev;
}

static void processBuffer(Dattorro& rev, const std::vector<float>& in,
                          std::vector<float>& L, std::vector<float>& R) {
    L.assign(in.size(), 0.0f);
    R.assign(in.size(), 0.0f);
    for (size_t i = 0; i < in.size(); ++i) {
        rev.process(in[i], in[i]);
        L[i] = rev.getLeftOutput();
        R[i] = rev.getRightOutput();
    }
}

static std::vector<float> impulse(int n) {
    std::vector<float> x(n, 0.0f);
    if (n > 0) x[0] = 1.0f;
    return x;
}

static std::vector<float> musical(float host, float seconds) {
    const int n = static_cast<int>(seconds * host);
    std::vector<float> x(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const double t = i / double(host);
        const double env = std::exp(-t * 1.1) * (1.0 - std::exp(-t * 120.0));
        const double s = std::sin(2 * M_PI * 196.0 * t) + 0.8 * std::sin(2 * M_PI * 246.94 * t) +
                         0.6 * std::sin(2 * M_PI * 293.66 * t) + 0.4 * std::sin(2 * M_PI * 392.0 * t);
        x[i] = static_cast<float>(0.2 * env * s);
    }
    return x;
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

struct Metrics {
    double onsetMs = 0.0;
    double density0_50 = 0.0;
    double density0_100 = 0.0;
    double density100_500 = 0.0;
    double crest = 0.0;
    double rmsTail = 0.0;
    double peak = 0.0;
    double rt20 = 0.0;
    double rt30 = 0.0;
    double rt60 = 0.0;
    double centroidHz = 0.0;
    double energyDb = 0.0;
};

// Iterative radix-2 FFT (in-place, complex).
static void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2.0 * M_PI / double(len);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k) {
                const std::complex<double> u = a[i + k];
                const std::complex<double> v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

static double spectralCentroid(const std::vector<float>& x, double host,
                               double t0, double t1) {
    int a = std::max(0, static_cast<int>(t0 * host));
    int b = std::min<int>(static_cast<int>(t1 * host), static_cast<int>(x.size()));
    if (b - a < 256) return 0.0;
    const int N = 4096;
    if (b - a > N) b = a + N;
    std::vector<std::complex<double>> buf(1 << 14, {0.0, 0.0});
    for (int i = 0; i < b - a; ++i) {
        const double w = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (b - a - 1));
        buf[i] = x[a + i] * w;
    }
    fft(buf);
    const double binHz = host / buf.size();
    double num = 0.0, den = 0.0;
    for (size_t k = 1; k < buf.size() / 2; ++k) {
        const double m = std::abs(buf[k]);
        num += (k * binHz) * m;
        den += m;
    }
    return den > 1e-12 ? num / den : 0.0;
}

static Metrics analyse(const std::vector<float>& x, double host) {
    Metrics m;
    if (x.empty()) return m;

    double energy = 0.0;
    for (float v : x) {
        const double a = std::fabs(v);
        m.peak = std::max(m.peak, a);
        energy += double(v) * double(v);
    }
    m.energyDb = 10.0 * std::log10(energy + 1e-30);

    // Onset: first sample above a threshold relative to peak.
    const double thr = m.peak * 1e-4;
    for (size_t i = 0; i < x.size(); ++i) {
        if (std::fabs(x[i]) > thr) { m.onsetMs = i / host * 1000.0; break; }
    }

    // Schroeder EDC.
    std::vector<double> edc(x.size());
    double acc = 0.0;
    for (int i = static_cast<int>(x.size()) - 1; i >= 0; --i) {
        acc += double(x[i]) * double(x[i]);
        edc[i] = acc;
    }
    const double peakDb = 10.0 * std::log10(edc[0] + 1e-30);
    auto idxAt = [&](double dbTarget) {
        for (int i = 0; i < static_cast<int>(edc.size()); ++i)
            if (10.0 * std::log10(edc[i] + 1e-30) - peakDb <= dbTarget) return i;
        return -1;
    };
    const int i5 = idxAt(-5.0), i25 = idxAt(-25.0), i35 = idxAt(-35.0);
    if (i5 >= 0 && i25 > i5) m.rt20 = 60.0 * (i25 - i5) / host / 20.0;
    if (i5 >= 0 && i35 > i5) m.rt30 = 60.0 * (i35 - i5) / host / 30.0;
    if (i5 >= 0 && i35 > i5) m.rt60 = 60.0 * (i35 - i5) / host / 30.0; // estimate

    // Tail crest / rms after 100 ms.
    const int tailStart = std::min<int>(static_cast<int>(0.1 * host), static_cast<int>(x.size()) - 1);
    double te = 0.0, tp = 0.0;
    for (int i = tailStart; i < static_cast<int>(x.size()); ++i) {
        te += double(x[i]) * double(x[i]);
        tp = std::max(tp, double(std::fabs(x[i])));
    }
    const int tailN = std::max(1, static_cast<int>(x.size()) - tailStart);
    m.rmsTail = std::sqrt(te / tailN);
    m.crest = m.rmsTail > 0 ? tp / m.rmsTail : 0.0;

    auto countPeaks = [&](double a, double b, double refractoryMs) {
        const double thrPeak = m.peak * 0.1;
        const int ia = static_cast<int>(a * host);
        const int ib = std::min<int>(static_cast<int>(b * host), static_cast<int>(x.size()));
        const int refr = static_cast<int>(refractoryMs * 0.001 * host);
        int count = 0, last = -1000000;
        for (int i = std::max(1, ia); i < ib - 1; ++i) {
            if (x[i] > thrPeak && x[i] >= x[i - 1] && x[i] >= x[i + 1] && (i - last) > refr) {
                ++count;
                last = i;
            }
        }
        return count / std::max(1e-9, (ib - ia) / host);
    };
    m.density0_50 = countPeaks(0.0, 0.05, 0.3);
    m.density0_100 = countPeaks(0.0, 0.1, 0.5);
    m.density100_500 = countPeaks(0.1, 0.5, 0.5);

    m.centroidHz = spectralCentroid(x, host, 0.15, 0.9);
    return m;
}

// ---------------------------------------------------------------------------
// Internal delay / tap tables
// ---------------------------------------------------------------------------

static void dumpDelays(std::ofstream& csv, Model m, float host, int sizeIdx) {
    const ReverbConfig rc;
    auto rev = makeReverb(m, host, sizeIdx, 0.877465f, rc);
    const float proc = host;
    auto emit = [&](const char* kind, const char* name, float samples) {
        csv << modelName(m) << "," << host << "," << sizeIdx << "," << kind << "," << name
            << "," << samples << "," << (samples / proc * 1000.0f) << "\n";
    };
    emit("delay", "leftApf1", rev->tank.scaledLeftApf1Time);
    emit("delay", "leftDelay1", rev->tank.scaledLeftDelay1Time);
    emit("delay", "leftApf2", rev->tank.scaledLeftApf2Time);
    emit("delay", "leftDelay2", rev->tank.scaledLeftDelay2Time);
    emit("delay", "rightApf1", rev->tank.scaledRightApf1Time);
    emit("delay", "rightDelay1", rev->tank.scaledRightDelay1Time);
    emit("delay", "rightApf2", rev->tank.scaledRightApf2Time);
    emit("delay", "rightDelay2", rev->tank.scaledRightDelay2Time);
    emit("delay", "inApf1", Dattorro::kInApf1Time * rev->dattorroScaleFactor);
    emit("delay", "inApf2", Dattorro::kInApf2Time * rev->dattorroScaleFactor);
    emit("delay", "inApf3", Dattorro::kInApf3Time * rev->dattorroScaleFactor);
    emit("delay", "inApf4", Dattorro::kInApf4Time * rev->dattorroScaleFactor);
    static const char* tapNames[7] = {"L_DELAY1_T1", "L_DELAY1_T2", "L_APF2_T", "L_DELAY2_T",
                                      "R_DELAY1_T", "R_APF2_T", "R_DELAY2_T"};
    for (int i = 0; i < 7; ++i)
        emit("tap", tapNames[i], static_cast<float>(rev->tank.scaledOutputTaps[i]));
}

// ---------------------------------------------------------------------------
// LFO audit
// ---------------------------------------------------------------------------

static double measureLfoHz(float effectiveRate, double host, double baseFreq,
                           double modSpeed, double maxSeconds) {
    TriSawLFO lfo(effectiveRate, baseFreq * modSpeed);
    lfo.setRevPoint(0.5);
    const long maxSteps = static_cast<long>(maxSeconds * host);
    double prev = lfo.process();
    double prevPos = -1.0; // interpolated position of the previous upward crossing
    double periodSum = 0.0;
    int periods = 0;
    for (long i = 1; i < maxSteps; ++i) {
        const double cur = lfo.process();
        if (prev < 0.0 && cur >= 0.0) {
            // Linear interpolation of the exact crossing position.
            const double frac = (0.0 - prev) / (cur - prev);
            const double pos = double(i - 1) + frac;
            if (prevPos >= 0.0) {
                periodSum += pos - prevPos;
                ++periods;
            }
            prevPos = pos;
            if (periods >= 4) break;
        }
        prev = cur;
    }
    return periods > 0 ? (host * periods / periodSum) : 0.0;
}

static float effectiveTankRate(Model m, float host) {
    switch (m) {
        case Model::Legacy32k: return std::min(host, 32000.0f);
        case Model::Correct: return host;
        case Model::SrInvariant: return host * (2.0f / 3.0f);
    }
    return host;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    fs::path outDir = (argc > 1) ? fs::path(argv[1]) : fs::path(".");
    fs::create_directories(outDir);

    const std::array<float, 6> hosts = {32000.0f, 44100.0f, 48000.0f, 88200.0f, 96000.0f, 192000.0f};
    const std::array<int, 3> sizes = {0, 1, 2};
    const std::array<float, 3> decays = {0.5f, 0.877465f, 0.95f};
    const std::array<Model, 3> models = {Model::Legacy32k, Model::Correct, Model::SrInvariant};
    const ReverbConfig rc;

    // --- internal delay / tap matrix -------------------------------------
    {
        std::ofstream csv(outDir / "internal_delays.csv");
        csv << "model,host,size,kind,name,samples,ms\n";
        for (auto m : models)
            for (float h : hosts)
                for (int s : sizes)
                    dumpDelays(csv, m, h, s);
    }

    // --- LFO audit --------------------------------------------------------
    {
        std::ofstream csv(outDir / "lfo_audit.csv");
        csv << "model,host,lfo,target_hz,measured_hz,error_pct,excursion_samples,excursion_ms\n";
        const std::array<const char*, 4> lfoNames = {"lfo1", "lfo2", "lfo3", "lfo4"};
        const std::array<double, 4> lfoBase = {0.10, 0.150, 0.120, 0.180};
        const std::array<std::pair<const char*, double>, 4> speeds = {
            {{"min", 0.0}, {"default", 0.0466}, {"mid", 0.5}, {"max", 1.0}}};
        for (auto m : models) {
            for (float h : hosts) {
                const float eff = effectiveTankRate(m, h);
                const double sampleRateScale = eff / kDattorroRate;
                const double excursion = 0.5 * 16.0 * sampleRateScale; // default mod depth
                for (auto& [speedName, speedNorm] : speeds) {
                    const double modSpeed = 0.3 + speedNorm * 15.0;
                    for (int li = 0; li < 4; ++li) {
                        const double target = lfoBase[li] * modSpeed;
                        const double measured = measureLfoHz(eff, h, lfoBase[li], modSpeed, 60.0);
                        const double err = target > 0 ? (measured - target) / target * 100.0 : 0.0;
                        csv << modelName(m) << "," << h << "," << lfoNames[li] << "_" << speedName
                            << "," << target << "," << measured << "," << err << ","
                            << excursion << "," << (excursion / h * 1000.0) << "\n";
                    }
                }
            }
        }
    }

    // --- reverb metric matrix --------------------------------------------
    {
        std::ofstream csv(outDir / "metrics.csv");
        csv << "model,host,size,decay,onset_ms,density_0_50,density_0_100,density_100_500,"
               "crest,rms_tail,peak,rt20,rt30,rt60,centroid_hz,energy_db\n";
        for (auto m : models) {
            for (float h : hosts) {
                for (int s : sizes) {
                    for (float d : decays) {
                        auto rev = makeReverb(m, h, s, d, rc);
                        std::vector<float> L, R;
                        const int n = static_cast<int>(4.0 * h);
                        processBuffer(*rev, impulse(n), L, R);
                        const Metrics mt = analyse(L, h);
                        csv << modelName(m) << "," << h << "," << s << "," << d << ","
                            << mt.onsetMs << "," << mt.density0_50 << "," << mt.density0_100 << ","
                            << mt.density100_500 << "," << mt.crest << "," << mt.rmsTail << ","
                            << mt.peak << "," << mt.rt20 << "," << mt.rt30 << "," << mt.rt60 << ","
                            << mt.centroidHz << "," << mt.energyDb << "\n";
                    }
                }
            }
        }
    }

    // --- golden reference (48 kHz, Large, canonical decay) ----------------
    {
        auto rev = makeReverb(Model::Legacy32k, 48000.0f, 2, 0.877465f, rc);
        std::vector<float> L, R;
        processBuffer(*rev, impulse(static_cast<int>(4.0 * 48000)), L, R);
        parity::writeWav32f((outDir / "golden_legacy_48k_ir.wav").string(), {L}, 48000);
        const Metrics mt = analyse(L, 48000.0);
        std::ofstream csv(outDir / "golden_metrics.csv");
        csv << "onsets_ms,density_0_50,density_0_100,density_100_500,crest,rms_tail,peak,rt20,rt30,rt60,centroid_hz\n";
        csv << mt.onsetMs << "," << mt.density0_50 << "," << mt.density0_100 << ","
            << mt.density100_500 << "," << mt.crest << "," << mt.rmsTail << "," << mt.peak << ","
            << mt.rt20 << "," << mt.rt30 << "," << mt.rt60 << "," << mt.centroidHz << "\n";
    }

    // --- audio renders (listening) ---------------------------------------
    const std::array<float, 3> listenHosts = {44100.0f, 48000.0f, 96000.0f};
    const char* sizeName[3] = {"small", "medium", "large"};
    for (float h : listenHosts) {
        for (auto m : models) {
            // IR (Large) + wet IR
            auto revIR = makeReverb(m, h, 2, 0.877465f, rc);
            std::vector<float> L, R;
            processBuffer(*revIR, impulse(static_cast<int>(4.0 * h)), L, R);
            const std::string base = std::string(modelName(m)) + "_" + std::to_string((int)h);
            parity::writeWav32f((outDir / (base + "_ir.wav")).string(), {L}, (uint32_t)h);

            // Musical, with dry (75% wet-ish) and 100% wet.
            auto revMus = makeReverb(m, h, 2, 0.877465f, rc);
            const std::vector<float> dry = musical(h, 3.0);
            std::vector<float> mL, mR;
            processBuffer(*revMus, dry, mL, mR);
            std::vector<float> wetL(mL), wetR(mR); // 100% wet
            std::vector<float> mixL(mL.size()), mixR(mL.size());
            for (size_t i = 0; i < mL.size(); ++i) {
                mixL[i] = dry[i] * 0.3f + mL[i] * 0.7f;
                mixR[i] = dry[i] * 0.3f + mR[i] * 0.7f;
            }
            parity::writeWav32f((outDir / (base + "_music_mix.wav")).string(), {mixL, mixR}, (uint32_t)h);
            parity::writeWav32f((outDir / (base + "_music_wet.wav")).string(), {wetL, wetR}, (uint32_t)h);
            std::printf("rendered %s (Large, decay 0.877)\n", base.c_str());
        }
    }

    // --- LFO waveform renders (default mod speed) ------------------------
    {
        const double modSpeed = 0.3 + 0.0466 * 15.0;
        for (auto m : models) {
            for (float h : {48000.0f, 96000.0f}) {
                const float eff = effectiveTankRate(m, h);
                TriSawLFO lfo(eff, 0.10 * modSpeed);
                lfo.setRevPoint(0.5);
                const int n = static_cast<int>(30.0 * h);
                std::vector<float> y(n, 0.0f);
                for (int i = 0; i < n; ++i) y[i] = static_cast<float>(lfo.process());
                const std::string fn = std::string("lfo1_") + modelName(m) + "_" +
                                       std::to_string((int)h) + ".wav";
                parity::writeWav32f((outDir / fn).string(), {y}, (uint32_t)h);
            }
        }
    }

    std::printf("\nStage F harness wrote to %s\n", outDir.string().c_str());
    return 0;
}
