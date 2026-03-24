#pragma once

#include <JuceHeader.h>

#include "Dsp/EqCurve.h"
#include "Dsp/ResonanceEngine.h"
#include "Dsp/SafetyLimiter.h"

/**
 * @mainpage ResonanceEQ Documentation
 *
 * @section intro Introduction
 * ResonanceEQ is a VST plugin built with JUCE.
 * It implements a spectral EQ with dynamic resonances.
 *
 * @section architecture Architecture
 * DSP pipeline:
 * - EQ
 * - Resonance Engine
 * - Safety Limiter
 *
 * @section usage Usage
 * The plugin processes audio in real-time and applies
 * dynamic spectral transformations.
 */

/**
 * @brief Core audio processor for the ResonanceEQ plugin.
 *
 * Handles parameter state, DSP chain orchestration (EQ -> resonance -> limiter),
 * and host integration (preset/state, sample rate, buffer setup).
 */
class ResonanceEQAudioProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater
{
  public:
    /** @brief Constructor. */
    ResonanceEQAudioProcessor();

    /** @brief Destructor. */
    ~ResonanceEQAudioProcessor() override;

    /**
     * @brief Prepare DSP modules for playback.
     *
     * Called on audio thread startup.
     * @param sampleRate Current sample rate in Hz.
     * @param samplesPerBlock Maximum block size (frames).
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    /** @brief Release audio resources. */
    void releaseResources() override;

#if !JucePlugin_IsMidiEffect
    /**
     * @brief Validate input/output bus layouts.
     *
     * @param layouts Bus layout configuration to validate.
     * @return True if layout is supported.
     */
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    /**
     * @brief Process an audio block through the DSP pipeline.
     *
     * Swaps order of EQ and resonance modules based on parameter,
     * applies gain and hard limiter, and mixes dry/wet.
     * @param buffer Audio buffer to process.
     * @param midiBuffer Unused in this effect.
     */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer) override;

    /** @brief Create the built-in Generic editor. */
    juce::AudioProcessorEditor* createEditor() override;

    /** @brief Report that this plugin has a GUI editor. */
    bool hasEditor() const override;

    /** @brief Get the plugin name. */
    const juce::String getName() const override;

    /** @brief MIDI input support query. */
    bool acceptsMidi() const override;

    /** @brief MIDI output support query. */
    bool producesMidi() const override;

    /** @brief Is MIDI effect query. */
    bool isMidiEffect() const override;

    /** @brief Tail length in seconds: no tail. */
    double getTailLengthSeconds() const override;

    /** @brief Program count (single program). */
    int getNumPrograms() override;

    /** @brief Active program index. */
    int getCurrentProgram() override;

    /** @brief Set active program index (unused). */
    void setCurrentProgram(int index) override;

    /** @brief Get program name (unused). */
    const juce::String getProgramName(int index) override;

    /** @brief Rename program (unused). */
    void changeProgramName(int index, const juce::String& newName) override;

    /**
     * @brief Serialize state to host.
     * @param destData Destination memory block to fill.
     */
    void getStateInformation(juce::MemoryBlock& destData) override;

    /**
     * @brief Deserialize state from host.
     * @param data Source state data.
     * @param sizeInBytes Size of source in bytes.
     */
    void setStateInformation(const void* data, int sizeInBytes) override;

    /**
     * @brief Create the parameter layout for this plugin.
     * @return Configured parameter layout.
     */
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** @brief Parameter storage (ValueTree state). */
    juce::AudioProcessorValueTreeState parameters;

  private:
    /** @brief Handle async reroll requests from UI.
     *
     * @note This is called on the message thread, not the audio thread.
     */
    void handleAsyncUpdate() override;

    /**
     * @brief Read BPM from host transport.
     * @return BPM clamped to [20, 300], defaults to 120.
     */
    float readBpm() const;

    /** @brief Update each EQ band target from parameters. */
    void updateEqTargetsFromParameters();

    std::atomic<bool> rerollPending{false};
    bool rerollButtonWasDown = false;

    int lastSeed = 12345;

    EqCurve eqCurve;
    ResonanceEngine resonanceEngine;
    SafetyLimiter limiter;

    juce::dsp::Gain<float> outputGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonanceEQAudioProcessor)
};
