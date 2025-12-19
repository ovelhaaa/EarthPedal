#include "Dattorro.hpp"
#include <algorithm>

Dattorro1997Tank::Dattorro1997Tank(const float initSampleRate,
                                   const float initMaxLfoDepth,
                                   const float initMaxTimeScale) :
    maxTimeScale(initMaxTimeScale) 
{
    timePadding = initMaxLfoDepth;
    // We can't setSampleRate properly until buffers are Init'd,
    // but setSampleRate calls initialiseDelaysAndApfs which we are neutering.
    // So it might be safe.

    // setSampleRate(initSampleRate); // Moved to Init or handled carefully

    // We will call setSampleRate inside Init after buffers are set.
    sampleRate = initSampleRate; // Just set variable for now.

    leftOutDCBlock.setCutoffFreq(20.0);
    rightOutDCBlock.setCutoffFreq(20.0);

    // Equivalent to the same cutoff in the exponential function
    leftOutDCBlock.setCutoffFreq(-0.004);
    rightOutDCBlock.setCutoffFreq(-0.004);

    lfo1.setFrequency(lfo1Freq);
    lfo2.setFrequency(lfo2Freq);
    lfo3.setFrequency(lfo3Freq);
    lfo4.setFrequency(lfo4Freq);

    lfo2.phase = 0.25;
    lfo3.phase = 0.5;
    lfo4.phase = 0.75;

    lfo1.setRevPoint(0.5);
    lfo2.setRevPoint(0.5);
    lfo3.setRevPoint(0.5);
    lfo4.setRevPoint(0.5);
}

void Dattorro1997Tank::Init(const DattorroTankBuffers& buffers) {
    leftApf1.Init(buffers.leftApf1, buffers.leftApf1Size);
    leftDelay1.Init(buffers.leftDelay1, buffers.leftDelay1Size);
    leftApf2.Init(buffers.leftApf2, buffers.leftApf2Size);
    leftDelay2.Init(buffers.leftDelay2, buffers.leftDelay2Size);

    rightApf1.Init(buffers.rightApf1, buffers.rightApf1Size);
    rightDelay1.Init(buffers.rightDelay1, buffers.rightDelay1Size);
    rightApf2.Init(buffers.rightApf2, buffers.rightApf2Size);
    rightDelay2.Init(buffers.rightDelay2, buffers.rightDelay2Size);

    setSampleRate(sampleRate);
}

void Dattorro1997Tank::process(const float leftIn, const float rightIn,
                               float* leftOut, float* rightOut) {
    tickApfModulation();

    decay = decayParam;

    leftSum += leftIn;
    rightSum += rightIn;

    leftApf1.input = leftSum;
    leftDelay1.input = leftApf1.process();
    leftDelay1.process();
    leftHighCutFilter.input = leftDelay1.output;
    leftLowCutFilter.input = leftHighCutFilter.process();
    leftApf2.input = (leftDelay1.output * (1. - fade) + leftLowCutFilter.process() * fade) * decay;
    leftDelay2.input = leftApf2.process();
    leftDelay2.process();

    rightApf1.input = rightSum;
    rightDelay1.input = rightApf1.process();
    rightDelay1.process();
    rightHighCutFilter.input = rightDelay1.output;
    rightLowCutFilter.input =  rightHighCutFilter.process();
    rightApf2.input = (rightDelay1.output * (1. - fade) + rightLowCutFilter.process() * fade) * decay;
    rightDelay2.input = rightApf2.process();
    rightDelay2.process();

    rightSum = leftDelay2.output * decay;
    leftSum = rightDelay2.output * decay;

    leftOutDCBlock.input = leftApf1.output;
    leftOutDCBlock.input += leftDelay1.tap(scaledOutputTaps[L_DELAY_1_L_TAP_1]);
    leftOutDCBlock.input += leftDelay1.tap(scaledOutputTaps[L_DELAY_1_L_TAP_2]);
    leftOutDCBlock.input -= leftApf2.delay.tap(scaledOutputTaps[L_APF_2_L_TAP]);
    leftOutDCBlock.input += leftDelay2.tap(scaledOutputTaps[L_DELAY_2_L_TAP]);
    leftOutDCBlock.input -= rightDelay1.tap(scaledOutputTaps[R_DELAY_1_L_TAP]);
    leftOutDCBlock.input -= rightApf2.delay.tap(scaledOutputTaps[R_APF_2_L_TAP]);
    leftOutDCBlock.input -= rightDelay2.tap(scaledOutputTaps[R_DELAY_2_L_TAP]);

    rightOutDCBlock.input = rightApf1.output;
    rightOutDCBlock.input += rightDelay1.tap(scaledOutputTaps[R_DELAY_1_R_TAP_1]);
    rightOutDCBlock.input += rightDelay1.tap(scaledOutputTaps[R_DELAY_1_R_TAP_2]);
    rightOutDCBlock.input -= rightApf2.delay.tap(scaledOutputTaps[R_APF_2_R_TAP]);
    rightOutDCBlock.input += rightDelay2.tap(scaledOutputTaps[R_DELAY_2_R_TAP]);
    rightOutDCBlock.input -= leftDelay1.tap(scaledOutputTaps[L_DELAY_1_R_TAP]);
    rightOutDCBlock.input -= leftApf2.delay.tap(scaledOutputTaps[L_APF_2_R_TAP]);
    rightOutDCBlock.input -= leftDelay2.tap(scaledOutputTaps[L_DELAY_2_R_TAP]);

    *leftOut = leftOutDCBlock.process() * 0.5;
    *rightOut = rightOutDCBlock.process() * 0.5;

    fade += fadeStep * fadeDir;
    fade = (fade < 0.) ? 0. : ((fade > 1.) ? 1. : fade);
}

void Dattorro1997Tank::freeze(bool freezeFlag) {
    frozen = freezeFlag;
    if (frozen) {
        fadeDir = -1.;
    }
    else {
        fadeDir = 1.;
    }
}

void Dattorro1997Tank::setSampleRate(const float newSampleRate) {
    sampleRate = newSampleRate;
    sampleRate = sampleRate > maxSampleRate ? maxSampleRate : sampleRate;
    sampleRate = sampleRate < 1. ? 1. : sampleRate;
    sampleRateScale = sampleRate / dattorroSampleRate;

    fadeStep = 1. / sampleRate;

    leftOutDCBlock.setSampleRate(sampleRate);
    rightOutDCBlock.setSampleRate(sampleRate);

    rescaleTapTimes();
    setTimeScale(timeScale);
    initialiseDelaysAndApfs();
}

#pragma GCC push_options
#pragma GCC optimize ("Ofast")

void Dattorro1997Tank::setTimeScale(const float newTimeScale) {
    timeScale = newTimeScale < 0.0001 ? 0.0001 : newTimeScale;
    rescaleApfAndDelayTimes();
}

#pragma GCC pop_options

void Dattorro1997Tank::setDecay(const float newDecay) {
    decayParam = (float)(newDecay > 1. ? 1. :
                         (newDecay < 0. ? 0. : newDecay));
}

void Dattorro1997Tank::setModSpeed(const float newModSpeed) {
    lfo1.setFrequency(lfo1Freq * newModSpeed);
    lfo2.setFrequency(lfo2Freq * newModSpeed);
    lfo3.setFrequency(lfo3Freq * newModSpeed);
    lfo4.setFrequency(lfo4Freq * newModSpeed);
}

void Dattorro1997Tank::setModDepth(const float newModDepth) {
    modDepth = newModDepth;
    lfoExcursion = newModDepth * lfoMaxExcursion * sampleRateScale;
}

void Dattorro1997Tank::setModShape(const float shape) {
    lfo1.setRevPoint(shape);
    lfo2.setRevPoint(shape);
    lfo3.setRevPoint(shape);
    lfo4.setRevPoint(shape);
}

void Dattorro1997Tank::setHighCutFrequency(const float frequency) {
    leftHighCutFilter.setCutoffFreq(frequency);
    rightHighCutFilter.setCutoffFreq(frequency);
}

void Dattorro1997Tank::setLowCutFrequency(const float frequency) {
    leftLowCutFilter.setCutoffFreq(frequency);
    rightLowCutFilter.setCutoffFreq(frequency);
}


void Dattorro1997Tank::setDiffusion(const float diffusion) {
    leftApf1.setGain(-diffusion);
    leftApf2.setGain(diffusion);
    rightApf1.setGain(-diffusion);
    rightApf2.setGain(diffusion);
}

void Dattorro1997Tank::clear() {
    leftApf1.clear();
    leftDelay1.clear();
    leftHighCutFilter.clear();
    leftLowCutFilter.clear();
    leftApf2.clear();
    leftDelay2.clear();

    rightApf1.clear();
    rightDelay1.clear();
    rightHighCutFilter.clear();;
    rightLowCutFilter.clear();
    rightApf2.clear();
    rightDelay2.clear();

    leftOutDCBlock.clear();
    rightOutDCBlock.clear();

    leftSum = 0.;
    rightSum = 0.;
}

int maxScaledOutputTap = 0;

inline int Dattorro1997Tank::calcMaxTime(float delayTime) {
    maxScaledOutputTap = *std::max_element(scaledOutputTaps.begin(),
                                                scaledOutputTaps.end());

    return (int)(sampleRateScale * (delayTime * maxTimeScale + 
                                         maxScaledOutputTap + timePadding));
}

void Dattorro1997Tank::initialiseDelaysAndApfs() {
    // This function used to re-allocate / re-construct objects.
    // Now we assume Init has provided large enough buffers.
    // We do nothing here, or we could verify sizes if we had access to buffer sizes in InterpDelay (we do: l).

    // Actually, we should probably call setDelayTime here?
    // No, setDelayTime is called in rescaleApfAndDelayTimes which is called by setSampleRate/setTimeScale.

    // So this function is now effectively empty or just a placeholder.
    // The APF sizes are fixed by Init.
    // The delay lines sizes are fixed by Init.
}

void Dattorro1997Tank::tickApfModulation() {
    leftApf1.delay.setDelayTime(lfo1.process() * lfoExcursion + scaledLeftApf1Time);
    leftApf2.delay.setDelayTime(lfo2.process() * lfoExcursion + scaledLeftApf2Time);
    rightApf1.delay.setDelayTime(lfo3.process() * lfoExcursion + scaledRightApf1Time);
    rightApf2.delay.setDelayTime(lfo4.process() * lfoExcursion + scaledRightApf2Time);
}

float scaleFactor = 0.;


#pragma GCC push_options
#pragma GCC optimize ("Ofast")

void Dattorro1997Tank::rescaleApfAndDelayTimes() {
    scaleFactor = timeScale * sampleRateScale;

    scaledLeftApf1Time = leftApf1Time * scaleFactor;
    scaledLeftDelay1Time = leftDelay1Time * scaleFactor;
    scaledLeftApf2Time = leftApf2Time * scaleFactor;
    scaledLeftDelay2Time = leftDelay2Time * scaleFactor;

    scaledRightApf1Time = rightApf1Time * scaleFactor;
    scaledRightDelay1Time = rightDelay1Time * scaleFactor;
    scaledRightApf2Time = rightApf2Time * scaleFactor;
    scaledRightDelay2Time = rightDelay2Time * scaleFactor;

    leftDelay1.setDelayTime(scaledLeftDelay1Time);
    leftDelay2.setDelayTime(scaledLeftDelay2Time);
    rightDelay1.setDelayTime(scaledRightDelay1Time);
    rightDelay2.setDelayTime(scaledRightDelay2Time);
}

#pragma GCC pop_options

void Dattorro1997Tank::rescaleTapTimes() {
    for (size_t i = 0; i < scaledOutputTaps.size(); ++i) {
        scaledOutputTaps[i] = (int)((float)kOutputTaps[i] * sampleRateScale);
    }
}

Dattorro::Dattorro(const float initMaxSampleRate,
                   const float initMaxLfoDepth,
                   const float initMaxTimeScale)
    : tank(initMaxSampleRate, initMaxLfoDepth, initMaxTimeScale)
{
    sampleRate = initMaxSampleRate;
    dattorroScaleFactor = sampleRate / dattorroSampleRate;

    // Buffers are not set yet.

    inputLpf = OnePoleLPFilter(22000.0);
    inputHpf = OnePoleHPFilter(0.0);

    // Initial gains/diffusion
    inApf1.setGain(inputDiffusion1);
    inApf2.setGain(inputDiffusion1);
    inApf3.setGain(inputDiffusion2);
    inApf4.setGain(inputDiffusion2);

    leftInputDCBlock.setCutoffFreq(20.0);
    rightInputDCBlock.setCutoffFreq(20.0);
}

void Dattorro::Init(const DattorroBuffers& buffers) {
    preDelay.Init(buffers.preDelay, buffers.preDelaySize);

    inApf1.Init(buffers.inApf1, buffers.inApf1Size);
    inApf2.Init(buffers.inApf2, buffers.inApf2Size);
    inApf3.Init(buffers.inApf3, buffers.inApf3Size);
    inApf4.Init(buffers.inApf4, buffers.inApf4Size);

    tank.Init(buffers.tankBuffers);

    // Now valid to set parameters that might depend on buffers (though most don't)
    setSampleRate(sampleRate);
}

//float subApfOut = 0.;

void Dattorro::process(float leftInput, float rightInput) {
    leftInputDCBlock.input = leftInput;
    rightInputDCBlock.input = rightInput;
    inputLpf.setCutoffFreq(inputHighCut);
    inputHpf.setCutoffFreq(inputLowCut);
    inputLpf.input = leftInputDCBlock.process() + rightInputDCBlock.process();
    inputHpf.input = inputLpf.process();
    inputHpf.process();
    preDelay.input = inputHpf.output;
    preDelay.process();
    inApf1.input = preDelay.output;
    inApf2.input = inApf1.process();
    inApf3.input = inApf2.process();
    inApf4.input = inApf3.process();
    tankFeed = preDelay.output * (1. - diffuseInput) + inApf4.process() * diffuseInput;

    tank.process(tankFeed, tankFeed, &leftOut, &rightOut);
}

void Dattorro::clear() {
    leftInputDCBlock.clear();
    rightInputDCBlock.clear();

    inputLpf.clear();
    inputHpf.clear();
    preDelay.clear();
    inApf1.clear();
    inApf2.clear();
    inApf3.clear();
    inApf4.clear();

    tank.clear();
}

#pragma GCC push_options
#pragma GCC optimize ("Ofast")


void Dattorro::setTimeScale(float timeScale) {
    timeScale = timeScale < 0.01 ? 0.01 : timeScale;
    tank.setTimeScale(timeScale);
}

#pragma GCC pop_options
#pragma GCC push_options
#pragma GCC optimize ("Ofast")

void Dattorro::setPreDelay(float t) {
    preDelay.setDelayTime(t * sampleRate);
}


#pragma GCC pop_options

void Dattorro::setSampleRate(float newSampleRate) {
    sampleRate = newSampleRate;
    tank.setSampleRate(sampleRate);
    dattorroScaleFactor = sampleRate / dattorroSampleRate;
    setPreDelay(preDelayTime);

    // Update APF delay times
    inApf1.delay.setDelayTime(dattorroScale(kInApf1Time));
    inApf2.delay.setDelayTime(dattorroScale(kInApf2Time));
    inApf3.delay.setDelayTime(dattorroScale(kInApf3Time));
    inApf4.delay.setDelayTime(dattorroScale(kInApf4Time));

    leftInputDCBlock.setSampleRate(sampleRate);
    rightInputDCBlock.setSampleRate(sampleRate);
    inputLpf.setSampleRate(sampleRate);
    inputHpf.setSampleRate(sampleRate);
}

void Dattorro::freeze(bool freezeFlag) {
    tank.freeze(freezeFlag);
}

#pragma GCC push_options
#pragma GCC optimize ("Ofast")

void Dattorro::setInputFilterLowCutoffPitch(float pitch) {
    inputLowCut = 440.0 * std::pow(2.0, pitch - 5.0);
}

#pragma GCC pop_options
#pragma GCC push_options
#pragma GCC optimize ("Ofast")


void Dattorro::setInputFilterHighCutoffPitch(float pitch) {
    inputHighCut = 440.0 * std::pow(2.0, pitch - 5.0);
}

#pragma GCC pop_options

void Dattorro::enableInputDiffusion(bool enable) {
    diffuseInput = enable ? 1. : 0.;
}

void Dattorro::setDecay(float newDecay) {
    decay = newDecay;
    tank.setDecay(decay);
}

void Dattorro::setTankDiffusion(const float diffusion) {
    tank.setDiffusion(diffusion);
}

#pragma GCC push_options
#pragma GCC optimize ("Ofast")


void Dattorro::setTankFilterHighCutFrequency(const float pitch) {
    auto frequency = 440.0 * std::pow(2.0, pitch - 5.0);
    tank.setHighCutFrequency(frequency);
}

#pragma GCC pop_options
#pragma GCC push_options
#pragma GCC optimize ("Ofast")


void Dattorro::setTankFilterLowCutFrequency(const float pitch) {
    auto frequency = 440.0 * std::pow(2.0, pitch - 5.0);
    tank.setLowCutFrequency(frequency);
}

#pragma GCC pop_options

void Dattorro::setTankModSpeed(const float modSpeed) {
    tank.setModSpeed(modSpeed);
}

void Dattorro::setTankModDepth(const float modDepth) {
    tank.setModDepth(modDepth);
}

void Dattorro::setTankModShape(const float modShape) {
    tank.setModShape(modShape);
}

float Dattorro::getLeftOutput() const {
    return leftOut;
}

float Dattorro::getRightOutput() const {
    return rightOut;
}

float Dattorro::dattorroScale(float delayTime) {
    return delayTime * dattorroScaleFactor;
}
