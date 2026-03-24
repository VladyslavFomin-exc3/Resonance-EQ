#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

/**
 * @brief Generic editor wrapper for ResonanceEQAudioProcessor.
 *
 * The plugin uses JUCE's built-in generic editor to map parameters automatically.
 */
class ResonanceEQAudioProcessorEditor final : public juce::GenericAudioProcessorEditor
{
  public:
    /**
     * @brief Construct the editor with reference to the processor.
     * @param p The audio processor instance.
     */
    explicit ResonanceEQAudioProcessorEditor(ResonanceEQAudioProcessor& p)
        : juce::GenericAudioProcessorEditor(p)
    {
    }
};
