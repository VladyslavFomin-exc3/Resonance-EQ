#pragma once

#include <JuceHeader.h>

#include "Dsp/EqCurve.h"
#include "Dsp/ResonanceEngine.h"
#include "Dsp/SafetyLimiter.h"

class ResonanceEQAudioProcessor final : public juce::AudioProcessor,
                                        private juce::AsyncUpdater
{
public:
    ResonanceEQAudioProcessor();
    ~ResonanceEQAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;

private:
    void handleAsyncUpdate() override;
    float readBpm() const;
    void updateEqTargetsFromParameters();

    std::atomic<bool> rerollPending { false };
    bool rerollButtonWasDown = false;

    int lastSeed = 12345;

    EqCurve eqCurve;
    ResonanceEngine resonanceEngine;
    SafetyLimiter limiter;

    juce::dsp::Gain<float> outputGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonanceEQAudioProcessor)
};
