#pragma once

#include <JuceHeader.h>
#include "Dsp/EqCurve.h"

class ResonanceEQAudioProcessor;

class EqDisplayComponent : public juce::Component,
                           private juce::Timer,
                           private juce::Button::Listener
{
public:
    EqDisplayComponent(juce::AudioProcessorValueTreeState& state, ResonanceEQAudioProcessor& processor);
    ~EqDisplayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    void timerCallback() override;
    void buttonClicked(juce::Button* button) override;

    float xToFrequency(float x) const noexcept;
    float frequencyToX(float frequency) const noexcept;
    float yToGain(float y) const noexcept;
    float gainToY(float gain) const noexcept;
    int findClosestBand(const juce::Point<float>& point) const noexcept;
    bool hitTestNode(int band, const juce::Point<float>& point) const noexcept;
    void updateNodeHighlight(const juce::Point<float>& point);
    void updateBandValue(int band, float frequency, float gain);
    void updateBandQ(int band, float deltaQ);
    void resetBandGain(int band);
    void flattenAllBands();
    void updateAnalyzer();
    void pushAnalyzerSample(float sample) noexcept;
    void calculateAnalyzerFrame();
    float computeEqGainAtFrequency(float frequency) const noexcept;
    float computeBandContribution(int band, float frequency) const noexcept;

    juce::AudioProcessorValueTreeState& parameters;
    ResonanceEQAudioProcessor& audioProcessor;
    std::array<std::atomic<float>*, EqCurve::numBands> freqValues{};
    std::array<std::atomic<float>*, EqCurve::numBands> gainValues{};
    std::array<std::atomic<float>*, EqCurve::numBands> qValues{};

    juce::TextButton flattenButton;
    juce::Rectangle<int> graphArea;
    std::array<juce::Colour, EqCurve::numBands> nodeColors;
    std::vector<float> analyzerSamples;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, fftSize> timeDomainBlock{};
    std::array<float, fftSize * 2> fftData{};
    std::array<float, 512> pullBuffer{};
    int analyzerWritePosition = 0;
    bool analyzerFrameReady = false;

    int selectedBand = -1;
    int hoverBand = -1;
    bool draggingBand = false;
};
