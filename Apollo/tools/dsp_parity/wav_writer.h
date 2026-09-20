#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// Minimal, dependency-free 32-bit IEEE-float WAV writer.
// Channels are written interleaved from planar input.
namespace parity {

inline void writeLE16(FILE* f, uint16_t v) {
    uint8_t b[2] = {static_cast<uint8_t>(v & 0xff), static_cast<uint8_t>((v >> 8) & 0xff)};
    std::fwrite(b, 1, 2, f);
}

inline void writeLE32(FILE* f, uint32_t v) {
    uint8_t b[4] = {static_cast<uint8_t>(v & 0xff),
                    static_cast<uint8_t>((v >> 8) & 0xff),
                    static_cast<uint8_t>((v >> 16) & 0xff),
                    static_cast<uint8_t>((v >> 24) & 0xff)};
    std::fwrite(b, 1, 4, f);
}

// channels: planar buffers, frames = length of each channel.
inline bool writeWav32f(const std::string& path,
                        const std::vector<std::vector<float>>& channels,
                        uint32_t sampleRate) {
    if (channels.empty()) return false;
    const uint32_t numChannels = static_cast<uint32_t>(channels.size());
    const uint32_t numFrames = static_cast<uint32_t>(channels[0].size());

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;

    const uint32_t bitsPerSample = 32;
    const uint32_t blockAlign = numChannels * bitsPerSample / 8;
    const uint32_t byteRate = sampleRate * blockAlign;
    const uint32_t dataBytes = numFrames * blockAlign;

    std::fwrite("RIFF", 1, 4, f);
    writeLE32(f, 36 + dataBytes);
    std::fwrite("WAVE", 1, 4, f);

    std::fwrite("fmt ", 1, 4, f);
    writeLE32(f, 16);
    writeLE16(f, 3); // IEEE float
    writeLE16(f, static_cast<uint16_t>(numChannels));
    writeLE32(f, sampleRate);
    writeLE32(f, byteRate);
    writeLE16(f, static_cast<uint16_t>(blockAlign));
    writeLE16(f, static_cast<uint16_t>(bitsPerSample));

    std::fwrite("data", 1, 4, f);
    writeLE32(f, dataBytes);

    for (uint32_t i = 0; i < numFrames; ++i) {
        for (uint32_t c = 0; c < numChannels; ++c) {
            const float s = channels[c][i];
            uint32_t bits;
            std::memcpy(&bits, &s, sizeof(float));
            writeLE32(f, bits);
        }
    }

    std::fclose(f);
    return true;
}

} // namespace parity
