#pragma once

namespace poly_octave {

// Fast inverse square root approximation used by the original Earth octave code.
inline float fast_inv_sqrt(float number) noexcept
{
    const float three_halfs = 1.5F;

    float x2 = number * 0.5F;
    float y = number;

    long i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;

    y = y * (three_halfs - (x2 * y * y));
    return y;
}

inline float fast_sqrt(float x) noexcept
{
    return fast_inv_sqrt(x) * x;
}

} // namespace poly_octave
