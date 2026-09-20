// Generates the frozen 48 kHz golden references used by golden_test.cpp.
//
// IMPORTANT: this intentionally uses the *production* Apollo DSP (the
// pre-Stage-G Dattorro, Multirate and OctaveGenerator) plus earth.cpp's exact
// signal routing, so the golden is an independent reference and not produced by
// the shared core itself. The shelves use the shared RBJ definition (the newly
// unified implementation) — see PARAMETER_ADAPTERS.md.
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

// mode: 0 Off, 1 Up, 2 Down, 3 Both
static std::vector<float> render(int mode, bool includeDry, float mix, double seconds) {
    Dattorro reverb(48000, 16, 4.0);
    reverb.setSampleRate(48000.0f);
    reverb.setTimeScale(4.0f);
    reverb.setPreDelay(0.0f);
    reverb.setInputFilterLowCutoffPitch(0.0f);
    reverb.setInputFilterHighCutoffPitch(10.0f);
    reverb.enableInputDiffusion(true);
    reverb.setDecay(0.877465f);
    reverb.setTankDiffusion(0.7f);
    reverb.setTankFilterLowCutFrequency(0.0f);
    reverb.setTankFilterHighCutFrequency(10.0f);
    reverb.setTankModSpeed(0.3f + 0.0466667f * 15.0f);
    reverb.setTankModDepth(0.0625f * 8.0f);
    reverb.setTankModShape(0.5f);
    reverb.clear();

    Decimator2 decimate;
    Interpolator interpolate;
    OctaveGenerator octave(static_cast<float>(kSr / resample_factor));
    earth::Biquad highShelf = earth::makeHighShelf(kSr, 140.0, 0.707, -11.0);
    earth::Biquad lowShelf = earth::makeLowShelf(kSr, 160.0, 0.707, 5.0);

    std::array<float, resample_factor> buff{};
    std::array<float, resample_factor> buffOut{};
    int bin = 0;

    float dryGain = 1.0f, wetGain = 1.0f;
    mixGains(mix, dryGain, wetGain);

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
            if (mode == 1 || mode == 3) oct += octave.up1() * 2.0f;
            if (mode == 2 || mode == 3) { oct += octave.down1() * 2.0f; oct += octave.down2() * 2.0f; }
            const auto outChunk = interpolate(oct);
            for (size_t j = 0; j < outChunk.size(); ++j) {
                float m = lowShelf.process(highShelf.process(outChunk[j]));
                if (includeDry) m += 0.5f * buff[j];
                buffOut[j] = (mode != 0) ? m : 0.0f;
            }
        }
        bin += 1;
        if (bin > 5) bin = 0;

        const float reverbIn = (mode != 0) ? buffOut[bin] : mono;
        reverb.process(reverbIn, reverbIn);
        const float wet = reverb.getLeftOutput();
        out[i] = in * dryGain + wet * wetGain * 0.4f;
    }
    return out;
}

// Steady-state overdrive (Overdrive active, drive 0.6, octave Off).
static std::vector<float> renderOverdrive(float mix, double seconds) {
    Dattorro reverb(48000, 16, 4.0);
    reverb.setSampleRate(48000.0f);
    reverb.setTimeScale(4.0f);
    reverb.setPreDelay(0.0f);
    reverb.setInputFilterLowCutoffPitch(0.0f);
    reverb.setInputFilterHighCutoffPitch(10.0f);
    reverb.enableInputDiffusion(true);
    reverb.setDecay(0.877465f);
    reverb.setTankDiffusion(0.7f);
    reverb.setTankFilterLowCutFrequency(0.0f);
    reverb.setTankFilterHighCutFrequency(10.0f);
    reverb.setTankModSpeed(0.3f + 0.0466667f * 15.0f);
    reverb.setTankModDepth(0.0625f * 8.0f);
    reverb.setTankModShape(0.5f);
    reverb.clear();

    daisysp::Overdrive od;
    od.Init();
    const float drive = 0.6f;
    od.SetDrive(drive);
    const float comp = 1.0f - (drive * drive * 2.8f - 0.1296f);

    float dryGain = 1.0f, wetGain = 1.0f;
    mixGains(mix, dryGain, wetGain);

    const int n = static_cast<int>(seconds * kSr);
    std::vector<float> out(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const float in = (i == 0) ? 1.0f : 0.0f;
        reverb.process(in, in);
        const float wet = od.Process(reverb.getLeftOutput() * 0.25f) * comp;
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

    writeF32(outDir / "p0_48k.f32", render(0, true, 1.0f, 3.0));   // reverb only, wet
    writeF32(outDir / "p1_48k.f32", render(0, true, 0.5f, 3.0));   // default 50%
    writeF32(outDir / "p3_up_48k.f32", render(1, true, 0.5f, 3.0));   // octave up
    writeF32(outDir / "p4_down_48k.f32", render(2, true, 0.5f, 3.0)); // octave down
    writeF32(outDir / "p5_both_48k.f32", render(3, true, 0.5f, 3.0)); // octave both
    writeF32(outDir / "p7_overdrive_48k.f32", renderOverdrive(0.5f, 3.0)); // overdrive

    std::printf("golden written to %s\n", outDir.string().c_str());
    return 0;
}
