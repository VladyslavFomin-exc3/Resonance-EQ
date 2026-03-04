#include "EqCurve.h"

#include "Utilities.h"

namespace
{
constexpr float minFreq = 20.0f;
constexpr float maxFreq = 20000.0f;
constexpr float minQ = 0.3f;
constexpr float maxQ = 6.0f;
constexpr float minGainDb = -12.0f;
constexpr float maxGainDb = 12.0f;
}

void EqCurve::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto band = 0; band < numBands; ++band)
    {
        freqSmoothed[band].reset(sampleRate, 0.12);
        gainDbSmoothed[band].reset(sampleRate, 0.12);
        qSmoothed[band].reset(sampleRate, 0.12);

        freqSmoothed[band].setCurrentAndTargetValue(1000.0f);
        gainDbSmoothed[band].setCurrentAndTargetValue(0.0f);
        qSmoothed[band].setCurrentAndTargetValue(1.0f);

        filters[band].prepare(spec);
        *filters[band].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 1.0f, 1.0f);
    }
}

void EqCurve::reset()
{
    for (auto& filter : filters)
        filter.reset();
}

void EqCurve::setBandTarget(const int bandIndex, const float frequencyHz, const float gainDb, const float qValue)
{
    if (! juce::isPositiveAndBelow(bandIndex, numBands))
        return;

    freqSmoothed[bandIndex].setTargetValue(hreq::util::clampFloat(hreq::util::sanitize(frequencyHz, 1000.0f), minFreq, maxFreq));
    gainDbSmoothed[bandIndex].setTargetValue(hreq::util::clampFloat(hreq::util::sanitize(gainDb, 0.0f), minGainDb, maxGainDb));
    qSmoothed[bandIndex].setTargetValue(hreq::util::clampFloat(hreq::util::sanitize(qValue, 1.0f), minQ, maxQ));
}

void EqCurve::processBlock(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    updateCoefficients(numSamples);

    juce::dsp::AudioBlock<float> block(buffer);

    for (auto& filter : filters)
    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }
}

void EqCurve::updateCoefficients(const int numSamples)
{
    for (auto band = 0; band < numBands; ++band)
    {
        const auto frequency = hreq::util::clampFloat(freqSmoothed[band].skip(numSamples), minFreq, maxFreq);
        const auto qValue = hreq::util::clampFloat(qSmoothed[band].skip(numSamples), minQ, maxQ);
        const auto gain = juce::Decibels::decibelsToGain(hreq::util::clampFloat(gainDbSmoothed[band].skip(numSamples), minGainDb, maxGainDb));

        *filters[band].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, frequency, qValue, gain);
    }
}
