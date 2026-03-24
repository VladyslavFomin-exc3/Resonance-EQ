#pragma once

#include <JuceHeader.h>

#include <array>

/**
 * @brief Multi-band EQ curve controller.
 *
 * Manages per-band IIR filters, smoothing of frequency/gain/Q targets, and
 * real-time processing of audio blocks.
 */
class EqCurve
{
  public:
    static constexpr int numBands = 7;

    /**
     * @brief Initialize internal DSP components.
     * @param spec Process specification (sample rate, block size, channels).
     */
    void prepare(const juce::dsp::ProcessSpec& spec);

    /**
     * @brief Reset internal state and smoothing filters.
     */
    void reset();

    /**
     * @brief Set frequency/gain/Q target for a specific band.
     * @param bandIndex Band index in [0, numBands).
     * @param frequencyHz Target center frequency.
     * @param gainDb Target gain in dB.
     * @param qValue Target filter Q factor.
     *
     * @note Values are smoothed over time in processBlock.
     */
    void setBandTarget(const int bandIndex, const float frequencyHz, const float gainDb,
                       const float qValue);

    /**
     * @brief Process one audio buffer through all EQ bands.
     * @param buffer Audio buffer to process in-place.
     */
    void processBlock(juce::AudioBuffer<float>& buffer);

  private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    /** @brief Internal coefficient updater called at process time. */
    void updateCoefficients(int numSamples);

    double sampleRate = 44100.0;

    std::array<Filter, numBands> filters;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, numBands>
        freqSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, numBands>
        gainDbSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, numBands>
        qSmoothed;
};
