#pragma once

#include <cmath>
#include <complex>
#include <numbers>

#include "fast_math.h"

namespace poly_octave {

class BandShifter
{
  public:
    BandShifter() = default;

    BandShifter(float center, float sample_rate, float bandwidth)
    {
        Init(center, sample_rate, bandwidth);
    }

    void Init(float center, float sample_rate, float bandwidth)
    {
        constexpr auto pi = std::numbers::pi_v<double>;
        constexpr auto j  = std::complex<double>(0, 1);

        const auto w0      = pi * bandwidth / sample_rate;
        const auto cos_w0  = std::cos(w0);
        const auto sin_w0  = std::sin(w0);
        const auto sqrt_2  = std::sqrt(2.0);
        const auto a0      = (1 + sqrt_2 * sin_w0 / 2);
        const auto g       = (1 - cos_w0) / (2 * a0);
        const auto w1      = 2 * pi * center / sample_rate;
        const auto e1      = std::exp(j * w1);
        const auto e2      = std::exp(j * w1 * 2.0);

        _d0 = g;
        _d1 = std::complex<float>((e1 * 2.0 * g).real(), (e1 * 2.0 * g).imag());
        _d2 = std::complex<float>((e2 * g).real(), (e2 * g).imag());
        _c1 = std::complex<float>((e1 * (-2 * cos_w0) / a0).real(), (e1 * (-2 * cos_w0) / a0).imag());
        _c2 = std::complex<float>((e2 * (1 - sqrt_2 * sin_w0 / 2) / a0).real(),
                                  (e2 * (1 - sqrt_2 * sin_w0 / 2) / a0).imag());

        Reset();
    }

    void Reset()
    {
        _s1 = _s2 = _y = _down1 = std::complex<float>{0.0f, 0.0f};
        _up1 = _down2 = 0.0f;
        _down1_sign = 1.0f;
        _down2_sign = 1.0f;
    }

    void Update(float sample)
    {
        UpdateFilter(sample);
        UpdateUp1();
        UpdateDown1();
        UpdateDown2();
    }

    float up1() const { return _up1; }
    float down1() const { return _down1.real(); }
    float down2() const { return _down2; }

  private:
    void UpdateFilter(float sample)
    {
        const auto prev_y = _y;
        _y = _s2 + _d0 * sample;
        _s2 = _s1 + _d1 * sample - _c1 * _y;
        _s1 = _d2 * sample - _c2 * _y;

        if((_y.real() < 0) && (std::signbit(_y.imag()) != std::signbit(prev_y.imag())))
            _down1_sign = -_down1_sign;
    }

    void UpdateUp1()
    {
        const auto a = _y.real();
        const auto b = _y.imag();
        _up1         = (a * a - b * b) * fast_inv_sqrt(a * a + b * b);
    }

    void UpdateDown1()
    {
        const auto a      = _y.real();
        const auto b      = _y.imag();
        const auto b_sign = (b < 0) ? -1.0f : 1.0f;

        const auto x = 0.5f * a * fast_inv_sqrt(a * a + b * b);
        const auto c = fast_sqrt(0.5f + x);
        const auto d = b_sign * fast_sqrt(0.5f - x);

        const auto prev_down1 = _down1;
        _down1 = _down1_sign * std::complex<float>((a * c + b * d), (b * c - a * d));

        if((_down1.real() < 0) && (std::signbit(_down1.imag()) != std::signbit(prev_down1.imag())))
            _down2_sign = -_down2_sign;
    }

    void UpdateDown2()
    {
        const auto a      = _down1.real();
        const auto b      = _down1.imag();
        const auto b_sign = (b < 0) ? -1.0f : 1.0f;

        const auto x = 0.5f * a * fast_inv_sqrt(a * a + b * b);
        const auto c = fast_sqrt(0.5f + x);
        const auto d = b_sign * fast_sqrt(0.5f - x);

        _down2 = _down2_sign * (a * c + b * d);
    }

    float               _d0 = 0.0f;
    std::complex<float> _d1{};
    std::complex<float> _d2{};
    std::complex<float> _c1{};
    std::complex<float> _c2{};

    std::complex<float> _s1{};
    std::complex<float> _s2{};

    std::complex<float> _y{};
    float               _up1 = 0.0f;
    std::complex<float> _down1{};
    float               _down2 = 0.0f;

    float _down1_sign = 1.0f;
    float _down2_sign = 1.0f;
};

} // namespace poly_octave
