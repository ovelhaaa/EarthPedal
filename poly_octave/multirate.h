#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace poly_octave {

inline constexpr std::size_t kResampleFactor = 6;

template <std::size_t N>
class DelayLine
{
  public:
    void push(float value)
    {
        data_[index_] = value;
        index_ = (index_ + 1) % N;
    }

    float operator[](std::size_t i) const
    {
        return data_[(index_ + i) % N];
    }

    void reset()
    {
        data_.fill(0.0f);
        index_ = 0;
    }

  private:
    std::array<float, N> data_{};
    std::size_t          index_ = 0;
};

class Decimator2
{
  public:
    float Process(std::span<const float, kResampleFactor> s)
    {
        buffer1_.push(s[0]);
        buffer1_.push(s[1]);
        buffer1_.push(s[2]);
        buffer2_.push(Filter1());

        buffer1_.push(s[3]);
        buffer1_.push(s[4]);
        buffer1_.push(s[5]);
        buffer2_.push(Filter1());

        return Filter2();
    }

    void Reset()
    {
        buffer1_.reset();
        buffer2_.reset();
    }

  private:
    float Filter1() const
    {
        return 0.000066177472224418f * (buffer1_[offset1 + 0] + buffer1_[offset1 + 20])
               + 0.0009613901552378511f * (buffer1_[offset1 + 1] + buffer1_[offset1 + 19])
               + 0.003835090815380887f * (buffer1_[offset1 + 2] + buffer1_[offset1 + 18])
               + 0.010496532623165526f * (buffer1_[offset1 + 3] + buffer1_[offset1 + 17])
               + 0.02272703591356282f * (buffer1_[offset1 + 4] + buffer1_[offset1 + 16])
               + 0.041464390530886956f * (buffer1_[offset1 + 5] + buffer1_[offset1 + 15])
               + 0.06591039391505207f * (buffer1_[offset1 + 6] + buffer1_[offset1 + 14])
               + 0.09309984953947406f * (buffer1_[offset1 + 7] + buffer1_[offset1 + 13])
               + 0.11829177835273737f * (buffer1_[offset1 + 8] + buffer1_[offset1 + 12])
               + 0.13620590247679107f * (buffer1_[offset1 + 9] + buffer1_[offset1 + 11])
               + 0.14270010010002276f * buffer1_[offset1 + 10];
    }

    float Filter2() const
    {
        return -0.00299995f * (buffer2_[offset2 + 0] + buffer2_[offset2 + 14])
               + 0.01858487f * (buffer2_[offset2 + 2] + buffer2_[offset2 + 12])
               - 0.06984829f * (buffer2_[offset2 + 4] + buffer2_[offset2 + 10])
               + 0.30421664f * (buffer2_[offset2 + 6] + buffer2_[offset2 + 8])
               + 0.5f * buffer2_[offset2 + 7];
    }

    static constexpr std::size_t bsize1  = 32;
    static constexpr std::size_t fsize1  = 21;
    static constexpr std::size_t offset1 = bsize1 - fsize1;

    static constexpr std::size_t bsize2  = 16;
    static constexpr std::size_t fsize2  = 15;
    static constexpr std::size_t offset2 = bsize2 - fsize2;

    DelayLine<bsize1> buffer1_;
    DelayLine<bsize2> buffer2_;
};

class Interpolator
{
  public:
    std::array<float, kResampleFactor> Process(float s)
    {
        std::array<float, kResampleFactor> output{};

        buffer1_.push(s);

        buffer2_.push(Filter1a());
        output[0] = Filter2a();
        output[1] = Filter2b();
        output[2] = Filter2c();

        buffer2_.push(Filter1b());
        output[3] = Filter2a();
        output[4] = Filter2b();
        output[5] = Filter2c();

        return output;
    }

    void Reset()
    {
        buffer1_.reset();
        buffer2_.reset();
    }

  private:
    float Filter1a() const
    {
        return -0.0028536199247471473f * (buffer1_[offset1 + 0] + buffer1_[offset1 + 24])
               - 0.040326725115203695f * (buffer1_[offset1 + 1] + buffer1_[offset1 + 23])
               - 0.036134596458820015f * (buffer1_[offset1 + 2] + buffer1_[offset1 + 22])
               + 0.033522051189265496f * (buffer1_[offset1 + 3] + buffer1_[offset1 + 21])
               - 0.031442224275585025f * (buffer1_[offset1 + 4] + buffer1_[offset1 + 20])
               + 0.03258337681750486f * (buffer1_[offset1 + 5] + buffer1_[offset1 + 19])
               - 0.03538414864961937f * (buffer1_[offset1 + 6] + buffer1_[offset1 + 18])
               + 0.038811868988079715f * (buffer1_[offset1 + 7] + buffer1_[offset1 + 17])
               - 0.042204493894155204f * (buffer1_[offset1 + 8] + buffer1_[offset1 + 16])
               + 0.045128824129776035f * (buffer1_[offset1 + 9] + buffer1_[offset1 + 15])
               - 0.04736995557907843f * (buffer1_[offset1 + 10] + buffer1_[offset1 + 14])
               + 0.048831901671617876f * (buffer1_[offset1 + 11] + buffer1_[offset1 + 13])
               + 0.9507771467941135f * buffer1_[offset1 + 12];
    }

    float Filter1b() const
    {
        return -0.015961858776449508f * (buffer1_[offset1 + 0] + buffer1_[offset1 + 23])
               - 0.056128740058266235f * (buffer1_[offset1 + 1] + buffer1_[offset1 + 22])
               + 0.011026026040094625f * (buffer1_[offset1 + 2] + buffer1_[offset1 + 21])
               + 0.003198795994721635f * (buffer1_[offset1 + 3] + buffer1_[offset1 + 20])
               - 0.01108582057161854f * (buffer1_[offset1 + 4] + buffer1_[offset1 + 19])
               + 0.01951384497860086f * (buffer1_[offset1 + 5] + buffer1_[offset1 + 18])
               - 0.030860282826182514f * (buffer1_[offset1 + 6] + buffer1_[offset1 + 17])
               + 0.04707993944078406f * (buffer1_[offset1 + 7] + buffer1_[offset1 + 16])
               - 0.07155908583004919f * (buffer1_[offset1 + 8] + buffer1_[offset1 + 15])
               + 0.1129220770668398f * (buffer1_[offset1 + 9] + buffer1_[offset1 + 14])
               - 0.2033122562119347f * (buffer1_[offset1 + 10] + buffer1_[offset1 + 13])
               + 0.6336728217960803f * (buffer1_[offset1 + 11] + buffer1_[offset1 + 12]);
    }

    float Filter2a() const
    {
        return 0.00036440608905813593f * buffer2_[offset2 + 0] + 0.0005821260464558225f * buffer2_[offset2 + 1]
               - 0.043244023722481956f * buffer2_[offset2 + 2] - 0.10310036386076359f * buffer2_[offset2 + 3]
               + 0.13604229993913602f * buffer2_[offset2 + 4] + 0.5503466630244301f * buffer2_[offset2 + 5]
               + 0.4407091552750118f * buffer2_[offset2 + 6] + 0.009420000864297772f * buffer2_[offset2 + 7]
               - 0.09801301258361905f * buffer2_[offset2 + 8] - 0.019627176246818184f * buffer2_[offset2 + 9]
               + 0.001762424830497545f * buffer2_[offset2 + 10];
    }

    float Filter2b() const
    {
        return 0.001112114188613258f * (buffer2_[offset2 + 0] + buffer2_[offset2 + 10])
               - 0.005449383064836152f * (buffer2_[offset2 + 1] + buffer2_[offset2 + 9])
               - 0.07276547446584428f * (buffer2_[offset2 + 2] + buffer2_[offset2 + 8])
               - 0.0709695783332148f * (buffer2_[offset2 + 3] + buffer2_[offset2 + 7])
               + 0.2904591843823435f * (buffer2_[offset2 + 4] + buffer2_[offset2 + 6])
               + 0.590541634315722f * buffer2_[offset2 + 5];
    }

    float Filter2c() const
    {
        return 0.001762424830497545f * buffer2_[offset2 + 0] - 0.019627176246818184f * buffer2_[offset2 + 1]
               - 0.09801301258361905f * buffer2_[offset2 + 2] + 0.009420000864297772f * buffer2_[offset2 + 3]
               + 0.4407091552750118f * buffer2_[offset2 + 4] + 0.5503466630244301f * buffer2_[offset2 + 5]
               + 0.13604229993913602f * buffer2_[offset2 + 6] - 0.10310036386076359f * buffer2_[offset2 + 7]
               - 0.043244023722481956f * buffer2_[offset2 + 8] + 0.0005821260464558225f * buffer2_[offset2 + 9]
               + 0.00036440608905813593f * buffer2_[offset2 + 10];
    }

    static constexpr std::size_t bsize1  = 32;
    static constexpr std::size_t fsize1  = 25;
    static constexpr std::size_t offset1 = bsize1 - fsize1;

    static constexpr std::size_t bsize2  = 16;
    static constexpr std::size_t fsize2  = 11;
    static constexpr std::size_t offset2 = bsize2 - fsize2;

    DelayLine<bsize1> buffer1_;
    DelayLine<bsize2> buffer2_;
};

} // namespace poly_octave
