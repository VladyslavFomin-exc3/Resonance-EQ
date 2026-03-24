#pragma once

#include <JuceHeader.h>

class SafetyLimiter
{
  public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer);

  private:
    juce::dsp::Limiter<float> limiter;
};
