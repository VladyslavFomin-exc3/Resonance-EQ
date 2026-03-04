#include "ResonanceEngine.h"

#include "Utilities.h"

#include <cmath>

namespace
{
constexpr float minFreq = 20.0f;
constexpr float maxFreq = 20000.0f;
constexpr float minQ = 0.9f;
constexpr float maxQ = 14.0f;
constexpr float minGainDb = 0.0f;
constexpr float maxGainDbAbsolute = 12.0f;
}

void ResonanceEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto i = 0; i < maxResonances; ++i)
    {
        freqSmoothed[i].reset(sampleRate, 0.22);
        gainDbSmoothed[i].reset(sampleRate, 0.18);
        qSmoothed[i].reset(sampleRate, 0.22);

        freqSmoothed[i].setCurrentAndTargetValue(300.0f + 40.0f * static_cast<float>(i));
        gainDbSmoothed[i].setCurrentAndTargetValue(0.0f);
        qSmoothed[i].setCurrentAndTargetValue(1.0f);
        lastFreqHz[i] = 300.0f + 40.0f * static_cast<float>(i);

        filters[i].prepare(spec);
        *filters[i].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 1.0f, 1.0f);
    }

    samplesUntilTick = 0;
}

void ResonanceEngine::reset()
{
    for (auto& filter : filters)
        filter.reset();

    initialized.fill(false);
    samplesUntilTick = 0;
}

void ResonanceEngine::setSeed(const int newSeed)
{
    const auto clamped = juce::jlimit(0, std::numeric_limits<int>::max(), newSeed);

    if (clamped == currentSeed)
        return;

    currentSeed = clamped;
    prng.seed(static_cast<std::mt19937::result_type>(currentSeed));
    initialized.fill(false);
    samplesUntilTick = 0;
}

void ResonanceEngine::setParameters(const Params& newParams)
{
    params = newParams;

    shapedRandomness = hreq::util::smoothstep(hreq::util::clampFloat(params.randomness, 0.0f, 1.0f));

    const auto countMaxClamped = hreq::util::clampInt(params.countMax, 1, maxResonances);
    const auto countMinFixed = (countMaxClamped < 2 ? 1 : 2);
    effectiveCount = juce::jlimit(1, maxResonances,
                                  static_cast<int>(std::round(hreq::util::lerp(static_cast<float>(countMinFixed), static_cast<float>(countMaxClamped), shapedRandomness))));

    effectiveQ = hreq::util::clampFloat(hreq::util::logLerp(minQ, hreq::util::clampFloat(params.qMax, minQ, maxQ), shapedRandomness), minQ, maxQ);
    effectiveMotion = hreq::util::clampFloat(hreq::util::lerp(0.0f, hreq::util::clampFloat(params.motionMax, 0.0f, 1.0f), shapedRandomness), 0.0f, 1.0f);
}

void ResonanceEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    updateControlTicks(numSamples);
    updateCoefficients(numSamples);

    juce::dsp::AudioBlock<float> block(buffer);
    for (auto& filter : filters)
    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }
}

void ResonanceEngine::updateControlTicks(const int numSamples)
{
    auto samplesLeft = numSamples;

    while (samplesLeft > 0)
    {
        if (samplesUntilTick <= 0)
        {
            triggerNewTargets();
            const auto rateHz = juce::jlimit(0.05f, 20.0f, computeRateHz());
            samplesUntilTick = juce::jmax(1, static_cast<int>(std::round(sampleRate / rateHz)));
        }

        const auto step = juce::jmin(samplesLeft, samplesUntilTick);
        samplesLeft -= step;
        samplesUntilTick -= step;
    }
}

void ResonanceEngine::triggerNewTargets()
{
    const auto maxBoostDb = juce::jlimit(3.0f, 9.0f, hreq::util::lerp(3.0f, 9.0f, shapedRandomness));
    const auto jumpOctaves = hreq::util::lerp(0.06f, 2.3f, shapedRandomness) * juce::jmax(0.1f, effectiveMotion);

    for (auto i = 0; i < maxResonances; ++i)
    {
        if (i >= effectiveCount)
        {
            gainDbSmoothed[i].setTargetValue(0.0f);
            qSmoothed[i].setTargetValue(1.0f);
            continue;
        }

        float frequency = lastFreqHz[i];

        if (! initialized[i])
        {
            frequency = hreq::util::logLerp(minFreq, maxFreq, nextRandom01());
            initialized[i] = true;
        }
        else if (effectiveMotion > 0.0001f)
        {
            const auto octaveOffset = hreq::util::lerp(-jumpOctaves, jumpOctaves, nextRandom01());
            frequency *= std::pow(2.0f, octaveOffset);
        }

        frequency = hreq::util::clampFloat(frequency, minFreq, maxFreq);
        lastFreqHz[i] = frequency;

        auto localMaxBoost = juce::jmin(maxBoostDb, maxGainDbAbsolute);

        if (frequency > 2000.0f && frequency < 6000.0f)
            localMaxBoost *= 0.6f;

        const auto gainTarget = hreq::util::clampFloat(localMaxBoost * nextRandom01(), minGainDb, maxGainDbAbsolute);
        const auto qVariation = hreq::util::lerp(0.9f, 1.15f, nextRandom01());

        freqSmoothed[i].setTargetValue(frequency);
        gainDbSmoothed[i].setTargetValue(gainTarget);
        qSmoothed[i].setTargetValue(hreq::util::clampFloat(effectiveQ * qVariation, minQ, maxQ));
    }
}

void ResonanceEngine::updateCoefficients(const int numSamples)
{
    for (auto i = 0; i < maxResonances; ++i)
    {
        const auto frequency = hreq::util::clampFloat(freqSmoothed[i].skip(numSamples), minFreq, maxFreq);
        const auto gainDb = hreq::util::clampFloat(gainDbSmoothed[i].skip(numSamples), minGainDb, maxGainDbAbsolute);
        const auto qValue = hreq::util::clampFloat(qSmoothed[i].skip(numSamples), minQ, maxQ);

        *filters[i].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, frequency, qValue, juce::Decibels::decibelsToGain(gainDb));
    }
}

float ResonanceEngine::computeRateHz() const
{
    if (params.rateMode == 0)
    {
        const auto baseSeconds = noteToSeconds(params.syncNote, hreq::util::clampFloat(params.bpm, 20.0f, 300.0f));
        const auto minSeconds = juce::jmax(0.02f, baseSeconds * 0.25f);
        const auto maxSeconds = juce::jmax(minSeconds, baseSeconds * 4.0f);
        const auto seconds = hreq::util::lerp(maxSeconds, minSeconds, shapedRandomness);
        return 1.0f / juce::jmax(0.02f, seconds);
    }

    const auto maxFree = hreq::util::clampFloat(params.freeHzMax, 0.05f, 10.0f);
    return hreq::util::logLerp(0.05f, maxFree, shapedRandomness);
}

float ResonanceEngine::noteToSeconds(const int noteIndex, const float bpm) const
{
    const auto quarter = 60.0f / juce::jmax(1.0f, bpm);

    float ratio = 1.0f;
    switch (noteIndex)
    {
        case 0: ratio = 4.0f; break; // 1/1
        case 1: ratio = 2.0f; break; // 1/2
        case 2: ratio = 1.0f; break; // 1/4
        case 3: ratio = 0.5f; break; // 1/8
        case 4: ratio = 0.25f; break; // 1/16
        default: break;
    }

    float seconds = quarter * ratio;

    if (params.syncDotted)
        seconds *= 1.5f;

    if (params.syncTriplet)
        seconds *= (2.0f / 3.0f);

    return juce::jmax(0.02f, seconds);
}

float ResonanceEngine::nextRandom01()
{
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(prng);
}
