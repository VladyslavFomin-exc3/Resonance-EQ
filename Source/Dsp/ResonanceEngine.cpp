#include "ResonanceEngine.h"

#include "Utilities.h"

#include <cmath>

namespace
{
constexpr float minFreq = 20.0f;
constexpr float maxFreq = 20000.0f;
constexpr float minQ = 0.9f;
constexpr float maxQ = 30.0f;
constexpr float minGainDb = 0.0f;
constexpr float maxGainDbAbsolute = 18.0f;
} // namespace

void ResonanceEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto i = 0; i < maxResonances; ++i)
    {
        freqSmoothed[i].reset(sampleRate, 0.16);
        gainDbSmoothed[i].reset(sampleRate, 0.12);
        qSmoothed[i].reset(sampleRate, 0.14);

        freqSmoothed[i].setCurrentAndTargetValue(300.0f + 30.0f * static_cast<float>(i));
        gainDbSmoothed[i].setCurrentAndTargetValue(0.0f);
        qSmoothed[i].setCurrentAndTargetValue(1.0f);
        lastFreqHz[i] = 300.0f + 30.0f * static_cast<float>(i);

        filters[i].prepare(spec);
        *filters[i].state =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 1.0f, 1.0f);
    }

    samplesUntilTick = 0;
    publishVisualState();
}

void ResonanceEngine::reset()
{
    for (auto& filter : filters)
        filter.reset();

    initialized.fill(false);
    samplesUntilTick = 0;
    publishVisualState();
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

void ResonanceEngine::forceRegenerateTargets()
{
    initialized.fill(false);
    triggerNewTargets();

    const auto rateHz = juce::jlimit(0.05f, 20.0f, computeRateHz());
    samplesUntilTick = juce::jmax(1, static_cast<int>(std::round(sampleRate / rateHz)));
    publishVisualState();
}

void ResonanceEngine::setParameters(const Params& newParams)
{
    params = newParams;

    shapedRandomness =
        hreq::util::smoothstep(hreq::util::clampFloat(params.randomness, 0.0f, 1.0f));

    const auto countMaxClamped = hreq::util::clampInt(params.countMax, 1, maxResonances);
    effectiveCount = countMaxClamped;

    effectiveQ = hreq::util::clampFloat(
        hreq::util::logLerp(minQ, hreq::util::clampFloat(params.qMax, minQ, maxQ),
                            shapedRandomness),
        minQ, maxQ);
    effectiveMotion = hreq::util::clampFloat(
        hreq::util::lerp(0.0f, hreq::util::clampFloat(params.motionMax, 0.0f, 1.0f),
                         shapedRandomness),
        0.0f, 1.0f);
}

void ResonanceEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    updateControlTicks(numSamples);
    updateCoefficients(numSamples);
    publishVisualState();

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
    const auto normalBoostDb = hreq::util::lerp(8.0f, 12.0f, shapedRandomness);
    const auto extremeBoostDb = hreq::util::lerp(12.0f, 18.0f, juce::jlimit(0.0f, 1.0f, (shapedRandomness - 0.65f) / 0.35f));
    const auto maxBoostDb = shapedRandomness > 0.65f ? extremeBoostDb : normalBoostDb;
    const auto jumpOctaves =
        hreq::util::lerp(0.03f, 2.8f, shapedRandomness) * juce::jmax(0.05f, effectiveMotion);
    const auto jumpChance = hreq::util::lerp(0.02f, 0.36f, shapedRandomness) * juce::jmax(0.1f, effectiveMotion);

    const bool makeHarmonics = nextRandom01() < 0.38f;
    const bool makeCluster = nextRandom01() < 0.32f;
    const float fundamental = hreq::util::logLerp(80.0f, 600.0f, nextRandom01());
    const float clusterCentre = randomLogFrequency(180.0f, 8000.0f);
    int generated = 0;

    for (auto i = 0; i < maxResonances; ++i)
    {
        if (i >= effectiveCount)
        {
            gainDbSmoothed[i].setTargetValue(0.0f);
            qSmoothed[i].setTargetValue(1.0f);
            visualActive[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
            continue;
        }

        float frequency = lastFreqHz[i];

        if (!initialized[i])
        {
            bool assigned = false;

            if (makeHarmonics && generated < 4)
            {
                const auto partial = generated + 1;
                const auto harmonic = fundamental * static_cast<float>(partial);
                if (harmonic <= maxFreq)
                {
                    frequency = harmonic * hreq::util::lerp(0.985f, 1.015f, nextRandom01());
                    assigned = true;
                }
            }

            if (!assigned && makeCluster && generated >= 4 && generated < 8)
            {
                const auto spread = hreq::util::lerp(-0.16f, 0.16f, nextRandom01());
                frequency = clusterCentre * std::pow(2.0f, spread);
                assigned = true;
            }

            for (int attempt = 0; !assigned && attempt < 18; ++attempt)
            {
                const auto candidate = randomLogFrequency();
                if (isFarEnoughFromExisting(candidate, i, 0.035f))
                {
                    frequency = candidate;
                    assigned = true;
                }
            }

            if (!assigned)
                frequency = randomLogFrequency();

            initialized[i] = true;
        }
        else if (effectiveMotion > 0.0001f && nextRandom01() < jumpChance)
        {
            const auto octaveOffset = hreq::util::lerp(-jumpOctaves, jumpOctaves, nextRandom01());
            frequency *= std::pow(2.0f, octaveOffset);
        }
        else if (effectiveMotion > 0.0001f)
        {
            const auto drift = motionDirection[i] * motionSpeed[i] * hreq::util::lerp(0.15f, 1.0f, shapedRandomness);
            frequency *= std::pow(2.0f, drift);
        }

        frequency = hreq::util::clampFloat(frequency, minFreq, maxFreq);
        lastFreqHz[i] = frequency;

        auto localMaxBoost = juce::jmin(maxBoostDb, maxGainDbAbsolute);

        if (frequency > 2000.0f && frequency < 6000.0f)
            localMaxBoost *= 0.72f;

        const auto rank = (float)i / (float)juce::jmax(1, effectiveCount - 1);
        float gainFloor = 0.08f;
        float gainCeiling = 0.38f;
        if (i < 3)
        {
            gainFloor = 0.72f;
            gainCeiling = 1.0f;
        }
        else if (rank < 0.55f)
        {
            gainFloor = 0.34f;
            gainCeiling = 0.72f;
        }

        const auto intensity = hreq::util::lerp(gainFloor, gainCeiling, std::pow(nextRandom01(), 0.65f));
        const auto gainMotion = hreq::util::lerp(-3.5f, 3.5f, nextRandom01()) * effectiveMotion * shapedRandomness;
        const auto gainTarget =
            hreq::util::clampFloat(localMaxBoost * intensity + gainMotion * gainDirection[i], minGainDb, maxGainDbAbsolute);
        const auto qVariation = hreq::util::lerp(0.65f, 1.55f, nextRandom01() * shapedRandomness + 0.2f);
        const auto qMotion = 1.0f + qDirection[i] * effectiveMotion * shapedRandomness * hreq::util::lerp(0.0f, 0.25f, nextRandom01());

        freqSmoothed[i].setTargetValue(frequency);
        gainDbSmoothed[i].setTargetValue(gainTarget);
        qSmoothed[i].setTargetValue(hreq::util::clampFloat(effectiveQ * qVariation * qMotion, minQ, maxQ));

        motionDirection[i] = nextRandom01() < 0.5f ? -1.0f : 1.0f;
        gainDirection[i] = nextRandom01() < 0.5f ? -1.0f : 1.0f;
        qDirection[i] = nextRandom01() < 0.5f ? -1.0f : 1.0f;
        motionSpeed[i] = hreq::util::lerp(0.002f, 0.028f, nextRandom01());
        ++generated;
    }
}

void ResonanceEngine::updateCoefficients(const int numSamples)
{
    for (auto i = 0; i < maxResonances; ++i)
    {
        const auto frequency =
            hreq::util::clampFloat(freqSmoothed[i].skip(numSamples), minFreq, maxFreq);
        const auto gainDb = hreq::util::clampFloat(gainDbSmoothed[i].skip(numSamples), minGainDb,
                                                   maxGainDbAbsolute);
        const auto qValue = hreq::util::clampFloat(qSmoothed[i].skip(numSamples), minQ, maxQ);

        *filters[i].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, frequency, qValue, juce::Decibels::decibelsToGain(gainDb));
    }
}

void ResonanceEngine::publishVisualState() noexcept
{
    for (auto i = 0; i < maxResonances; ++i)
    {
        visualFrequencyHz[static_cast<size_t>(i)].store(lastFreqHz[static_cast<size_t>(i)], std::memory_order_relaxed);
        visualGainDb[static_cast<size_t>(i)].store(gainDbSmoothed[static_cast<size_t>(i)].getTargetValue(), std::memory_order_relaxed);
        visualQ[static_cast<size_t>(i)].store(qSmoothed[static_cast<size_t>(i)].getTargetValue(), std::memory_order_relaxed);
        visualActive[static_cast<size_t>(i)].store(i < effectiveCount, std::memory_order_relaxed);
    }
}

std::array<ResonanceEngine::ResonanceVisualState, ResonanceEngine::maxResonances>
ResonanceEngine::getVisualState() const noexcept
{
    std::array<ResonanceVisualState, maxResonances> result{};

    for (auto i = 0; i < maxResonances; ++i)
    {
        result[static_cast<size_t>(i)].frequencyHz = visualFrequencyHz[static_cast<size_t>(i)].load(std::memory_order_relaxed);
        result[static_cast<size_t>(i)].gainDb = visualGainDb[static_cast<size_t>(i)].load(std::memory_order_relaxed);
        result[static_cast<size_t>(i)].q = visualQ[static_cast<size_t>(i)].load(std::memory_order_relaxed);
        result[static_cast<size_t>(i)].active = visualActive[static_cast<size_t>(i)].load(std::memory_order_relaxed);
    }

    return result;
}

float ResonanceEngine::computeRateHz() const
{
    if (params.rateMode == 0)
    {
        const auto baseSeconds =
            noteToSeconds(params.syncNote, hreq::util::clampFloat(params.bpm, 20.0f, 300.0f));
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
    case 0:
        ratio = 4.0f;
        break; // 1/1
    case 1:
        ratio = 2.0f;
        break; // 1/2
    case 2:
        ratio = 1.0f;
        break; // 1/4
    case 3:
        ratio = 0.5f;
        break; // 1/8
    case 4:
        ratio = 0.25f;
        break; // 1/16
    default:
        break;
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

float ResonanceEngine::randomLogFrequency(float minHz, float maxHz)
{
    return hreq::util::logLerp(juce::jlimit(minFreq, maxFreq, minHz),
                               juce::jlimit(minFreq, maxFreq, maxHz),
                               nextRandom01());
}

bool ResonanceEngine::isFarEnoughFromExisting(const float frequency, const int count, const float minLogDistance) const noexcept
{
    const auto logFrequency = std::log10(juce::jlimit(minFreq, maxFreq, frequency));

    for (auto i = 0; i < count; ++i)
    {
        const auto other = std::log10(juce::jlimit(minFreq, maxFreq, lastFreqHz[static_cast<size_t>(i)]));
        if (std::abs(logFrequency - other) < minLogDistance)
            return false;
    }

    return true;
}
