#include "PluginProcessor.h"
#include "AppLogger.h"

#include "Dsp/Utilities.h"
#include "PluginEditor.h"

#include <array>
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

    for (int band = 0; band < EqCurve::numBands; ++band)
    {
        const auto idx = juce::String(band + 1);
        eqFreqParameters[band] = parameters.getRawParameterValue("eq" + idx + "Freq");
        eqGainParameters[band] = parameters.getRawParameterValue("eq" + idx + "Gain");
        eqQParameters[band] = parameters.getRawParameterValue("eq" + idx + "Q");
    }
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

    int seedNow = lastSeed;
    if (seedParameter)
        seedNow = juce::jlimit(0, std::numeric_limits<int>::max(), static_cast<int>(seedParameter->load()));
    if (seedNow != lastSeed)
    {
        lastSeed = seedNow;
        resonanceEngine.setSeed(lastSeed);
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
    resonanceParams.syncDotted = syncDotted;
    bool syncTriplet = false;
    if (syncTripletParameter)
        syncTriplet = syncTripletParameter->load() >= 0.5f;
    resonanceParams.syncTriplet = syncTriplet;
    if (freeHzMaxParameter)
        resonanceParams.freeHzMax = freeHzMaxParameter->load();
    resonanceParams.bpm = readBpm();
    resonanceEngine.setParameters(resonanceParams);

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
    }
    else
    {
        {
            PerformanceProfiler::ScopedTimer timer{profiler, PerformanceProfiler::Section::ResonanceProcess};
            resonanceEngine.processBlock(buffer);
        }
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
    return new juce::GenericAudioProcessorEditor(*this);
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

juce::AudioProcessorValueTreeState::ParameterLayout
ResonanceEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        amountParam, "Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        randomnessParam, "Randomness", juce::NormalisableRange<float>(0.0f, 1.0f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        orderParam, "Order", juce::StringArray{"EQ->Res", "Res->EQ"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        outputGainParam, "Output Gain", juce::NormalisableRange<float>(-24.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(bypassParam, "Bypass", false));

    params.push_back(
        std::make_unique<juce::AudioParameterInt>(countMaxParam, "Count Max", 1, 12, 8));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        qMaxParam, "Q Max", juce::NormalisableRange<float>(0.7f, 14.0f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        motionMaxParam, "Motion Max", juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

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

    constexpr std::array<float, EqCurve::numBands> defaultFreqs{50.0f,   120.0f,  350.0f,  1000.0f,
                                                                2800.0f, 7000.0f, 14000.0f};

    for (auto i = 0; i < EqCurve::numBands; ++i)
    {
        const auto idx = juce::String(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "eq" + idx + "Freq", "EQ " + idx + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f),
            defaultFreqs[static_cast<size_t>(i)]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "eq" + idx + "Gain", "EQ " + idx + " Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
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

    if (!rerollPending.exchange(false))
        return;

    std::random_device rd;
    const auto raw = (static_cast<uint32_t>(rd()) << 1U) ^ static_cast<uint32_t>(rd());
    const auto newSeed = static_cast<int>(raw & 0x7fffffffU);

    if (auto* seed = parameters.getParameter(seedParam))
    {
        seed->beginChangeGesture();
        seed->setValueNotifyingHost(seed->convertTo0to1(static_cast<float>(newSeed)));
        seed->endChangeGesture();
    }

    if (auto* reroll = parameters.getParameter(rerollParam))
    {
        reroll->beginChangeGesture();
        reroll->setValueNotifyingHost(0.0f);
        reroll->endChangeGesture();
    }

    AppLogging::AppLogger::info("Parameters", "Reroll triggered", "newSeed=" + juce::String(newSeed));
}

void ResonanceEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
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
