// Generates the frozen 48 kHz golden references used by golden_test.cpp.
//
// IMPORTANT: this intentionally uses the *production* Apollo DSP (the
// pre-Stage-G Dattorro, Multirate, OctaveGenerator and Overdrive) plus
// earth.cpp's exact signal routing, so the golden is an independent reference
// and not produced by the shared core itself. The shelves use the shared RBJ
// definition (the newly unified implementation) — see PARAMETER_ADAPTERS.md.
//
//   usage: make_golden <output-dir>
//
#include "Dattorro.hpp"        // Apollo/Source/DSP/Dattorro/Dattorro.hpp
#include "Multirate.h"         // Apollo/Source/DSP/Util/Multirate.h
#include "OctaveGenerator.h"   // Apollo/Source/DSP/Util/OctaveGenerator.h
#include "Filters/ShelfFilter.h" // shared/Filters/ShelfFilter.h
#include "overdrive.h"           // Apollo/Source/DSP/DaisySP/overdrive.h

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static constexpr double kSr = 48000.0;

static void mixGains(float mix, float& dryGain, float& wetGain) {
    const float x2 = 1.0f - mix;
    const float a = mix * x2;
    const float b = a * (1.0f + 1.4186f * a);
    const float c = b + mix;
    const float d = b + x2;
    wetGain = c * c;
    dryGain = d * d;
}

struct Cfg {
    int mode = 0;              // 0 Off, 1 Up, 2 Down, 3 Both
    bool includeDry = true;
    float mix = 0.5f;
    float preDelay = 0.0f;
    float decay = 0.877465f;
    float modDepth = 0.0625f;
    float modSpeed = 0.0466667f;
    bool inputDiffusion = true;
    bool overdrive = false;
};

static std::vector<float> render(const Cfg& cfg, double seconds) {
    Dattorro reverb(48000, 16, 4.0);
    reverb.setSampleRate(48000.0f);
    reverb.setTimeScale(4.0f);
    reverb.setPreDelay(cfg.preDelay);
    reverb.setInputFilterLowCutoffPitch(0.0f);
    reverb.setInputFilterHighCutoffPitch(10.0f);
    reverb.enableInputDiffusion(cfg.inputDiffusion);
    reverb.setDecay(cfg.decay);
    reverb.setTankDiffusion(0.7f);
    reverb.setTankFilterLowCutFrequency(0.0f);
    reverb.setTankFilterHighCutFrequency(10.0f);
    reverb.setTankModSpeed(0.3f + cfg.modSpeed * 15.0f);
    reverb.setTankModDepth(cfg.modDepth * 8.0f);
    reverb.setTankModShape(0.5f);
    reverb.clear();

    Decimator2 decimate;
    Interpolator interpolate;
    OctaveGenerator octave(static_cast<float>(kSr / resample_factor));
    earth::Biquad highShelf = earth::makeHighShelf(kSr, 140.0, 0.707, -11.0);
    earth::Biquad lowShelf = earth::makeLowShelf(kSr, 160.0, 0.707, 5.0);

    daisysp::Overdrive od;
    od.Init();
    const float odDrive = 0.6f;
    od.SetDrive(odDrive);
    const float odComp = 1.0f - (odDrive * odDrive * 2.8f - 0.1296f);

    std::array<float, resample_factor> buff{};
    std::array<float, resample_factor> buffOut{};
    int bin = 0;

    float dryGain = 1.0f, wetGain = 1.0f;
    mixGains(cfg.mix, dryGain, wetGain);

    const int n = static_cast<int>(seconds * kSr);
    std::vector<float> out(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const float in = (i == 0) ? 1.0f : 0.0f;
        const float mono = in;

        buff[bin] = mono;
        if (bin > 4) {
            std::span<const float, resample_factor> chunk(&buff[0], resample_factor);
            const float sample = decimate(chunk);
            octave.update(sample);
            float oct = 0.0f;
            if (cfg.mode == 1 || cfg.mode == 3) oct += octave.up1() * 2.0f;
            if (cfg.mode == 2 || cfg.mode == 3) { oct += octave.down1() * 2.0f; oct += octave.down2() * 2.0f; }
            const auto outChunk = interpolate(oct);
            for (size_t j = 0; j < outChunk.size(); ++j) {
                float m = lowShelf.process(highShelf.process(outChunk[j]));
                if (cfg.includeDry) m += 0.5f * buff[j];
                buffOut[j] = (cfg.mode != 0) ? m : 0.0f;
            }
        }
        bin += 1;
        if (bin > 5) bin = 0;

        const float reverbIn = (cfg.mode != 0) ? buffOut[bin] : mono;
        reverb.process(reverbIn, reverbIn);
        float wet = reverb.getLeftOutput();
        if (cfg.overdrive) wet = od.Process(wet * 0.25f) * odComp;

        out[i] = in * dryGain + wet * wetGain * 0.4f;
    }
    return out;
}

static void writeF32(const fs::path& path, const std::vector<float>& x) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(x.data()), std::streamsize(x.size() * sizeof(float)));
}

int main(int argc, char** argv) {
    const fs::path outDir = (argc > 1) ? fs::path(argv[1]) : fs::path("golden");
    fs::create_directories(outDir);

    Cfg p0; p0.mix = 1.0f;
    Cfg p1;
    Cfg p2; p2.modDepth = 1.0f; p2.modSpeed = 0.5f;
    Cfg p3; p3.mode = 1;
    Cfg p4; p4.mode = 2;
    Cfg p5; p5.mode = 3;
    Cfg p7; p7.overdrive = true;
    Cfg p8; p8.preDelay = 0.1f;
    Cfg p9; p9.preDelay = 0.5f;
    Cfg p10; p10.mix = 0.0f;
    Cfg p12; p12.inputDiffusion = false;

    writeF32(outDir / "p0_48k.f32", render(p0, 3.0));
    writeF32(outDir / "p1_48k.f32", render(p1, 3.0));
    writeF32(outDir / "p2_mod_48k.f32", render(p2, 3.0));
    writeF32(outDir / "p3_up_48k.f32", render(p3, 3.0));
    writeF32(outDir / "p4_down_48k.f32", render(p4, 3.0));
    writeF32(outDir / "p5_both_48k.f32", render(p5, 3.0));
    writeF32(outDir / "p7_overdrive_48k.f32", render(p7, 3.0));
    writeF32(outDir / "p8_predelay100_48k.f32", render(p8, 3.0));
    writeF32(outDir / "p9_predelay500_48k.f32", render(p9, 3.0));
    writeF32(outDir / "p10_dry_48k.f32", render(p10, 3.0));
    writeF32(outDir / "p12_nodiffusion_48k.f32", render(p12, 3.0));

    std::printf("golden written to %s\n", outDir.string().c_str());
    return 0;
}
