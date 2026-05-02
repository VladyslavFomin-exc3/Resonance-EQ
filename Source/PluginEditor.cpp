#include "PluginEditor.h"

ResonanceEQAudioProcessorEditor::ResonanceEQAudioProcessorEditor(ResonanceEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), eqDisplay(p.getAPVTS(), p), resonancePanel(p.getAPVTS(), p)
{
    setLookAndFeel(&lookAndFeel);

    titleLabel.setText("Harmonic Resonance EQ", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    {
        juce::Font font(juce::FontOptions{24.0f, juce::Font::bold});
        titleLabel.setFont(font);
    }
    addAndMakeVisible(titleLabel);

    presetComboBox.setTextWhenNothingSelected("PRESETS");
    presetComboBox.setJustificationType(juce::Justification::centred);
    presetComboBox.onChange = [this] { loadSelectedPreset(); };
    addAndMakeVisible(presetComboBox);
    refreshPresetComboBox();

    bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3bb8ff));
    bypassButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this] { setBypassFromPowerButton(); };
    addAndMakeVisible(bypassButton);

    powerBypassLabel.setText("Power / Bypass", juce::dontSendNotification);
    powerBypassLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
    powerBypassLabel.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
    powerBypassLabel.setJustificationType(juce::Justification::centred);
    powerBypassLabel.setTooltip("Enables or bypasses plugin processing.");
    addAndMakeVisible(powerBypassLabel);

    stereoModeCombo.addItem("Stereo", 1);
    stereoModeCombo.setSelectedId(1);
    stereoModeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stereoModeCombo);

    statusLabel.setText("Resonance Engine: ON", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff52ff6f));
    {
        juce::Font font(juce::FontOptions{14.0f, juce::Font::bold});
        statusLabel.setFont(font);
    }
    statusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusLabel);

    addAndMakeVisible(eqDisplay);
    addAndMakeVisible(resonancePanel);

    updatePowerStateFromBypass();
    startTimerHz(15);

    setResizable(true, true);
    setResizeLimits(900, 560, 1300, 780);
    setSize(1000, 620);
}

ResonanceEQAudioProcessorEditor::~ResonanceEQAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ResonanceEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient gradient(juce::Colour(0xff10151e), bounds.getTopLeft(), juce::Colour(0xff0f141a), bounds.getBottomLeft(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.reduced(4.0f), 20.0f, 2.0f);
}

void ResonanceEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(14);
    auto topBar = bounds.removeFromTop(58);
    auto footer = bounds.removeFromBottom(56);
    const auto bottomPanelHeight = 230;
    auto bottomPanel = bounds.removeFromBottom(bottomPanelHeight);
    bounds.removeFromBottom(14);

    titleLabel.setBounds(topBar.removeFromLeft(320).withTrimmedLeft(8));

    auto rightBar = topBar.removeFromRight(220);
    presetComboBox.setBounds(rightBar.reduced(6));

    eqDisplay.setBounds(bounds);
    resonancePanel.setBounds(bottomPanel);

    auto footerLeft = footer.removeFromLeft(240);
    powerBypassLabel.setBounds(footerLeft.removeFromTop(16));
    bypassButton.setBounds(footerLeft.removeFromLeft(120).reduced(4, 2));
    stereoModeCombo.setBounds(footer.withTrimmedLeft((footer.getWidth() - 160) / 2).withWidth(160));
    statusLabel.setBounds(footer.removeFromRight(220).reduced(4));
}

void ResonanceEQAudioProcessorEditor::timerCallback()
{
    updatePowerStateFromBypass();
}

void ResonanceEQAudioProcessorEditor::updatePowerStateFromBypass()
{
    const auto* bypass = audioProcessor.getAPVTS().getRawParameterValue("bypass");
    const bool bypassed = bypass != nullptr && bypass->load() >= 0.5f;
    const bool powered = !bypassed;

    updatingPowerButton = true;
    bypassButton.setToggleState(powered, juce::dontSendNotification);
    updatingPowerButton = false;

    bypassButton.setButtonText(powered ? "Power" : "Bypassed");
    bypassButton.setColour(juce::TextButton::buttonColourId,
                           powered ? juce::Colour(0xff3bb8ff) : juce::Colour(0xff29313d));
    statusLabel.setText(powered ? "Resonance Engine: ON" : "Resonance Engine: OFF",
                        juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          powered ? juce::Colour(0xff52ff6f) : juce::Colour(0xffffc052));
    resonancePanel.setAlpha(powered ? 1.0f : 0.48f);
    eqDisplay.setAlpha(powered ? 1.0f : 0.78f);
}

void ResonanceEQAudioProcessorEditor::setBypassFromPowerButton()
{
    if (updatingPowerButton)
        return;

    if (auto* bypass = audioProcessor.getAPVTS().getParameter("bypass"))
    {
        const bool powerOn = bypassButton.getToggleState();
        bypass->beginChangeGesture();
        bypass->setValueNotifyingHost(powerOn ? 0.0f : 1.0f);
        bypass->endChangeGesture();
    }

    updatePowerStateFromBypass();
}

void ResonanceEQAudioProcessorEditor::refreshPresetComboBox()
{
    presetComboBox.clear(juce::dontSendNotification);

    const auto& presets = audioProcessor.getPresets();
    for (auto index = 0; index < static_cast<int>(presets.size()); ++index)
        presetComboBox.addItem(presets[static_cast<size_t>(index)].name, index + 1);

    const auto selectedIndex = audioProcessor.getSelectedPresetIndex();
    if (juce::isPositiveAndBelow(selectedIndex, static_cast<int>(presets.size())))
        presetComboBox.setSelectedId(selectedIndex + 1, juce::dontSendNotification);
}

void ResonanceEQAudioProcessorEditor::loadSelectedPreset()
{
    const auto presetIndex = presetComboBox.getSelectedId() - 1;
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(audioProcessor.getPresets().size())))
        return;

    audioProcessor.loadPreset(presetIndex);
    resonancePanel.syncControlsToParameters();
    updatePowerStateFromBypass();
}
