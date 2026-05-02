#pragma once

#include <JuceHeader.h>

class ResonanceEQAudioProcessor;

class ResonancePanelComponent : public juce::Component
{
public:
    ResonancePanelComponent(juce::AudioProcessorValueTreeState& state, ResonanceEQAudioProcessor& processor);
    ~ResonancePanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void syncControlsToParameters();

private:
    juce::AudioProcessorValueTreeState& parameters;
    ResonanceEQAudioProcessor& audioProcessor;

    juce::Slider amountSlider;
    juce::Slider randomnessSlider;
    juce::Slider countMaxSlider;
    juce::Slider qMaxSlider;
    juce::Slider motionMaxSlider;
    juce::Slider seedSlider;

    juce::ComboBox orderCombo;
    juce::ComboBox rateModeCombo;
    juce::ComboBox syncNoteCombo;
    juce::ComboBox syncModifierCombo;

    juce::TextButton rerollButton {"REROLL"};

    juce::Label titleLabel;
    juce::Label amountLabel;
    juce::Label randomnessLabel;
    juce::Label orderLabel;
    juce::Label countMaxLabel;
    juce::Label qMaxLabel;
    juce::Label motionMaxLabel;
    juce::Label rateModeLabel;
    juce::Label syncNoteLabel;
    juce::Label syncModifierLabel;
    juce::Label seedLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> randomnessAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> countMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> motionMaxAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rerollAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> orderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncNoteAttachment;

    void configureSlider(juce::Slider& slider, bool rotary);
    void configureSeedSlider();
    void configureValueFormatters();
    void refreshFormattedSliderText();
    void configureLabel(juce::Label& label, const juce::String& text);
    void configureTooltips();
    void setLabelTooltip(juce::Label& label, const juce::String& text);
    void updateSyncModifierFromParameters();
    void setBoolParameter(const juce::String& id, bool value);
};
