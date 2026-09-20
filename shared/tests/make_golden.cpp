// Generates the frozen 48 kHz golden references used by golden_test.cpp.
//
// IMPORTANT: this intentionally uses the *production* Apollo Dattorro (the
// pre-Stage-G implementation) plus earth.cpp's output stage, so the golden is
// an independent reference and not produced by the shared core itself.
//
//   usage: make_golden <output-dir>
//
#include "Dattorro.hpp" // Apollo/Source/DSP/Dattorro/Dattorro.hpp

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static constexpr double kSr = 48000.0;
static constexpr int kFrames = 144000; // 3 s

static void mixGains(float mix, float& dryGain, float& wetGain) {
    const float x2 = 1.0f - mix;
    const float a = mix * x2;
    const float b = a * (1.0f + 1.4186f * a);
    const float c = b + mix;
    const float d = b + x2;
    wetGain = c * c;
    dryGain = d * d;
}

static std::vector<float> render(float mix, double seconds) {
    Dattorro reverb(48000, 16, 4.0);
    reverb.setSampleRate(48000.0f); // legacy clamp -> tank at 32000
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

    float dryGain = 1.0f, wetGain = 1.0f;
    mixGains(mix, dryGain, wetGain);

    const int n = static_cast<int>(seconds * kSr);
    std::vector<float> out(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const float in = (i == 0) ? 1.0f : 0.0f;
        reverb.process(in, in);
        const float wetL = reverb.getLeftOutput();
        out[i] = in * dryGain + wetL * wetGain * 0.4f;
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

    writeF32(outDir / "p0_48k.f32", render(1.0f, 3.0)); // 100% wet
    writeF32(outDir / "p1_48k.f32", render(0.5f, 3.0)); // default 50%

    std::printf("golden written to %s\n", outDir.string().c_str());
    return 0;
}
