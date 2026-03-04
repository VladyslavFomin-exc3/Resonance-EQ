#pragma once

#include <JuceHeader.h>

#include <array>

class EqCurve
{
public:
    static constexpr int numBands = 7;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBandTarget(int bandIndex, float frequencyHz, float gainDb, float qValue);
    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    void updateCoefficients(int numSamples);

    double sampleRate = 44100.0;

    std::array<Filter, numBands> filters;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, numBands> freqSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, numBands> gainDbSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, numBands> qSmoothed;
};
