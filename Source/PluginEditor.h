#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ResonanceEQAudioProcessorEditor final : public juce::GenericAudioProcessorEditor
{
public:
    explicit ResonanceEQAudioProcessorEditor(ResonanceEQAudioProcessor& p)
        : juce::GenericAudioProcessorEditor(p)
    {
    }
};
