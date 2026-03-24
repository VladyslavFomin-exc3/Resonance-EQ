#pragma once

#include <JuceHeader.h>

/**
 * @brief Soft-limiter module to keep output levels safe and avoid clipping.
 *
 * This is the final stage of the DSP chain before output.
 */
class SafetyLimiter
{
  public:
    /**
     * @brief Initialize limiter with the host spec.
     * @param spec Audio process specification.
     */
    void prepare(const juce::dsp::ProcessSpec& spec);

    /** @brief Reset limiter state (clear any attack/release footprint). */
    void reset();

    /**
     * @brief Process audio with limiter.
     * @param buffer Audio buffer to process in-place.
     */
    void processBlock(juce::AudioBuffer<float>& buffer);

  private:
    juce::dsp::Limiter<float> limiter;
};
