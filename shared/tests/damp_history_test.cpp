#include "EarthDSPCore.h"

#include <array>
#include <cmath>
#include <cstdio>

// Inspect the filter settings without resetting the core: reset/snap alone
// would miss a stale filter during live, smoothed automation.
int main()
{
    constexpr std::array<std::array<float, 2>, 6> transitions {{
        { 0.5f, 0.2f }, { 0.2f, 0.5f }, { 0.5f, 0.8f },
        { 0.8f, 0.5f }, { 0.2f, 0.8f }, { 0.8f, 0.2f }
    }};
    int failures = 0;
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (bool smoothed : { false, true })
            for (const auto& transition : transitions)
            {
                earth::EarthDSPCore core, reference;
                core.prepare(rate, 256);
                reference.prepare(rate, 256);
                auto p = earth::EarthParameters::defaults();
                p.damp = transition[0];
                core.setParameters(p);
                core.snapParameters();
                p.damp = transition[1];
                core.setParameters(p);
                reference.setParameters(p);
                reference.snapParameters();
                bool finite = true;
                if (smoothed)
                {
                    std::array<float, 256> silence {}, left {}, right {};
                    for (int block = 0; block < 8; ++block)
                    {
                        core.process(silence.data(), silence.data(), left.data(), right.data(), 256);
                        for (size_t i = 0; i < left.size(); ++i)
                            finite = finite && std::isfinite(left[i]) && std::isfinite(right[i]);
                    }
                }
                else
                    core.snapParameters();

                const float highPitch = p.damp < 0.5f ? 14.0f * p.damp + 3.0f : 10.0f;
                const float lowPitch = p.damp > 0.5f ? 18.0f * (p.damp - 0.5f) : 0.0f;
                const auto hz = [] (float pitch) { return 440.0f * std::pow(2.0f, pitch - 5.0f); };
                const auto close = [] (float a, float b) { return std::abs(a - b) < 0.01f; };
                const bool ok = finite
                    && close(core.reverb().inputHighCut, hz(highPitch))
                    && close(core.reverb().inputLowCut, hz(lowPitch))
                    && close(core.reverb().inputHighCut, reference.reverb().inputHighCut)
                    && close(core.reverb().inputLowCut, reference.reverb().inputLowCut);
                std::printf("[%s] %.0f Hz %s Tone %.1f -> %.1f\n", ok ? "PASS" : "FAIL",
                            rate, smoothed ? "smoothed" : "snapped", transition[0], transition[1]);
                failures += !ok;
            }
    return failures == 0 ? 0 : 1;
}
