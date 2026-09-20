#include "Overdrive.h"

#include <algorithm>

namespace earth {

static inline float softClip(float x) {
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

void Overdrive::init() {
    setDrive(0.5f);
}

float Overdrive::process(float in) {
    return softClip(preGain_ * in) * postGain_;
}

void Overdrive::setDrive(float drive) {
    drive = std::clamp(drive, 0.0f, 1.0f);
    drive_ = 2.0f * drive;

    const float drive2 = drive_ * drive_;
    const float preGainA = drive_ * 0.5f;
    const float preGainB = drive2 * drive2 * drive_ * 24.0f;
    preGain_ = preGainA + (preGainB - preGainA) * drive2;

    const float driveSquashed = drive_ * (2.0f - drive_);
    postGain_ = 1.0f / softClip(0.33f + driveSquashed * (preGain_ - 0.33f));
}

} // namespace earth
