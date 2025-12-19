#pragma once
#include "InterpDelay.hpp"

class AllpassFilter {
public:
    AllpassFilter() {
        gain = 0.;
    }

    void Init(float* buffer, size_t size) {
        delay.Init(buffer, size);
        clear();
    }

    #pragma GCC push_options
    #pragma GCC optimize ("Ofast")

    inline float process() {
        _inSum = input + delay.output * gain;
        output = delay.output + _inSum * gain * -1.;
        delay.input = _inSum;
        delay.process();
        return output;
    }

    #pragma GCC pop_options

    void clear() {
        input = 0.;
        output = 0.;
        _inSum = 0.;
        _outSum = 0.;
        delay.clear();
    }

    inline void setGain(const float &newGain) {
        gain = newGain;
    }

    // Pass through for delay time setting
    void setDelayTime(float time) {
        delay.setDelayTime(time);
    }

    float input;
    float output;
    InterpDelay delay;

private:
    float gain;
    float _inSum;
    float _outSum;
};
