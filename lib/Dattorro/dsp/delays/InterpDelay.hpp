#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

class InterpDelay {
public:
    float input = 0.;
    float output = 0.;
    float* buffer = nullptr;
    int r = 0;
    int upperR = 0;
    int j = 0;
    float dataR = 0.;
    float dataUpperR = 0.;

    // Default constructor
    InterpDelay() {}

    // Init method to set buffer
    void Init(float* buf, size_t size) {
        buffer = buf;
        l = size;
        lDouble = static_cast<float>(l);
        clear();
    }

    // For compatibility if needed, but we prefer Init
    // InterpDelay(unsigned int maxLength, float initDelayTime = 0.) { ... }
    // We can't allocate here easily without malloc/new which we want to avoid or control carefully.
    // So we rely on Init.

    #pragma GCC push_options
    #pragma GCC optimize ("Ofast")

    inline void process() {
        if (!buffer) return;

        buffer[w] = input;
        r = w - t;

        if (r < 0) {
            r += l;
        }

        ++w;
        if (w >= l) {
            w = 0;
        }

        upperR = r - 1;
        if (upperR < 0) {
            upperR += l;
        }

        dataR = buffer[r];
        dataUpperR = buffer[upperR];

        // Removed clearPopCancelValue logic for simplicity unless it's defined somewhere I missed.
        // The original code had extern float clearPopCancelValue;
        // I'll assume it was for pop reduction on clear. I'll omit it or set it to 1.0 if not found.

        output = hold * (dataR + f * (dataUpperR - dataR));
    }

    #pragma GCC pop_options
    #pragma GCC push_options
    #pragma GCC optimize ("Ofast")

    inline float tap(const int &i) {
        if (!buffer) return 0.0f;
        j = w - i;
        if (j < 0) {
            j += l;
        }
        return buffer[j];
    }

    #pragma GCC pop_options
    #pragma GCC push_options
    #pragma GCC optimize ("Ofast")

    inline void setDelayTime(float newDelayTime) {
        if (newDelayTime >= lDouble) {
            newDelayTime = lDouble - 1.;
        }
        if (newDelayTime < 0.) {
            newDelayTime = 0.;
        }
        t = static_cast<int>(newDelayTime);
        f = newDelayTime - static_cast<float>(t);
    }

    #pragma GCC pop_options

    void clear() {
        if (!buffer) return;
        for(int i = 0; i < l; ++i) {
            buffer[i] = 0.0f;
        }
        input = 0.;
        output = 0.;
        w = 0;
    }

    // Helper to allow external control of hold (was extern float hold)
    void setHold(float h) { hold = h; }
    // float hold = 1.0f; // Made member

private:
    int w = 0;
    int t = 0;
    float f = 0.;
    int l = 512;
    float lDouble = 512.;
    float hold = 1.0f;
};
