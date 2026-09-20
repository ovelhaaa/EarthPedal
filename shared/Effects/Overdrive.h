/*
Port of the DaisySP Overdrive (MIT, Electrosmith / Emilie Gillet), used by the
Apollo plugin. Moved into the shared core so Web and JUCE run the same
algorithm. Namespaced as `earth` to avoid clashing with the legacy daisysp copy.
*/
#pragma once

namespace earth {

class Overdrive {
public:
    void init();
    float process(float in);
    void setDrive(float drive);

private:
    float drive_ = 0.0f;
    float preGain_ = 0.0f;
    float postGain_ = 0.0f;
};

} // namespace earth
