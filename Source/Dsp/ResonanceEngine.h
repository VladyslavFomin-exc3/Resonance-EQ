#pragma once

#include <JuceHeader.h>

#include <array>
#include <random>

class ResonanceEngine
{
public:
    static constexpr int maxResonances = 12;

    struct Params
    {
        float randomness = 0.25f;
        int countMax = 8;
        float qMax = 10.0f;
        float motionMax = 0.6f;
        int rateMode = 0;
        int syncNote = 2;
        bool syncDotted = false;
        bool syncTriplet = false;
        float freeHzMax = 2.5f;
        float bpm = 120.0f;
    };

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setSeed(int newSeed);
    void setParameters(const Params& newParams);
    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    void updateControlTicks(int numSamples);
    void triggerNewTargets();
    void updateCoefficients(int numSamples);

    float computeRateHz() const;
    float noteToSeconds(int noteIndex, float bpm) const;
    float nextRandom01();

    double sampleRate = 44100.0;

    Params params;

    int currentSeed = 12345;
    std::mt19937 prng { static_cast<std::mt19937::result_type> (currentSeed) };

    int samplesUntilTick = 0;
    int effectiveCount = 2;
    float effectiveQ = 1.0f;
    float effectiveMotion = 0.0f;
    float shapedRandomness = 0.0f;

    std::array<bool, maxResonances> initialized {};
    std::array<float, maxResonances> lastFreqHz {};

    std::array<Filter, maxResonances> filters;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, maxResonances> freqSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, maxResonances> gainDbSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, maxResonances> qSmoothed;
};
