#pragma once

#include <JuceHeader.h>

#include <array>
#include <random>

/**
 * @brief Randomized resonance engine for dynamic spectral movement.
 *
 * Generates time-varying resonances controlled by BPM sync, randomness, and
 * frequency/Q parameters. Applied as a second stage in the plugin chain.
 */
class ResonanceEngine
{
  public:
    static constexpr int maxResonances = 24;

    struct ResonanceVisualState
    {
        float frequencyHz = 20.0f;
        float gainDb = 0.0f;
        float q = 1.0f;
        bool active = false;
    };

    /**
     * @brief Container for real-time resonance control settings.
     */
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

    /**
     * @brief Initialize resonance filters and smoothing states.
     * @param spec Process spec containing sample rate and channel count.
     */
    void prepare(const juce::dsp::ProcessSpec& spec);

    /** @brief Reset state and parameter smoothing for a new playback run. */
    void reset();

    /**
     * @brief Set the pseudo-random generator seed for deterministic behavior.
     * @param newSeed New seed value.
     */
    void setSeed(const int newSeed);

    /** @brief Immediately generate a fresh set of resonance targets from the current seed and params. */
    void forceRegenerateTargets();

    /**
     * @brief Update runtime parameters in the engine.
     * @param newParams Resonance parameters structure.
     */
    void setParameters(const Params& newParams);

    /**
     * @brief Process audio buffer applying resonance filters.
     * @param buffer Audio buffer to process in-place.
     */
    void processBlock(juce::AudioBuffer<float>& buffer);

    std::array<ResonanceVisualState, maxResonances> getVisualState() const noexcept;

  private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    // Control engine internals
    void updateControlTicks(int numSamples);
    void triggerNewTargets();
    void updateCoefficients(int numSamples);
    void publishVisualState() noexcept;

    float computeRateHz() const;
    float noteToSeconds(int noteIndex, float bpm) const;
    float nextRandom01();
    float randomLogFrequency(float minHz = 20.0f, float maxHz = 20000.0f);
    bool isFarEnoughFromExisting(float frequency, int count, float minLogDistance) const noexcept;

    double sampleRate = 44100.0;

    Params params;

    int currentSeed = 12345;
    std::mt19937 prng{static_cast<std::mt19937::result_type>(currentSeed)};

    int samplesUntilTick = 0;
    int effectiveCount = 2;
    float effectiveQ = 1.0f;
    float effectiveMotion = 0.0f;
    float shapedRandomness = 0.0f;

    std::array<bool, maxResonances> initialized{};
    std::array<float, maxResonances> lastFreqHz{};
    std::array<float, maxResonances> motionDirection{};
    std::array<float, maxResonances> gainDirection{};
    std::array<float, maxResonances> qDirection{};
    std::array<float, maxResonances> motionSpeed{};

    std::array<std::atomic<float>, maxResonances> visualFrequencyHz{};
    std::array<std::atomic<float>, maxResonances> visualGainDb{};
    std::array<std::atomic<float>, maxResonances> visualQ{};
    std::array<std::atomic<bool>, maxResonances> visualActive{};

    std::array<Filter, maxResonances> filters;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, maxResonances>
        freqSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, maxResonances>
        gainDbSmoothed;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, maxResonances>
        qSmoothed;
};
