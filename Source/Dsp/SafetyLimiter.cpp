#include "SafetyLimiter.h"

void SafetyLimiter::prepare(const juce::dsp::ProcessSpec& spec)
{
    limiter.prepare(spec);
    limiter.setThreshold(-0.3f);
    limiter.setRelease(60.0f);
    limiter.reset();
}

void SafetyLimiter::reset()
{
    limiter.reset();
}

void SafetyLimiter::processBlock(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    limiter.process(context);
}
