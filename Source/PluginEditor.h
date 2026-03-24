#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class ResonanceEQAudioProcessorEditor final : public juce::GenericAudioProcessorEditor
{
  public:
    explicit ResonanceEQAudioProcessorEditor(ResonanceEQAudioProcessor& p)
        : juce::GenericAudioProcessorEditor(p)
    {
    }
};
