#include "PluginProcessor.h"
#include "AppLogger.h"

#include "Dsp/Utilities.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>
#include <random>

namespace
{
constexpr auto amountParam = "amount";
constexpr auto randomnessParam = "randomness";
constexpr auto orderParam = "order";
constexpr auto outputGainParam = "outputGainDb";
constexpr auto bypassParam = "bypass";
constexpr auto countMaxParam = "countMax";
constexpr auto qMaxParam = "qMax";
constexpr auto motionMaxParam = "motionMax";
constexpr auto rateModeParam = "rateMode";
constexpr auto syncNoteParam = "syncNote";
constexpr auto syncDottedParam = "syncDotted";
constexpr auto syncTripletParam = "syncTriplet";
constexpr auto freeHzMaxParam = "freeHzMax";
constexpr auto seedParam = "seed";
constexpr auto rerollParam = "reroll";
constexpr auto saturationAmountParam = "saturationAmount";
constexpr auto selectedPresetParam = "selectedPreset";

void setStateParameterValue(juce::ValueTree& state, const juce::String& id, const juce::var& plainValue)
{
    if (auto parameterState = state.getChildWithProperty("id", id); parameterState.isValid())
        parameterState.setProperty("value", plainValue, nullptr);
}

void notifyHostOfState(juce::AudioProcessorValueTreeState& parameters, const juce::ValueTree& state)
{
    for (auto child : state)
    {
        const auto id = child.getProperty("id").toString();
        if (id.isEmpty())
            continue;

        if (auto* parameter = parameters.getParameter(id))
        {
            const auto plainValue = static_cast<float>(child.getProperty("value"));
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
            parameter->endChangeGesture();
        }
    }
}

void setParameterPlainValue(juce::AudioProcessorValueTreeState& parameters,
                            const juce::String& id,
                            const float plainValue)
{
    if (auto* parameter = parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        parameter->endChangeGesture();
    }
}
} // namespace

ResonanceEQAudioProcessor::ResonanceEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    AppLogging::AppLogger::initialize();
    AppLogging::AppLogger::info("Plugin", "Processor constructor");

    const auto addListener = [this](const juce::String& id) {
        if (id.isNotEmpty())
            parameters.addParameterListener(id, this);
    };

    addListener(amountParam);
    addListener(randomnessParam);
    addListener(orderParam);
    addListener(outputGainParam);
    addListener(bypassParam);
    addListener(countMaxParam);
    addListener(qMaxParam);
    addListener(motionMaxParam);
    addListener(rateModeParam);
    addListener(syncNoteParam);
    addListener(syncDottedParam);
    addListener(syncTripletParam);
    addListener(freeHzMaxParam);
    addListener(seedParam);
    addListener(rerollParam);
    addListener(saturationAmountParam);
    addListener(selectedPresetParam);

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto idx = juce::String(band + 1);
        addListener("eq" + idx + "Freq");
        addListener("eq" + idx + "Gain");
        addListener("eq" + idx + "Q");
    }

    amountParameter = parameters.getRawParameterValue(amountParam);
    randomnessParameter = parameters.getRawParameterValue(randomnessParam);
    orderParameter = parameters.getRawParameterValue(orderParam);
    outputGainParameter = parameters.getRawParameterValue(outputGainParam);
    bypassParameter = parameters.getRawParameterValue(bypassParam);
    countMaxParameter = parameters.getRawParameterValue(countMaxParam);
    qMaxParameter = parameters.getRawParameterValue(qMaxParam);
    motionMaxParameter = parameters.getRawParameterValue(motionMaxParam);
    rateModeParameter = parameters.getRawParameterValue(rateModeParam);
    syncNoteParameter = parameters.getRawParameterValue(syncNoteParam);
    syncDottedParameter = parameters.getRawParameterValue(syncDottedParam);
    syncTripletParameter = parameters.getRawParameterValue(syncTripletParam);
    freeHzMaxParameter = parameters.getRawParameterValue(freeHzMaxParam);
    seedParameter = parameters.getRawParameterValue(seedParam);
    rerollParameter = parameters.getRawParameterValue(rerollParam);
    saturationAmountParameter = parameters.getRawParameterValue(saturationAmountParam);

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto idx = juce::String(band + 1);
        eqFreqParameters[band] = parameters.getRawParameterValue("eq" + idx + "Freq");
        eqGainParameters[band] = parameters.getRawParameterValue("eq" + idx + "Gain");
        eqQParameters[band] = parameters.getRawParameterValue("eq" + idx + "Q");
    }

    initialiseFactoryPresets();
}

ResonanceEQAudioProcessor::~ResonanceEQAudioProcessor()
{
    AppLogging::AppLogger::info("Plugin", "Processor destructor");
    AppLogging::AppLogger::shutdown();
}

const juce::String ResonanceEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ResonanceEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ResonanceEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ResonanceEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ResonanceEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ResonanceEQAudioProcessor::getNumPrograms()
{
    return 1;
}

int ResonanceEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ResonanceEQAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ResonanceEQAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ResonanceEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void ResonanceEQAudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
{
    PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::PrepareToPlay};
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    currentBlockSize.store(samplesPerBlock, std::memory_order_relaxed);
    try
    {
        AppLogging::AppLogger::info("Plugin", "prepareToPlay", "sampleRate=" + juce::String(sampleRate) + ", blockSize=" + juce::String(samplesPerBlock));

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
        spec.numChannels = static_cast<juce::uint32>(
            juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels()));

        eqCurve.prepare(spec);
        resonanceEngine.prepare(spec);
        limiter.prepare(spec);

        outputGain.prepare(spec);
        outputGain.setRampDurationSeconds(0.05);
        outputGain.setGainDecibels(0.0f);

        dryWetMixer.prepare(spec);
        dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
        dryWetMixer.setWetMixProportion(0.5f);
        dryWetMixer.reset();

        analyzerFifo.fill(0.0f);
        analyzerWriteIndex.store(0, std::memory_order_relaxed);
        analyzerReadIndex.store(0, std::memory_order_relaxed);

        eqCurve.reset();
        resonanceEngine.reset();
        limiter.reset();

        lastSeed = juce::jlimit(0, std::numeric_limits<int>::max(),
                                static_cast<int>(parameters.getRawParameterValue(seedParam)->load()));
        resonanceEngine.setSeed(lastSeed);
    }
    catch (const std::exception& e)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::critical("Plugin", "prepareToPlay failed", "errorId=" + errorId + ", reason=" + e.what());
        setLastError("Failed to start playback. Error ID: " + errorId);
    }
    catch (...)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::critical("Plugin", "prepareToPlay failed with unknown exception", "errorId=" + errorId);
        setLastError("Failed to start playback. Error ID: " + errorId);
    }
}

void ResonanceEQAudioProcessor::releaseResources()
{
    try
    {
        AppLogging::AppLogger::info("Plugin", "releaseResources");

        eqCurve.reset();
        resonanceEngine.reset();
        limiter.reset();
        outputGain.reset();
        dryWetMixer.reset();
    }
    catch (const std::exception& e)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("Plugin", "releaseResources failed", "errorId=" + errorId + ", reason=" + e.what());
        setLastError("Failed to release resources. Error ID: " + errorId);
    }
    catch (...)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("Plugin", "releaseResources failed with unknown exception", "errorId=" + errorId);
        setLastError("Failed to release resources. Error ID: " + errorId);
    }
}

#if !JucePlugin_IsMidiEffect
bool ResonanceEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsSynth
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    {
        return false;
    }

    return true;
#endif
}
#endif

void ResonanceEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    PerformanceProfiler::ScopedTimer totalTimer{profiler, PerformanceProfiler::Section::ProcessBlock};
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumSamples() <= 0)
    {
        reportRealtimeError(1001, "processBlock has no samples");
        return;
    }

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    pushAnalyzerSamples(buffer);

    // If bypass is engaged, skip the DSP pipeline entirely.
    // This ensures zero-latency passthrough and prevents unwanted state changes.
    bool bypassed = false;
    if (bypassParameter)
        bypassed = bypassParameter->load() >= 0.5f;

    if (bypassed)
    {
        return;
    }

    // Dry/wet amount controls blending of processed signal and original input.
    float amount = 0.0f;
    if (amountParameter)
        amount = hreq::util::clampFloat(amountParameter->load(), 0.0f, 1.0f);

    int order = 0;
    if (orderParameter)
        order = static_cast<int>(orderParameter->load());

    float outputGainDb = 0.0f;
    if (outputGainParameter)
        outputGainDb = hreq::util::clampFloat(outputGainParameter->load(), -24.0f, 12.0f);

    bool rerollDown = false;
    if (rerollParameter)
        rerollDown = (rerollParameter->load() >= 0.5f);
    if (rerollDown && !rerollButtonWasDown)
    {
        rerollPending.store(true);
        triggerAsyncUpdate();
    }
    rerollButtonWasDown = rerollDown;

    bool seedChanged = false;
    int seedNow = lastSeed;
    if (seedParameter)
        seedNow = juce::jlimit(0, std::numeric_limits<int>::max(), static_cast<int>(seedParameter->load()));
    if (seedNow != lastSeed)
    {
        lastSeed = seedNow;
        resonanceEngine.setSeed(lastSeed);
        seedChanged = true;
    }

    updateEqTargetsFromParameters();

    ResonanceEngine::Params resonanceParams;
    if (randomnessParameter)
        resonanceParams.randomness = randomnessParameter->load();
    int countMax = 1;
    if (countMaxParameter)
        countMax = static_cast<int>(countMaxParameter->load());
    resonanceParams.countMax = countMax;
    if (qMaxParameter)
        resonanceParams.qMax = qMaxParameter->load();
    if (motionMaxParameter)
        resonanceParams.motionMax = motionMaxParameter->load();
    int rateMode = 0;
    if (rateModeParameter)
        rateMode = static_cast<int>(rateModeParameter->load());
    resonanceParams.rateMode = rateMode;
    int syncNote = 0;
    if (syncNoteParameter)
        syncNote = static_cast<int>(syncNoteParameter->load());
    resonanceParams.syncNote = syncNote;
    bool syncDotted = false;
    if (syncDottedParameter)
        syncDotted = syncDottedParameter->load() >= 0.5f;
    bool syncTriplet = false;
    if (syncTripletParameter)
        syncTriplet = syncTripletParameter->load() >= 0.5f;
    if (syncDotted && syncTriplet)
        syncTriplet = false;
    resonanceParams.syncDotted = syncDotted;
    resonanceParams.syncTriplet = syncTriplet;
    if (freeHzMaxParameter)
        resonanceParams.freeHzMax = freeHzMaxParameter->load();
    resonanceParams.bpm = readBpm();
    resonanceEngine.setParameters(resonanceParams);

    if (seedChanged || resonanceRegeneratePending.exchange(false))
        resonanceEngine.forceRegenerateTargets();

    float saturationAmount = 0.15f;
    if (saturationAmountParameter)
        saturationAmount = hreq::util::clampFloat(saturationAmountParameter->load(), 0.0f, 1.0f);

    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.setWetMixProportion(amount);
    dryWetMixer.pushDrySamples(block);

    if (order == 0)
    {
        {
            PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::EqProcess};
            eqCurve.processBlock(buffer);
        }
        {
            PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::ResonanceProcess};
            resonanceEngine.processBlock(buffer);
        }
        processSaturation(buffer, saturationAmount, resonanceParams.randomness * amount);
    }
    else
    {
        {
            PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::ResonanceProcess};
            resonanceEngine.processBlock(buffer);
        }
        processSaturation(buffer, saturationAmount, resonanceParams.randomness * amount);
        {
            PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::EqProcess};
            eqCurve.processBlock(buffer);
        }
    }

    outputGain.setGainDecibels(outputGainDb);
    juce::dsp::ProcessContextReplacing<float> gainContext(block);
    outputGain.process(gainContext);

    dryWetMixer.mixWetSamples(block);
    limiter.processBlock(buffer);
}

bool ResonanceEQAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ResonanceEQAudioProcessor::createEditor()
{
    return new ResonanceEQAudioProcessorEditor(*this);
}

void ResonanceEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::GetStateInformation};
    try
    {
        const auto state = parameters.copyState();

        if (const auto xml = state.createXml())
        {
            copyXmlToBinary(*xml, destData);
        }

        AppLogging::AppLogger::info("State", "getStateInformation saved");
    }
    catch (const std::exception& e)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("State", "getStateInformation failed", "errorId=" + errorId + ", reason=" + e.what());
        setLastError("State save failed. Error ID: " + errorId);
    }
    catch (...)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("State", "getStateInformation failed with unknown exception", "errorId=" + errorId);
        setLastError("State save failed. Error ID: " + errorId);
    }
}

void ResonanceEQAudioProcessor::setStateInformation(const void* data, const int sizeInBytes)
{
    PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::SetStateInformation};
    try
    {
        if (const auto xml = getXmlFromBinary(data, sizeInBytes))
        {
            if (xml->hasTagName(parameters.state.getType()))
                parameters.replaceState(juce::ValueTree::fromXml(*xml));
        }

        lastSeed = juce::jlimit(0, std::numeric_limits<int>::max(),
                                static_cast<int>(parameters.getRawParameterValue(seedParam)->load()));
        resonanceEngine.setSeed(lastSeed);

        AppLogging::AppLogger::info("State", "setStateInformation loaded");
    }
    catch (const std::exception& e)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("State", "setStateInformation failed", "errorId=" + errorId + ", reason=" + e.what());
        setLastError("State load failed. Error ID: " + errorId);
    }
    catch (...)
    {
        const auto errorId = AppLogging::AppLogger::makeErrorId();
        AppLogging::AppLogger::error("State", "setStateInformation failed with unknown exception", "errorId=" + errorId);
        setLastError("State load failed. Error ID: " + errorId);
    }
}

void ResonanceEQAudioProcessor::loadPreset(const int presetIndex)
{
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(presets.size())))
        return;

    auto stateToLoad = presets[static_cast<size_t>(presetIndex)].state.createCopy();
    setStateParameterValue(stateToLoad, selectedPresetParam, presetIndex);

    notifyHostOfState(parameters, stateToLoad);
    parameters.replaceState(stateToLoad);

    lastSeed = juce::jlimit(0, std::numeric_limits<int>::max(),
                            static_cast<int>(parameters.getRawParameterValue(seedParam)->load()));
    resonanceEngine.setSeed(lastSeed);
    resonanceRegeneratePending.store(true);
}

int ResonanceEQAudioProcessor::getSelectedPresetIndex() const noexcept
{
    if (const auto* selectedPreset = parameters.getRawParameterValue(selectedPresetParam))
        return juce::jlimit(0, static_cast<int>(presets.size()) - 1,
                            static_cast<int>(selectedPreset->load()));

    return 0;
}

void ResonanceEQAudioProcessor::initialiseFactoryPresets()
{
    struct FactoryPreset
    {
        const char* name;
        float amount;
        float randomness;
        int countMax;
        float qMax;
        float motionMax;
        int order;
        int rateMode;
        int syncNote;
        bool syncDotted;
        bool syncTriplet;
        float freeHzMax;
        int seed;
        float saturationAmount;
    };

    constexpr std::array<FactoryPreset, 11> factoryPresets{{
        {"DEFAULT", 0.0f, 0.0f, 3, 6.0f, 0.0f, 0, 0, 2, false, false, 2.5f, 12345, 0.0f},
        {"Subtle Movement", 0.28f, 0.16f, 5, 4.0f, 0.18f, 0, 0, 3, false, false, 1.2f, 104729, 0.08f},
        {"Wide Resonance", 0.46f, 0.34f, 10, 8.5f, 0.36f, 0, 0, 2, true, false, 2.0f, 208351, 0.12f},
        {"Metallic Texture", 0.68f, 0.72f, 18, 22.0f, 0.44f, 1, 1, 4, false, false, 6.5f, 314159, 0.38f},
        {"Vocal Formant", 0.52f, 0.28f, 7, 14.0f, 0.22f, 0, 0, 1, false, true, 1.8f, 456791, 0.14f},
        {"Lo-Fi Drift", 0.58f, 0.48f, 9, 6.5f, 0.74f, 1, 1, 3, false, false, 3.2f, 593441, 0.24f},
        {"Aggressive Peaks", 0.82f, 0.62f, 22, 27.0f, 0.38f, 1, 0, 4, false, false, 2.7f, 704977, 0.42f},
        {"Ambient Motion", 0.62f, 0.42f, 14, 11.5f, 0.86f, 0, 0, 0, true, false, 4.4f, 819191, 0.10f},
        {"Percussive Color", 0.48f, 0.55f, 12, 16.5f, 0.32f, 1, 0, 4, false, true, 5.5f, 920011, 0.20f},
        {"Harmonic Boost", 0.74f, 0.24f, 8, 9.0f, 0.28f, 0, 0, 2, false, false, 2.4f, 1033337, 0.18f},
        {"Chaos Engine", 0.95f, 0.96f, 24, 30.0f, 1.0f, 1, 1, 4, false, true, 10.0f, 1264843, 0.58f},
    }};

    presets.clear();
    presets.reserve(factoryPresets.size());

    for (const auto& factoryPreset : factoryPresets)
    {
        auto state = parameters.copyState();
        setStateParameterValue(state, amountParam, factoryPreset.amount);
        setStateParameterValue(state, randomnessParam, factoryPreset.randomness);
        setStateParameterValue(state, countMaxParam, factoryPreset.countMax);
        setStateParameterValue(state, qMaxParam, factoryPreset.qMax);
        setStateParameterValue(state, motionMaxParam, factoryPreset.motionMax);
        setStateParameterValue(state, orderParam, factoryPreset.order);
        setStateParameterValue(state, rateModeParam, factoryPreset.rateMode);
        setStateParameterValue(state, syncNoteParam, factoryPreset.syncNote);
        setStateParameterValue(state, syncDottedParam, factoryPreset.syncDotted);
        setStateParameterValue(state, syncTripletParam, factoryPreset.syncTriplet);
        setStateParameterValue(state, freeHzMaxParam, factoryPreset.freeHzMax);
        setStateParameterValue(state, seedParam, factoryPreset.seed);
        setStateParameterValue(state, saturationAmountParam, factoryPreset.saturationAmount);
        setStateParameterValue(state, selectedPresetParam, static_cast<int>(presets.size()));

        presets.push_back({factoryPreset.name, state});
    }
}

void ResonanceEQAudioProcessor::reportRealtimeError(int errorCode, const char* description) noexcept
{
    realtimeErrorCode.store(errorCode);
    realtimeErrorDescription = description;
    realtimeErrorPending.store(true);
    triggerAsyncUpdate();
}

void ResonanceEQAudioProcessor::setLastError(const juce::String& message)
{
    const juce::ScopedLock lock(lastErrorLock);
    lastErrorMessage = message;
}

ResonanceEQAudioProcessor::ProfilingReport ResonanceEQAudioProcessor::getProfilingReport() const noexcept
{
    ProfilingReport report;
    report.sampleRate = static_cast<int>(currentSampleRate.load(std::memory_order_relaxed));
    report.blockSize = currentBlockSize.load(std::memory_order_relaxed);
    report.blockDurationUs = (report.sampleRate > 0 && report.blockSize > 0)
                                   ? (static_cast<double>(report.blockSize) * 1'000'000.0 / report.sampleRate)
                                   : 0.0;

    report.snapshots = profiler.getSnapshots();
    const auto& processSnapshot = report.snapshots[static_cast<size_t>(PerformanceProfiler::Section::ProcessBlock)];
    report.processBlockCalls = processSnapshot.callCount;
    report.averageProcessBlockUs = processSnapshot.averageTimeUs;
    report.maxProcessBlockNs = processSnapshot.maxTimeNs;

    if (report.blockDurationUs > 0.0)
    {
        report.averageLoadRatioPct = (report.averageProcessBlockUs / report.blockDurationUs) * 100.0;
        report.peakLoadRatioPct = (static_cast<double>(report.maxProcessBlockNs) / 1000.0 / report.blockDurationUs) * 100.0;
    }

    return report;
}

juce::String ResonanceEQAudioProcessor::getProfilingSummary() const
{
    const auto report = getProfilingReport();
    juce::String summary;

    summary << "Profiling Summary\n";
    summary << "=================\n";
    summary << "Sample rate: " << report.sampleRate << " Hz\n";
    summary << "Block size: " << report.blockSize << " samples\n";
    summary << "Block budget: " << report.blockDurationUs << " us\n";
    summary << "ProcessBlock calls: " << static_cast<int64_t>(report.processBlockCalls) << "\n";
    summary << "ProcessBlock average: " << report.averageProcessBlockUs << " us\n";
    summary << "ProcessBlock maximum: " << static_cast<int64_t>(report.maxProcessBlockNs) << " ns\n";
    summary << "Average load ratio: " << report.averageLoadRatioPct << " %\n";
    summary << "Peak load ratio: " << report.peakLoadRatioPct << " %\n";
    summary << "\n";

    for (const auto& snapshot : report.snapshots)
    {
        summary << PerformanceProfiler::getSectionName(snapshot.section) << ":\n";
        summary << "  Calls: " << static_cast<int64_t>(snapshot.callCount) << "\n";
        summary << "  Total: " << static_cast<int64_t>(snapshot.totalTimeNs) << " ns\n";
        summary << "  Avg: " << snapshot.averageTimeUs << " us\n";
        summary << "  Max: " << static_cast<int64_t>(snapshot.maxTimeNs) << " ns\n";
    }

    return summary;
}

void ResonanceEQAudioProcessor::resetProfilingMetrics() noexcept
{
    profiler.reset();
}

void ResonanceEQAudioProcessor::processSaturation(juce::AudioBuffer<float>& buffer,
                                                  const float amount,
                                                  const float randomness) noexcept
{
    const auto driveAmount = hreq::util::clampFloat(amount + randomness * 0.25f, 0.0f, 1.0f);

    if (driveAmount <= 0.0001f)
        return;

    const auto drive = 1.0f + driveAmount * 2.4f;
    const auto trim = 1.0f / std::tanh(drive);
    const auto wet = hreq::util::clampFloat(driveAmount, 0.0f, 0.85f);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto input = samples[sample];
            const auto saturated = std::tanh(input * drive) * trim;
            samples[sample] = input + (saturated - input) * wet;
        }
    }
}

void ResonanceEQAudioProcessor::pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numInputChannels = juce::jmax(1, juce::jmin(buffer.getNumChannels(), getTotalNumInputChannels()));
    const auto numSamples = buffer.getNumSamples();

    auto write = analyzerWriteIndex.load(std::memory_order_relaxed);
    auto read = analyzerReadIndex.load(std::memory_order_acquire);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mono = 0.0f;
        for (int channel = 0; channel < numInputChannels; ++channel)
            mono += buffer.getSample(channel, sample);
        mono /= static_cast<float>(numInputChannels);

        analyzerFifo[static_cast<size_t>(write)] = mono;
        write = (write + 1) % analyzerFifoSize;

        if (write == read)
        {
            read = (read + 1) % analyzerFifoSize;
            analyzerReadIndex.store(read, std::memory_order_release);
        }
    }

    analyzerWriteIndex.store(write, std::memory_order_release);
}

int ResonanceEQAudioProcessor::pullAnalyzerSamples(float* destination, int maxSamples) noexcept
{
    if (destination == nullptr || maxSamples <= 0)
        return 0;

    auto read = analyzerReadIndex.load(std::memory_order_relaxed);
    const auto write = analyzerWriteIndex.load(std::memory_order_acquire);
    int copied = 0;

    while (read != write && copied < maxSamples)
    {
        destination[copied++] = analyzerFifo[static_cast<size_t>(read)];
        read = (read + 1) % analyzerFifoSize;
    }

    analyzerReadIndex.store(read, std::memory_order_release);
    return copied;
}

std::array<ResonanceEngine::ResonanceVisualState, ResonanceEngine::maxResonances>
ResonanceEQAudioProcessor::getResonanceVisualState() const noexcept
{
    return resonanceEngine.getVisualState();
}

void ResonanceEQAudioProcessor::applySeedRandomization(const int seed)
{
    const auto clampedSeed = juce::jlimit(0, std::numeric_limits<int>::max(), seed);

    if (isApplyingSeedRandomization.exchange(true))
        return;

    struct ScopedApplyFlag
    {
        std::atomic<bool>& flag;
        ~ScopedApplyFlag() { flag.store(false); }
    } scopedFlag{isApplyingSeedRandomization};

    std::mt19937 rng(static_cast<std::mt19937::result_type>(clampedSeed));
    const auto randomFloat = [&rng](const float min, const float max) {
        return std::uniform_real_distribution<float>(min, max)(rng);
    };
    const auto randomInt = [&rng](const int min, const int max) {
        return std::uniform_int_distribution<int>(min, max)(rng);
    };

    const auto amount = randomFloat(0.2f, 1.0f);
    const auto randomness = randomFloat(0.1f, 1.0f);
    const auto countMax = randomInt(2, 24);
    const auto qMax = randomFloat(3.0f, 30.0f);
    const auto motionMax = randomFloat(0.0f, 1.0f);
    const auto order = randomInt(0, 1);
    const auto rateMode = randomInt(0, 1);
    const auto syncNote = randomInt(0, 4);
    const auto syncModifier = randomInt(0, 2);

    setParameterPlainValue(parameters, seedParam, static_cast<float>(clampedSeed));
    setParameterPlainValue(parameters, amountParam, amount);
    setParameterPlainValue(parameters, randomnessParam, randomness);
    setParameterPlainValue(parameters, countMaxParam, static_cast<float>(countMax));
    setParameterPlainValue(parameters, qMaxParam, qMax);
    setParameterPlainValue(parameters, motionMaxParam, motionMax);
    setParameterPlainValue(parameters, orderParam, static_cast<float>(order));
    setParameterPlainValue(parameters, rateModeParam, static_cast<float>(rateMode));
    setParameterPlainValue(parameters, syncNoteParam, static_cast<float>(syncNote));
    setParameterPlainValue(parameters, syncDottedParam, syncModifier == 1 ? 1.0f : 0.0f);
    setParameterPlainValue(parameters, syncTripletParam, syncModifier == 2 ? 1.0f : 0.0f);

    lastSeed = clampedSeed;
    resonanceEngine.setSeed(lastSeed);
    resonanceRegeneratePending.store(true);
    seedRandomizationPending.store(false);

    DBG("Seed changed: " << clampedSeed);
    DBG("Randomized amount: " << amount
        << " randomness: " << randomness
        << " count: " << countMax
        << " qMax: " << qMax
        << " motion: " << motionMax);
    AppLogging::AppLogger::info("Parameters", "Seed randomization applied",
                                "seed=" + juce::String(clampedSeed) +
                                    ", amount=" + juce::String(amount) +
                                    ", randomness=" + juce::String(randomness) +
                                    ", count=" + juce::String(countMax) +
                                    ", qMax=" + juce::String(qMax) +
                                    ", motion=" + juce::String(motionMax));
}

void ResonanceEQAudioProcessor::rerollSeed()
{
    std::random_device rd;
    const auto raw = (static_cast<uint32_t>(rd()) << 1U) ^ static_cast<uint32_t>(rd());
    const auto newSeed = static_cast<int>(raw & 0x7fffffffU);

    pendingSeedRandomization.store(newSeed);
    seedRandomizationPending.store(true);
    setParameterPlainValue(parameters, seedParam, static_cast<float>(newSeed));
    AppLogging::AppLogger::info("Parameters", "Reroll triggered", "newSeed=" + juce::String(newSeed));
}

juce::AudioProcessorValueTreeState::ParameterLayout
ResonanceEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        amountParam, "Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        randomnessParam, "Randomness", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        orderParam, "Order", juce::StringArray{"EQ->Res", "Res->EQ"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        outputGainParam, "Output Gain", juce::NormalisableRange<float>(-24.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(bypassParam, "Bypass", false));

    params.push_back(
        std::make_unique<juce::AudioParameterInt>(countMaxParam, "Count Max", 1, 24, 3));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        qMaxParam, "Q Max", juce::NormalisableRange<float>(0.7f, 30.0f), 6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        motionMaxParam, "Motion Max", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        rateModeParam, "Rate Mode", juce::StringArray{"Sync", "Free"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        syncNoteParam, "Sync Note", juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16"}, 2));
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(syncDottedParam, "Sync Dotted", false));
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(syncTripletParam, "Sync Triplet", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        freeHzMaxParam, "Free Hz Max", juce::NormalisableRange<float>(0.05f, 10.0f, 0.0f, 0.3f),
        2.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        seedParam, "Seed", 0, std::numeric_limits<int>::max(), 12345));
    params.push_back(std::make_unique<juce::AudioParameterBool>(rerollParam, "Reroll", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        saturationAmountParam, "Saturation Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        selectedPresetParam, "Selected Preset",
        juce::StringArray{"DEFAULT", "Subtle Movement", "Wide Resonance", "Metallic Texture", "Vocal Formant",
                          "Lo-Fi Drift", "Aggressive Peaks", "Ambient Motion", "Percussive Color",
                          "Harmonic Boost", "Chaos Engine"},
        0));

    constexpr std::array<float, EqCurve::numBands> defaultFreqs{45.0f,   120.0f, 500.0f,  1500.0f,
                                                                3200.0f, 6000.0f, 11000.0f};

    for (auto i = 0; i < EqCurve::numBands; ++i)
    {
        const auto idx = juce::String(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "eq" + idx + "Freq", "EQ " + idx + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f),
            defaultFreqs[static_cast<size_t>(i)]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "eq" + idx + "Gain", "EQ " + idx + " Gain",
            juce::NormalisableRange<float>(-24.0f, 12.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "eq" + idx + "Q", "EQ " + idx + " Q",
            juce::NormalisableRange<float>(0.3f, 6.0f, 0.0f, 0.35f), 1.0f));
    }

    return {params.begin(), params.end()};
}

void ResonanceEQAudioProcessor::handleAsyncUpdate()
{
    if (realtimeErrorPending.exchange(false))
    {
        const auto code = realtimeErrorCode.load();
        const auto description = realtimeErrorDescription;
        AppLogging::AppLogger::warning("Realtime", "Recovered realtime error", "code=" + juce::String(code) + ", desc=" + description);
        setLastError("Realtime error occurred. See log for error ID.");
    }

    if (parameterChangePending.exchange(false))
    {
        const auto value = parameterChangeValue.load();

        juce::String paramName;
        {
            const juce::ScopedLock lock(parameterChangeLock);
            paramName = parameterChangeId;
        }

        AppLogging::AppLogger::info("Parameter", "parameterChanged", "id=" + paramName + ", value=" + juce::String(value));
    }

    const auto shouldReroll = rerollPending.exchange(false);
    if (shouldReroll)
        rerollSeed();

    if (seedRandomizationPending.exchange(false))
        applySeedRandomization(pendingSeedRandomization.load());

    if (shouldReroll)
    {
        if (auto* reroll = parameters.getParameter(rerollParam))
        {
            reroll->beginChangeGesture();
            reroll->setValueNotifyingHost(0.0f);
            reroll->endChangeGesture();
        }
    }
}

void ResonanceEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == seedParam && !isApplyingSeedRandomization.load())
    {
        pendingSeedRandomization.store(juce::jlimit(0, std::numeric_limits<int>::max(),
                                                    static_cast<int>(newValue)));
        seedRandomizationPending.store(true);
        triggerAsyncUpdate();
    }

    if ((parameterID == syncDottedParam || parameterID == syncTripletParam) && newValue >= 0.5f)
    {
        const auto otherId = parameterID == syncDottedParam ? syncTripletParam : syncDottedParam;
        if (auto* other = parameters.getParameter(otherId))
        {
            if (other->getValue() >= 0.5f)
                other->setValueNotifyingHost(0.0f);
        }
    }

    parameterChangeValue.store(newValue);
    {
        const juce::ScopedLock lock(parameterChangeLock);
        parameterChangeId = parameterID;
    }

    parameterChangePending.store(true);
    triggerAsyncUpdate();
}

float ResonanceEQAudioProcessor::readBpm() const
{
    if (auto* currentPlayHead = getPlayHead())
    {
        if (const auto pos = currentPlayHead->getPosition())
        {
            if (const auto bpm = pos->getBpm())
                return hreq::util::clampFloat(static_cast<float>(*bpm), 20.0f, 300.0f);
        }
    }

    return 120.0f;
}

void ResonanceEQAudioProcessor::updateEqTargetsFromParameters()
{
    PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::UpdateEqTargets};

    for (auto band = 0; band < EqCurve::numBands; ++band)
    {
        float freq = 1000.0f; // default
        if (eqFreqParameters[band])
            freq = eqFreqParameters[band]->load();

        float gain = 0.0f;
        if (eqGainParameters[band])
            gain = eqGainParameters[band]->load();

        float qValue = 1.0f;
        if (eqQParameters[band])
            qValue = eqQParameters[band]->load();

        eqCurve.setBandTarget(band, freq, gain, qValue);
    }
}

juce::String ResonanceEQAudioProcessor::getLastErrorMessage() const
{
    const juce::ScopedLock lock(lastErrorLock);
    return lastErrorMessage;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    AppLogging::AppLogger::info("Plugin", "createPluginFilter");
    return new ResonanceEQAudioProcessor();
}
