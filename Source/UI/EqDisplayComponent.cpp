#include "EqDisplayComponent.h"
#include "PluginProcessor.h"

EqDisplayComponent::EqDisplayComponent(juce::AudioProcessorValueTreeState& state,
                                       ResonanceEQAudioProcessor& processor)
    : parameters(state),
      audioProcessor(processor),
      flattenButton("Flatten"),
      fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    const auto makeId = [&](const juce::String& base, int band) {
        return base + juce::String(band + 1);
    };

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        freqValues[band] = parameters.getRawParameterValue(makeId("eq", band) + "Freq");
        gainValues[band] = parameters.getRawParameterValue(makeId("eq", band) + "Gain");
        qValues[band] = parameters.getRawParameterValue(makeId("eq", band) + "Q");
    }

    nodeColors = {juce::Colour(0xff7c4cff), juce::Colour(0xff4796ff), juce::Colour(0xff49c66a),
                  juce::Colour(0xfff4c140), juce::Colour(0xffff8b33), juce::Colour(0xff43d4dc),
                  juce::Colour(0xffd95379)};

    flattenButton.addListener(this);
    flattenButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a3343));
    flattenButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(flattenButton);

    analyzerSamples.resize(160, -90.0f);

    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    startTimerHz(30);
}

EqDisplayComponent::~EqDisplayComponent() = default;

void EqDisplayComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(16);
    graphArea = bounds;

    const auto background = juce::Colour(0xff141a24);
    g.fillAll(background);

    juce::ColourGradient gradient(background.brighter(0.08f), (float)bounds.getCentreX(), 0.0f,
                                  juce::Colour(0xff10151d), (float)bounds.getCentreX(), (float)getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colour(0x552c3b4e));
    g.fillRoundedRectangle(graphArea.toFloat(), 18.0f);

    const auto gridArea = graphArea.reduced(20, 18);
    g.setColour(juce::Colour(0x443b4d6b));

    static const std::array<float, 7> gainLines = {12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f};
    for (auto gain : gainLines)
    {
        const auto y = static_cast<int>(gainToY(gain));
        g.drawHorizontalLine(y, (float)gridArea.getX(), (float)gridArea.getRight());
    }

    static const std::array<float, 10> freqTicks = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
                                                    1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    for (auto freq : freqTicks)
    {
        const auto x = static_cast<int>(frequencyToX(freq));
        g.drawVerticalLine(x, (float)gridArea.getY(), (float)gridArea.getBottom());
    }

    juce::Path analyzerPath;
    const int sampleCount = static_cast<int>(analyzerSamples.size());
    for (int i = 0; i < sampleCount; ++i)
    {
        const float bin = (float)i / (float)(sampleCount - 1);
        const float freq = 20.0f * std::pow(1000.0f, bin);
        const float x = frequencyToX(freq);
        const float y = gainToY(juce::jlimit(-24.0f, 12.0f, analyzerSamples[static_cast<size_t>(i)] + 42.0f));
        if (i == 0)
            analyzerPath.startNewSubPath(x, y);
        else
            analyzerPath.lineTo(x, y);
    }
    auto analyzerFill = analyzerPath;
    analyzerFill.lineTo((float)gridArea.getRight(), (float)gridArea.getBottom());
    analyzerFill.lineTo((float)gridArea.getX(), (float)gridArea.getBottom());
    analyzerFill.closeSubPath();
    g.setColour(juce::Colour(0xff3bb8ff).withAlpha(0.11f));
    g.fillPath(analyzerFill);
    g.setColour(juce::Colours::white.withAlpha(0.22f));
    g.strokePath(analyzerPath, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto resonances = audioProcessor.getResonanceVisualState();
    for (const auto& resonance : resonances)
    {
        if (!resonance.active)
            continue;

        const auto frequency = juce::jlimit(20.0f, 20000.0f, resonance.frequencyHz);
        const auto gain = juce::jlimit(0.0f, 18.0f, resonance.gainDb);
        const auto q = juce::jlimit(0.9f, 30.0f, resonance.q);
        const auto x = frequencyToX(frequency);
        const auto zeroY = gainToY(0.0f);
        const auto topY = juce::jlimit((float)gridArea.getY(), zeroY, gainToY(juce::jmin(12.0f, gain)));
        const auto strength = juce::jlimit(0.0f, 1.0f, gain / 18.0f);
        const auto width = juce::jlimit(2.0f, 9.0f, 34.0f / q);

        juce::ColourGradient glow(juce::Colour(0xff37d9ff).withAlpha(0.0f), x - width * 3.5f, 0.0f,
                                  juce::Colour(0xff7b4cff).withAlpha(0.28f * strength), x, 0.0f, true);
        glow.addColour(1.0, juce::Colour(0xff37d9ff).withAlpha(0.0f));
        g.setGradientFill(glow);
        g.fillRect(juce::Rectangle<float>(x - width * 3.5f, (float)gridArea.getY(), width * 7.0f, (float)gridArea.getHeight()));

        g.setColour(juce::Colour(0xff48d8ff).withAlpha(0.22f + 0.58f * strength));
        g.drawLine(x, (float)gridArea.getBottom(), x, topY, 1.0f + strength * 2.0f);

        g.setColour(juce::Colour(0xffa774ff).withAlpha(0.28f + 0.55f * strength));
        g.fillEllipse(x - 3.0f - strength * 3.0f, topY - 3.0f - strength * 3.0f,
                      6.0f + strength * 6.0f, 6.0f + strength * 6.0f);
    }

    juce::Path eqPath;
    const int samplePoints = 180;
    for (int i = 0; i < samplePoints; ++i)
    {
        const float t = (float)i / (float)(samplePoints - 1);
        const float freq = 20.0f * std::pow(1000.0f, t);
        const float gain = computeEqGainAtFrequency(freq);
        const float x = frequencyToX(freq);
        const float y = gainToY(gain);
        if (i == 0)
            eqPath.startNewSubPath(x, y);
        else
            eqPath.lineTo(x, y);
    }

    g.setColour(juce::Colours::white);
    g.strokePath(eqPath, juce::PathStrokeType(2.4f));

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto freq = juce::jlimit(20.0f, 20000.0f, freqValues[band]->load());
        const auto gain = juce::jlimit(-24.0f, 12.0f, gainValues[band]->load());
        const auto x = frequencyToX(freq);
        const auto y = gainToY(gain);
        const float radius = 14.0f;
        const bool isActive = band == selectedBand || band == hoverBand;

        g.setColour(nodeColors[band].withAlpha(isActive ? 0.95f : 0.88f));
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.95f));
        g.drawEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        g.setColour(juce::Colours::white);
        {
            juce::Font font(juce::FontOptions{13.0f, juce::Font::bold});
            g.setFont(font);
        }
        g.drawText(juce::String(band + 1), juce::Rectangle<float>(x - radius, y - radius, radius * 2.0f, radius * 2.0f),
                   juce::Justification::centred);
    }

    g.setColour(juce::Colours::white.withAlpha(0.76f));
    {
        juce::Font font(juce::FontOptions{16.0f, juce::Font::bold});
        g.setFont(font);
    }
    g.drawText("EQ Curve", graphArea.withTop(10).removeFromLeft(220), juce::Justification::left);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    {
        juce::Font font(juce::FontOptions{12.0f});
        g.setFont(font);
    }
    for (auto freq : freqTicks)
    {
        const auto x = frequencyToX(freq);
        const auto label = freq >= 1000.0f ? juce::String(freq / 1000.0f, 0) + "k" : juce::String((int)freq);
        g.drawText(label, juce::Rectangle<float>(x - 28.0f, (float)gridArea.getBottom() + 4.0f, 56.0f, 18.0f), juce::Justification::centred);
    }

    for (auto gain : gainLines)
    {
        const auto y = gainToY(gain);
        const auto label = gain > 0 ? "+" + juce::String((int)gain) : juce::String((int)gain);
        g.drawText(label, juce::Rectangle<float>(4.0f, y - 10.0f, 52.0f, 20.0f), juce::Justification::left);
    }

}

void EqDisplayComponent::resized()
{
    graphArea = getLocalBounds().reduced(16);

    auto buttonArea = graphArea.reduced(16);
    flattenButton.setBounds(buttonArea.removeFromRight(120).removeFromTop(34));
}

void EqDisplayComponent::mouseDown(const juce::MouseEvent& event)
{
    const auto point = event.position;
    selectedBand = findClosestBand(point);
    if (selectedBand >= 0)
    {
        if (event.mods.isAltDown())
        {
            resetBandGain(selectedBand);
            return;
        }

        draggingBand = true;
        const auto freq = xToFrequency(point.x);
        const auto gain = yToGain(point.y);
        updateBandValue(selectedBand, freq, gain);
    }
}

void EqDisplayComponent::mouseMove(const juce::MouseEvent& event)
{
    updateNodeHighlight(event.position);
}

void EqDisplayComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingBand && selectedBand >= 0)
    {
        const auto freq = xToFrequency(event.position.x);
        const auto gain = yToGain(event.position.y);
        updateBandValue(selectedBand, freq, gain);
    }
}

void EqDisplayComponent::mouseUp(const juce::MouseEvent&)
{
    draggingBand = false;
}

void EqDisplayComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto point = event.position;
    int band = findClosestBand(point);
    if (band < 0)
    {
        float bestDistance = std::numeric_limits<float>::max();
        for (int candidate = 0; candidate < EqCurve::numBands; ++candidate)
        {
            const auto freq = juce::jlimit(20.0f, 20000.0f, freqValues[candidate]->load());
            const auto gain = juce::jlimit(-24.0f, 12.0f, gainValues[candidate]->load());
            const auto dx = point.x - frequencyToX(freq);
            const auto dy = point.y - gainToY(gain);
            const auto dist = std::sqrt(dx * dx + dy * dy);
            if (dist < bestDistance)
            {
                bestDistance = dist;
                band = candidate;
            }
        }
    }

    if (band >= 0)
    {
        const auto freq = xToFrequency(point.x);
        const auto gain = yToGain(point.y);
        updateBandValue(band, freq, gain);
    }
}

void EqDisplayComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const int band = selectedBand >= 0 ? selectedBand : findClosestBand(event.position);
    if (band >= 0)
        updateBandQ(band, wheel.deltaY * 0.18f);
}

void EqDisplayComponent::timerCallback()
{
    updateAnalyzer();
    repaint();
}

void EqDisplayComponent::buttonClicked(juce::Button* button)
{
    if (button == &flattenButton)
        flattenAllBands();
}

float EqDisplayComponent::xToFrequency(float x) const noexcept
{
    const auto graphX = (float)graphArea.reduced(20, 0).getX();
    const auto graphW = (float)graphArea.reduced(20, 0).getWidth();
    const auto t = juce::jlimit(0.0f, 1.0f, (x - graphX) / graphW);
    const auto logMin = std::log10(20.0f);
    const auto logMax = std::log10(20000.0f);
    const auto value = std::pow(10.0f, logMin + t * (logMax - logMin));
    return value;
}

float EqDisplayComponent::frequencyToX(float frequency) const noexcept
{
    const auto graphX = (float)graphArea.reduced(20, 0).getX();
    const auto graphW = (float)graphArea.reduced(20, 0).getWidth();
    const auto logMin = std::log10(20.0f);
    const auto logMax = std::log10(20000.0f);
    const auto t = (std::log10(juce::jlimit(20.0f, 20000.0f, frequency)) - logMin) / (logMax - logMin);
    return graphX + t * graphW;
}

float EqDisplayComponent::yToGain(float y) const noexcept
{
    const auto graphY = (float)graphArea.reduced(20, 0).getY();
    const auto graphH = (float)graphArea.reduced(20, 0).getHeight();
    const auto t = juce::jlimit(0.0f, 1.0f, (y - graphY) / graphH);
    return juce::jmap(t, 12.0f, -24.0f);
}

float EqDisplayComponent::gainToY(float gain) const noexcept
{
    const auto graphY = (float)graphArea.reduced(20, 0).getY();
    const auto graphH = (float)graphArea.reduced(20, 0).getHeight();
    const auto t = juce::jlimit(0.0f, 1.0f, juce::jmap(gain, 12.0f, -24.0f, 0.0f, 1.0f));
    return graphY + t * graphH;
}

int EqDisplayComponent::findClosestBand(const juce::Point<float>& point) const noexcept
{
    int nearest = -1;
    float bestDistance = 999999.0f;
    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto freq = juce::jlimit(20.0f, 20000.0f, freqValues[band]->load());
        const auto gain = juce::jlimit(-24.0f, 12.0f, gainValues[band]->load());
        const auto dx = point.x - frequencyToX(freq);
        const auto dy = point.y - gainToY(gain);
        const auto distance = dx * dx + dy * dy;
        if (distance < bestDistance)
        {
            bestDistance = distance;
            nearest = band;
        }
    }
    return bestDistance <= 28.0f * 28.0f ? nearest : -1;
}

bool EqDisplayComponent::hitTestNode(int band, const juce::Point<float>& point) const noexcept
{
    const auto freq = juce::jlimit(20.0f, 20000.0f, freqValues[band]->load());
    const auto gain = juce::jlimit(-24.0f, 12.0f, gainValues[band]->load());
    const auto dx = point.x - frequencyToX(freq);
    const auto dy = point.y - gainToY(gain);
    return dx * dx + dy * dy <= 16.0f * 16.0f;
}

void EqDisplayComponent::updateNodeHighlight(const juce::Point<float>& point)
{
    hoverBand = -1;
    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        if (hitTestNode(band, point))
        {
            hoverBand = band;
            break;
        }
    }
    repaint();
}

void EqDisplayComponent::updateBandValue(int band, float frequency, float gain)
{
    const auto clampedFreq = juce::jlimit(20.0f, 20000.0f, frequency);
    const auto clampedGain = juce::jlimit(-24.0f, 12.0f, gain);
    if (auto* freqParam = parameters.getParameter("eq" + juce::String(band + 1) + "Freq"))
    {
        freqParam->beginChangeGesture();
        freqParam->setValueNotifyingHost(freqParam->convertTo0to1(clampedFreq));
        freqParam->endChangeGesture();
    }
    if (auto* gainParam = parameters.getParameter("eq" + juce::String(band + 1) + "Gain"))
    {
        gainParam->beginChangeGesture();
        gainParam->setValueNotifyingHost(gainParam->convertTo0to1(clampedGain));
        gainParam->endChangeGesture();
    }
}

void EqDisplayComponent::updateBandQ(int band, float deltaQ)
{
    const auto paramId = "eq" + juce::String(band + 1) + "Q";
    if (auto* qParam = parameters.getParameter(paramId))
    {
        const auto currentQ = qValues[band]->load();
        const auto newQ = juce::jlimit(0.3f, 6.0f, currentQ + deltaQ);
        qParam->beginChangeGesture();
        qParam->setValueNotifyingHost(qParam->convertTo0to1(newQ));
        qParam->endChangeGesture();
    }
}

void EqDisplayComponent::resetBandGain(int band)
{
    const auto paramId = "eq" + juce::String(band + 1) + "Gain";
    if (auto* gainParam = parameters.getParameter(paramId))
    {
        gainParam->beginChangeGesture();
        gainParam->setValueNotifyingHost(gainParam->convertTo0to1(0.0f));
        gainParam->endChangeGesture();
    }
}

void EqDisplayComponent::flattenAllBands()
{
    constexpr std::array<float, EqCurve::numBands> defaultFreqs{45.0f,   120.0f, 500.0f,  1500.0f,
                                                                3200.0f, 6000.0f, 11000.0f};

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto idx = juce::String(band + 1);
        if (auto* freqParam = parameters.getParameter("eq" + idx + "Freq"))
        {
            freqParam->beginChangeGesture();
            freqParam->setValueNotifyingHost(freqParam->convertTo0to1(defaultFreqs[static_cast<size_t>(band)]));
            freqParam->endChangeGesture();
        }
        if (auto* gainParam = parameters.getParameter("eq" + idx + "Gain"))
        {
            gainParam->beginChangeGesture();
            gainParam->setValueNotifyingHost(gainParam->convertTo0to1(0.0f));
            gainParam->endChangeGesture();
        }
        if (auto* qParam = parameters.getParameter("eq" + idx + "Q"))
        {
            qParam->beginChangeGesture();
            qParam->setValueNotifyingHost(qParam->convertTo0to1(1.0f));
            qParam->endChangeGesture();
        }
    }

    selectedBand = -1;
    hoverBand = -1;
    repaint();
}

void EqDisplayComponent::updateAnalyzer()
{
    const auto copied = audioProcessor.pullAnalyzerSamples(pullBuffer.data(), static_cast<int>(pullBuffer.size()));
    for (int i = 0; i < copied; ++i)
        pushAnalyzerSample(pullBuffer[static_cast<size_t>(i)]);

    if (analyzerFrameReady)
        calculateAnalyzerFrame();
}

void EqDisplayComponent::pushAnalyzerSample(float sample) noexcept
{
    timeDomainBlock[static_cast<size_t>(analyzerWritePosition)] = sample;
    analyzerWritePosition = (analyzerWritePosition + 1) % fftSize;
    if (analyzerWritePosition == 0)
        analyzerFrameReady = true;
}

void EqDisplayComponent::calculateAnalyzerFrame()
{
    analyzerFrameReady = false;

    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::copy(timeDomainBlock.begin(), timeDomainBlock.end(), fftData.begin());
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const auto sampleRate = juce::jmax(1.0, audioProcessor.getAnalyzerSampleRate());
    const auto binForFrequency = [sampleRate](float frequency) {
        return juce::jlimit(1, fftSize / 2 - 1,
                            static_cast<int>(std::round(frequency * (float)fftSize / static_cast<float>(sampleRate))));
    };

    for (int i = 0; i < static_cast<int>(analyzerSamples.size()); ++i)
    {
        const float t = (float)i / (float)(analyzerSamples.size() - 1);
        const float frequency = 20.0f * std::pow(1000.0f, t);
        const int centreBin = binForFrequency(frequency);
        const int lowBin = juce::jmax(1, centreBin - 1);
        const int highBin = juce::jmin(fftSize / 2 - 1, centreBin + 1);

        float magnitude = 0.0f;
        for (int bin = lowBin; bin <= highBin; ++bin)
            magnitude = juce::jmax(magnitude, fftData[static_cast<size_t>(bin)]);

        const float db = juce::Decibels::gainToDecibels(magnitude / (float)fftSize, -90.0f);
        auto& smoothed = analyzerSamples[static_cast<size_t>(i)];
        smoothed = smoothed * 0.72f + juce::jlimit(-90.0f, 18.0f, db) * 0.28f;
    }
}

float EqDisplayComponent::computeEqGainAtFrequency(float frequency) const noexcept
{
    float result = 0.0f;
    for (int band = 0; band < EqCurve::numBands; ++band)
        result += computeBandContribution(band, frequency);
    return juce::jlimit(-24.0f, 12.0f, result);
}

float EqDisplayComponent::computeBandContribution(int band, float frequency) const noexcept
{
    const float centre = juce::jlimit(20.0f, 20000.0f, freqValues[band]->load());
    const float gain = gainValues[band]->load();
    const float qValue = juce::jlimit(0.3f, 6.0f, qValues[band]->load());

    const float logFreq = std::log10(frequency);
    const float logCentre = std::log10(centre);
    const float width = 0.4f + (1.0f / qValue) * 0.25f;
    const float delta = (logFreq - logCentre) / width;
    const float shape = std::exp(-delta * delta * 1.4f);
    return gain * shape;
}
