#pragma once

#include <JuceHeader.h>

#include "Dsp/EqCurve.h"
#include "Dsp/ResonanceEngine.h"
#include "Dsp/SafetyLimiter.h"
#include "PerformanceProfiler.h"

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
class ResonanceEQAudioProcessor final : public juce::AudioProcessor,
                                         private juce::AsyncUpdater,
                                         private juce::AudioProcessorValueTreeState::Listener
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

    /** @brief Get user-friendly last non-realtime error status. */
    juce::String getLastErrorMessage() const;

    struct ProfilingReport
    {
        int sampleRate = 44100;
        int blockSize = 512;
        double blockDurationUs = 0.0;
        double averageProcessBlockUs = 0.0;
        std::uint64_t maxProcessBlockNs = 0;
        double averageLoadRatioPct = 0.0;
        double peakLoadRatioPct = 0.0;
        std::uint64_t processBlockCalls = 0;
        std::array<PerformanceProfiler::Snapshot, PerformanceProfiler::sectionCount()> snapshots{};
    };

    /**
     * @brief Retrieve a formatted profiling report in a safe context.
     */
    juce::String getProfilingSummary() const;

    /**
     * @brief Retrieve raw profiling snapshots for lab comparison.
     */
    ProfilingReport getProfilingReport() const noexcept;

    /**
     * @brief Clear accumulated profiling metrics.
     */
    void resetProfilingMetrics() noexcept;

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

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    /** @brief Audio thread flags for recovered realtime issues. */
    void reportRealtimeError(int errorCode, const char* description) noexcept;

    /**
     * @brief Read BPM from host transport.
     * @return BPM clamped to [20, 300], defaults to 120.
     */
    float readBpm() const;

    void setLastError(const juce::String& message);

    /** @brief Update each EQ band target from parameters. */
    void updateEqTargetsFromParameters();

    std::atomic<bool> rerollPending{false};
    bool rerollButtonWasDown = false;

    std::atomic<bool> realtimeErrorPending{false};
    std::atomic<int> realtimeErrorCode{0};
    juce::String realtimeErrorDescription;

    std::atomic<bool> parameterChangePending{false};
    std::atomic<float> parameterChangeValue{0.0f};
    juce::String parameterChangeId;
    juce::CriticalSection parameterChangeLock;

    juce::String lastErrorMessage;
    mutable juce::CriticalSection lastErrorLock;

    int lastSeed = 12345;

    std::atomic<double> currentSampleRate{44100.0};
    std::atomic<int> currentBlockSize{512};

    std::atomic<float>* amountParameter = nullptr;
    std::atomic<float>* randomnessParameter = nullptr;
    std::atomic<float>* orderParameter = nullptr;
    std::atomic<float>* outputGainParameter = nullptr;
    std::atomic<float>* bypassParameter = nullptr;
    std::atomic<float>* countMaxParameter = nullptr;
    std::atomic<float>* qMaxParameter = nullptr;
    std::atomic<float>* motionMaxParameter = nullptr;
    std::atomic<float>* rateModeParameter = nullptr;
    std::atomic<float>* syncNoteParameter = nullptr;
    std::atomic<float>* syncDottedParameter = nullptr;
    std::atomic<float>* syncTripletParameter = nullptr;
    std::atomic<float>* freeHzMaxParameter = nullptr;
    std::atomic<float>* seedParameter = nullptr;
    std::atomic<float>* rerollParameter = nullptr;

    std::array<std::atomic<float>*, EqCurve::numBands> eqFreqParameters{};
    std::array<std::atomic<float>*, EqCurve::numBands> eqGainParameters{};
    std::array<std::atomic<float>*, EqCurve::numBands> eqQParameters{};

    EqCurve eqCurve;
    PerformanceProfiler profiler;

    ResonanceEngine resonanceEngine;
    SafetyLimiter limiter;

    juce::dsp::Gain<float> outputGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonanceEQAudioProcessor)
};
