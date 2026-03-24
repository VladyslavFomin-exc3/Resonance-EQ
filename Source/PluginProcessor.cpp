#include "PluginProcessor.h"

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
}

ResonanceEQAudioProcessor::~ResonanceEQAudioProcessor() = default;

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

void ResonanceEQAudioProcessor::releaseResources() {}

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
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    bool bypassed = false;
    if (auto* param = parameters.getRawParameterValue(bypassParam))
    {
        bypassed = param->load() >= 0.5f;
    }

    if (bypassed)
    {
        return;
    }

    float amount = 0.0f;
    if (auto* param = parameters.getRawParameterValue(amountParam))
        amount = hreq::util::clampFloat(param->load(), 0.0f, 1.0f);

    int order = 0;
    if (auto* param = parameters.getRawParameterValue(orderParam))
        order = static_cast<int>(param->load());

    float outputGainDb = 0.0f;
    if (auto* param = parameters.getRawParameterValue(outputGainParam))
        outputGainDb = hreq::util::clampFloat(param->load(), -24.0f, 12.0f);

    bool rerollDown = false;
    if (auto* param = parameters.getRawParameterValue(rerollParam))
        rerollDown = (param->load() >= 0.5f);
    if (rerollDown && !rerollButtonWasDown)
    {
        rerollPending.store(true);
        triggerAsyncUpdate();
    }
    rerollButtonWasDown = rerollDown;

    int seedNow = lastSeed;
    if (auto* param = parameters.getRawParameterValue(seedParam))
        seedNow = juce::jlimit(0, std::numeric_limits<int>::max(), static_cast<int>(param->load()));
    if (seedNow != lastSeed)
    {
        lastSeed = seedNow;
        resonanceEngine.setSeed(lastSeed);
    }

    updateEqTargetsFromParameters();

    ResonanceEngine::Params resonanceParams;
    if (auto* param = parameters.getRawParameterValue(randomnessParam))
        resonanceParams.randomness = param->load();
    int countMax = 1;
    if (auto* param = parameters.getRawParameterValue(countMaxParam))
        countMax = static_cast<int>(param->load());
    resonanceParams.countMax = countMax;
    if (auto* param = parameters.getRawParameterValue(qMaxParam))
        resonanceParams.qMax = param->load();
    if (auto* param = parameters.getRawParameterValue(motionMaxParam))
        resonanceParams.motionMax = param->load();
    int rateMode = 0;
    if (auto* param = parameters.getRawParameterValue(rateModeParam))
        rateMode = static_cast<int>(param->load());
    resonanceParams.rateMode = rateMode;
    int syncNote = 0;
    if (auto* param = parameters.getRawParameterValue(syncNoteParam))
        syncNote = static_cast<int>(param->load());
    resonanceParams.syncNote = syncNote;
    bool syncDotted = false;
    if (auto* param = parameters.getRawParameterValue(syncDottedParam))
        syncDotted = param->load() >= 0.5f;
    resonanceParams.syncDotted = syncDotted;
    bool syncTriplet = false;
    if (auto* param = parameters.getRawParameterValue(syncTripletParam))
        syncTriplet = param->load() >= 0.5f;
    resonanceParams.syncTriplet = syncTriplet;
    if (auto* param = parameters.getRawParameterValue(freeHzMaxParam))
        resonanceParams.freeHzMax = param->load();
    resonanceParams.bpm = readBpm();
    resonanceEngine.setParameters(resonanceParams);

    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.setWetMixProportion(amount);
    dryWetMixer.pushDrySamples(block);

    if (order == 0)
    {
        eqCurve.processBlock(buffer);
        resonanceEngine.processBlock(buffer);
    }
    else
    {
        resonanceEngine.processBlock(buffer);
        eqCurve.processBlock(buffer);
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
    const auto state = parameters.copyState();

    if (const auto xml = state.createXml())
    {
        copyXmlToBinary(*xml, destData);
    }
}

void ResonanceEQAudioProcessor::setStateInformation(const void* data, const int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }

    lastSeed = juce::jlimit(0, std::numeric_limits<int>::max(),
                            static_cast<int>(parameters.getRawParameterValue(seedParam)->load()));
    resonanceEngine.setSeed(lastSeed);
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
    for (auto band = 0; band < EqCurve::numBands; ++band)
    {
        const auto idx = juce::String(band + 1);
        const auto freqParamName = "eq" + idx + "Freq";
        const auto gainParamName = "eq" + idx + "Gain";
        const auto qParamName = "eq" + idx + "Q";

        float freq = 1000.0f; // default
        if (auto* param = parameters.getRawParameterValue(freqParamName))
            freq = param->load();

        float gain = 0.0f;
        if (auto* param = parameters.getRawParameterValue(gainParamName))
            gain = param->load();

        float qValue = 1.0f;
        if (auto* param = parameters.getRawParameterValue(qParamName))
            qValue = param->load();

        eqCurve.setBandTarget(band, freq, gain, qValue);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonanceEQAudioProcessor();
}
