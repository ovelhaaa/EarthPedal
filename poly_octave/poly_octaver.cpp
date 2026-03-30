#include "poly_octaver.h"

#include <span>

namespace poly_octave {

void PolyOctaver::Init(float sample_rate)
{
    sample_rate_ = sample_rate;

    eq1_.config(-11, 140_Hz, sample_rate_);
    eq2_.config(5, 160_Hz, sample_rate_);

    octave_generator_.Init(sample_rate_ / static_cast<float>(kResampleFactor));
    Reset();
}

float PolyOctaver::ProcessMono(float in)
{
    input_bin_[bin_counter_] = in;

    if(bin_counter_ > (kResampleFactor - 2))
    {
        std::span<const float, kResampleFactor> in_chunk(input_bin_.data(), kResampleFactor);
        const float                              sample = decimator_.Process(in_chunk);

        float octave_mix = 0.0f;
        octave_generator_.Update(sample);

        if(mode_ != Mode::Off)
            octave_mix += octave_generator_.up1() * up_gain_;

        if(mode_ == Mode::UpDown)
        {
            octave_mix += octave_generator_.down1() * down1_gain_;
            octave_mix += octave_generator_.down2() * down2_gain_;
        }

        const auto out_chunk = interpolator_.Process(octave_mix);
        for(std::size_t j = 0; j < out_chunk.size(); ++j)
        {
            float mix = eq2_(eq1_(out_chunk[j]));
            if(internal_dry_enabled_)
                mix += dry_blend_ * input_bin_[j];

            output_bin_[j] = (mode_ != Mode::Off) ? mix : 0.0f;
        }
    }

    ++bin_counter_;
    if(bin_counter_ >= kResampleFactor)
        bin_counter_ = 0;

    return output_bin_[bin_counter_];
}

void PolyOctaver::ProcessBlockMono(const float* in, float* out, std::size_t size)
{
    for(std::size_t i = 0; i < size; ++i)
        out[i] = ProcessMono(in[i]);
}

void PolyOctaver::Reset()
{
    decimator_.Reset();
    interpolator_.Reset();
    octave_generator_.Reset();

    input_bin_.fill(0.0f);
    output_bin_.fill(0.0f);
    bin_counter_ = 0;
}

} // namespace poly_octave
