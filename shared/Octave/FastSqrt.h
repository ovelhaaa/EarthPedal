#pragma once

#include <bit>
#include <cstdint>

// Fast inverse square root (Quake-style). Uses an explicit 32-bit integer so
// the result is identical on Windows (32-bit long) and Linux/macOS (64-bit
// long). The original `long`-based bit hack silently read 8 bytes on LP64
// platforms, which made the octave branch non-deterministic across hosts and
// broke the bit-exact golden test in CI.
static float fastInvSqrt(float number) noexcept
{
    const float threehalfs = 1.5F;
    const float x2 = number * 0.5F;
    float y = number;

    std::uint32_t i = std::bit_cast<std::uint32_t>(y);
    i = 0x5f3759dfu - (i >> 1);
    y = std::bit_cast<float>(i);

    y = y * (threehalfs - (x2 * y * y));
    return y;
}

static float fastSqrt(float x)
{
    return fastInvSqrt(x) * x;
}
