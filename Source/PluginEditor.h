#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/EqDisplayComponent.h"
#include "UI/ResonancePanelComponent.h"

class ResonanceEQAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit ResonanceEQAudioProcessorEditor(ResonanceEQAudioProcessor& p);
    ~ResonanceEQAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updatePowerStateFromBypass();
    void setBypassFromPowerButton();
    void refreshPresetComboBox();
    void loadSelectedPreset();

    ResonanceEQAudioProcessor& audioProcessor;
    CustomLookAndFeel lookAndFeel;
    EqDisplayComponent eqDisplay;
    ResonancePanelComponent resonancePanel;
    juce::TooltipWindow tooltipWindow {this, 2000};

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::ComboBox presetComboBox;
    juce::TextButton bypassButton {"Power"};
    juce::Label powerBypassLabel;
    juce::ComboBox stereoModeCombo;
    bool updatingPowerButton = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonanceEQAudioProcessorEditor)
};
