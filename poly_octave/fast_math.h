#pragma once

#include <bit>
#include <cstdint>

namespace poly_octave {

// Fast inverse square root approximation used by the original Earth octave code.
inline float fast_inv_sqrt(float number) noexcept
{
    // Guard against zero or extremely small values to avoid NaN propagation
    constexpr float epsilon = 1e-30f;
    if (number <= epsilon) {
        return 0.0f;
    }

    const float three_halfs = 1.5F;

    float x2 = number * 0.5F;
    float y = number;

    // Use std::bit_cast for well-defined type-punning instead of pointer casts
    uint32_t i = std::bit_cast<uint32_t>(y);
    i = 0x5f3759df - (i >> 1);
    y = std::bit_cast<float>(i);

    y = y * (three_halfs - (x2 * y * y));
    return y;
}

inline float fast_sqrt(float x) noexcept
{
    // Guard against zero or extremely small values
    constexpr float epsilon = 1e-30f;
    if (x <= epsilon) {
        return 0.0f;
    }

    return fast_inv_sqrt(x) * x;
}

} // namespace poly_octave