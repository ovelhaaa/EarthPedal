// Shared-core golden test (Stage G).
//
//   1. Golden @ 48 kHz:  EarthDSPCore must reproduce the frozen production
//      reference (make_golden) for P0 (100% wet) and P1 (default 50%).
//   2. Sample-rate invariance: tank delay time in ms must stay at the golden
//      value across 44.1/48/96/192 kHz.
//   3. Block-size invariance: 64 vs 128 vs 512 must be bit-identical.
//   4. Finite output.
//
//   usage: golden_test <golden-dir>
//
#include "EarthDSPCore.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using earth::EarthDSPCore;
using earth::EarthParameters;
using earth::OctaveMode;
using earth::ReverbSize;

static int g_failures = 0;

static void check(bool ok, const std::string& what) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) ++g_failures;
}

static std::vector<float> loadF32(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    const std::streamoff bytes = f.tellg();
    f.seekg(0);
    std::vector<float> x(static_cast<size_t>(bytes / sizeof(float)));
    f.read(reinterpret_cast<char*>(x.data()), bytes);
    return x;
}

static std::vector<float> renderImpulse(EarthDSPCore& core, double sr, int frames,
                                        int blockSize = 512) {
    std::vector<float> in(frames, 0.0f), out(frames, 0.0f);
    in[0] = 1.0f;
    std::vector<float> outR(frames, 0.0f);
    // Reuse input for both channels (mono impulse).
    for (int start = 0; start < frames; start += blockSize) {
        const int n = std::min(blockSize, frames - start);
        core.process(in.data() + start, in.data() + start,
                     out.data() + start, outR.data() + start, n);
    }
    return out;
}

static double rt30Seconds(const std::vector<float>& x, double sr) {
    const int n = static_cast<int>(x.size());
    std::vector<double> edc(n);
    double acc = 0.0;
    for (int i = n - 1; i >= 0; --i) { acc += double(x[i]) * x[i]; edc[i] = acc; }
    const double peakDb = 10.0 * std::log10(edc[0] + 1e-30);
    auto idxAt = [&](double target) {
        for (int i = 0; i < n; ++i)
            if (10.0 * std::log10(edc[i] + 1e-30) - peakDb <= target) return i;
        return -1;
    };
    const int i5 = idxAt(-5.0), i35 = idxAt(-35.0);
    if (i5 < 0 || i35 <= i5) return 0.0;
    return 60.0 * (i35 - i5) / sr / 30.0;
}

static void dumpF32(const fs::path& p, const std::vector<float>& x) {
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(x.data()), std::streamsize(x.size() * sizeof(float)));
}

static void testGolden(const fs::path& goldenDir, const fs::path& dumpDir) {
    std::printf("[Golden 48 kHz]\n");

    struct Case {
        const char* name;
        float mix;
        OctaveMode mode;
        earth::PerformanceMode perf = earth::PerformanceMode::Freeze;
        bool active = false;
    };
    const Case cases[] = {
        {"p0_48k.f32", 1.0f, OctaveMode::Off},
        {"p1_48k.f32", 0.5f, OctaveMode::Off},
        {"p3_up_48k.f32", 0.5f, OctaveMode::Up},
        {"p4_down_48k.f32", 0.5f, OctaveMode::Down},
        {"p5_both_48k.f32", 0.5f, OctaveMode::Both},
        {"p7_overdrive_48k.f32", 0.5f, OctaveMode::Off, earth::PerformanceMode::Overdrive, true},
    };

    for (const auto& c : cases) {
        const auto golden = loadF32(goldenDir / c.name);
        if (golden.empty()) {
            check(false, std::string("golden file missing (run make_golden): ") + c.name);
            continue;
        }

        EarthDSPCore core;
        core.prepare(48000.0, 512);
        EarthParameters p = EarthParameters::defaults();
        p.mix = c.mix;
        p.octaveMode = c.mode;
        p.performanceMode = c.perf;
        p.performanceActive = c.active;
        core.setParameters(p);
        core.snapParameters();

        const auto out = renderImpulse(core, 48000.0, static_cast<int>(golden.size()));
        if (!dumpDir.empty()) dumpF32(dumpDir / (std::string("core_") + c.name), out);

        double maxAbs = 0.0, diffEnergy = 0.0, refEnergy = 0.0;
        for (size_t i = 0; i < out.size(); ++i) {
            const double d = double(out[i]) - golden[i];
            maxAbs = std::max(maxAbs, std::fabs(d));
            diffEnergy += d * d;
            refEnergy += double(golden[i]) * golden[i];
        }
        const double refRms = std::sqrt(refEnergy / out.size());
        const double nullDb = 20.0 * std::log10((std::sqrt(diffEnergy / out.size()) + 1e-30) / (refRms + 1e-30));
        std::printf("    %s: max|diff|=%.3g  null=%.1f dB\n", c.name, maxAbs, nullDb);
        check(maxAbs < 1e-6, std::string(c.name) + " bit-exact (max|diff| < 1e-6)");
    }
}

static void testSampleRateInvariance() {
    std::printf("[Sample-rate invariance]\n");

    const double kGoldenDelayMs = 4453.0 * 4.0 * (earth::kLegacyTankRatio) / 29761.0 * 1000.0;
    for (double sr : {44100.0, 48000.0, 96000.0, 192000.0}) {
        EarthDSPCore core;
        core.prepare(sr, 512);
        EarthParameters p = EarthParameters::defaults();
        p.mix = 1.0f; // measure the wet tail only
        core.setParameters(p);
        core.snapParameters();

        const auto& rc = core.rateContext();
        const bool ratioOk = std::fabs(rc.timingReferenceRate - sr * earth::kLegacyTankRatio) < 1e-6;

        const double delayMs = core.reverb().tank.scaledLeftDelay1Time / sr * 1000.0;
        const bool delayOk = std::fabs(delayMs - kGoldenDelayMs) < 0.5;

        const auto out = renderImpulse(core, sr, static_cast<int>(sr * 4.0));
        const double rt = rt30Seconds(out, sr);
        const bool rtOk = std::fabs(rt - 5.28) < 0.45;

        bool finite = true;
        for (float v : out) if (!std::isfinite(v)) finite = false;

        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "sr=%.0f timingRef=%.1f leftDelay1=%.2fms RT30=%.2fs",
                      sr, rc.timingReferenceRate, delayMs, rt);
        check(ratioOk && delayOk && rtOk && finite, buf);
    }
}

static void testOctaveSampleRates() {
    std::printf("[Octave across sample rates]\n");

    for (double sr : {44100.0, 96000.0}) {
        EarthParameters p = EarthParameters::defaults();
        p.octaveMode = OctaveMode::Up;
        p.mix = 0.5f;

        auto render = [&](int block) {
            EarthDSPCore core;
            core.prepare(sr, block);
            core.setParameters(p);
            core.snapParameters();
            const int n = static_cast<int>(sr * 1.0);
            std::vector<float> in(n, 0.0f), out(n, 0.0f), outR(n, 0.0f);
            for (int i = 0; i < n; ++i)
                in[i] = 0.3f * std::sin(2.0 * M_PI * 220.0 * i / sr) * std::exp(-i / (sr * 0.5));
            for (int s = 0; s < n; s += block) {
                const int m = std::min(block, n - s);
                core.process(in.data() + s, in.data() + s, out.data() + s, outR.data() + s, m);
            }
            return out;
        };

        const auto a = render(128);
        double rms = 0.0;
        bool finite = true;
        for (float v : a) { rms += double(v) * v; if (!std::isfinite(v)) finite = false; }
        rms = std::sqrt(rms / a.size());

        // Block invariance must also hold with the resampled octave path.
        const auto b = render(512);
        double d = 0.0;
        for (size_t i = 0; i < a.size(); ++i) d = std::max(d, std::fabs(double(a[i]) - b[i]));

        char buf[200];
        std::snprintf(buf, sizeof(buf), "sr=%.0f octave Up: finite=%d rms=%.5f blockDiff=%.3g",
                      sr, finite ? 1 : 0, rms, d);
        check(finite && rms > 1e-5 && d == 0.0, buf);
    }
}

static void testFreeze() {
    std::printf("[Freeze]\n");

    auto tailEnergy = [](const std::vector<float>& x, double sr, double t0, double t1) {
        double e = 0.0;
        const int a = static_cast<int>(t0 * sr), b = std::min<int>(static_cast<int>(t1 * sr), static_cast<int>(x.size()));
        for (int i = a; i < b; ++i) e += double(x[i]) * x[i];
        return e;
    };

    auto render = [&](bool frozen) {
        EarthDSPCore core;
        core.prepare(48000.0, 512);
        EarthParameters p = EarthParameters::defaults();
        p.mix = 1.0f;
        p.performanceMode = earth::PerformanceMode::Freeze;
        p.performanceActive = frozen;
        core.setParameters(p);
        core.snapParameters();
        std::vector<float> in(48000 * 2, 0.0f);
        in[0] = 1.0f;
        std::vector<float> out(in.size(), 0.0f), outR(in.size(), 0.0f);
        for (int s = 0; s < static_cast<int>(in.size()); s += 512) {
            const int n = std::min(512, static_cast<int>(in.size()) - s);
            core.process(in.data() + s, in.data() + s, out.data() + s, outR.data() + s, n);
        }
        return out;
    };

    const auto normal = render(false);
    const auto frozen = render(true);
    bool finite = true;
    for (float v : frozen) if (!std::isfinite(v)) finite = false;

    const double en = tailEnergy(normal, 48000.0, 1.5, 2.0);
    const double ef = tailEnergy(frozen, 48000.0, 1.5, 2.0);
    check(finite && ef > en * 2.0, "freeze sustains the tail and stays finite");

    // Engage / release transition must not produce non-finite output.
    EarthDSPCore core;
    core.prepare(48000.0, 512);
    EarthParameters p = EarthParameters::defaults();
    p.mix = 1.0f;
    p.performanceMode = earth::PerformanceMode::Freeze;
    core.setParameters(p);
    core.snapParameters();
    std::vector<float> in(48000, 0.0f);
    in[0] = 1.0f;
    std::vector<float> out(in.size(), 0.0f), outR(in.size(), 0.0f);
    bool finite2 = true;
    for (int s = 0; s < 48000; s += 512) {
        const int n = std::min(512, 48000 - s);
        p.performanceActive = (s > 10000 && s < 30000);
        core.setParameters(p);
        core.process(in.data() + s, in.data() + s, out.data() + s, outR.data() + s, n);
    }
    for (float v : out) if (!std::isfinite(v)) finite2 = false;
    check(finite2, "freeze engage/release transition is finite");
}

static void testBlockInvariance() {
    std::printf("[Block-size invariance]\n");

    EarthParameters p = EarthParameters::defaults();
    p.octaveMode = OctaveMode::Off;

    auto run = [&](int block) {
        EarthDSPCore core;
        core.prepare(48000.0, block);
        core.setParameters(p);
        core.snapParameters();
        return renderImpulse(core, 48000.0, 48000, block);
    };

    const auto a = run(64);
    const auto b = run(128);
    const auto c = run(512);
    double dab = 0.0, dac = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dab = std::max(dab, std::fabs(double(a[i]) - b[i]));
        dac = std::max(dac, std::fabs(double(a[i]) - c[i]));
    }
    check(dab == 0.0, "64 vs 128 bit-identical");
    check(dac == 0.0, "64 vs 512 bit-identical");
}

int main(int argc, char** argv) {
    const fs::path goldenDir = (argc > 1) ? fs::path(argv[1]) : fs::path("golden");
    const fs::path dumpDir = (argc > 2) ? fs::path(argv[2]) : fs::path();

    testGolden(goldenDir, dumpDir);
    testSampleRateInvariance();
    testOctaveSampleRates();
    testFreeze();
    testBlockInvariance();

    std::printf("\n%s (%d failure(s))\n", g_failures == 0 ? "ALL PASS" : "FAILURES", g_failures);
    return g_failures == 0 ? 0 : 1;
}
