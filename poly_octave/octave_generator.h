#pragma once

#include <cmath>
#include <vector>

#include "band_shifter.h"

namespace poly_octave {

class OctaveGenerator
{
  public:
    void Init(float sample_rate)
    {
        shifters_.clear();
        shifters_.reserve(kNumBands);
        for(int i = 0; i < kNumBands; ++i)
        {
            shifters_.emplace_back(center_freq(i), sample_rate, bandwidth(i));
        }
        Reset();
    }

    void Reset()
    {
        up1_ = down1_ = down2_ = 0.0f;
        for(auto& shifter : shifters_)
            shifter.Reset();
    }

    void Update(float sample)
    {
        up1_ = down1_ = down2_ = 0.0f;

        for(auto& shifter : shifters_)
        {
            shifter.Update(sample);
            up1_ += shifter.up1();
            down1_ += shifter.down1();
            down2_ += shifter.down2();
        }
    }

    float up1() const { return up1_; }
    float down1() const { return down1_; }
    float down2() const { return down2_; }

  private:
    static constexpr int kNumBands = 80;

    static float center_freq(int n)
    {
        return 480.0f * std::pow(2.0f, (0.027f * static_cast<float>(n))) - 420.0f;
    }

    static float bandwidth(int n)
    {
        const float f0 = center_freq(n - 1);
        const float f1 = center_freq(n);
        const float f2 = center_freq(n + 1);
        const float a  = (f2 - f1);
        const float b  = (f1 - f0);
        return 2.0f * (a * b) / (a + b);
    }

    std::vector<BandShifter> shifters_;

    float up1_ = 0.0f;
    float down1_ = 0.0f;
    float down2_ = 0.0f;
};

} // namespace poly_octave
