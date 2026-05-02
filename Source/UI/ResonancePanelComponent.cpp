#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ResonancePanelComponent.h"

namespace
{
constexpr int labelHeight = 18;
constexpr int valueHeight = 24;
constexpr int comboHeight = 34;
constexpr int controlGap = 10;
} // namespace

ResonancePanelComponent::ResonancePanelComponent(juce::AudioProcessorValueTreeState& state,
                                                 ResonanceEQAudioProcessor& processor)
    : parameters(state), audioProcessor(processor)
{
    configureLabel(titleLabel, "RESONANCE ENGINE");
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    {
        juce::Font font(juce::FontOptions{14.0f, juce::Font::bold});
        titleLabel.setFont(font);
    }
    addAndMakeVisible(titleLabel);

    configureLabel(amountLabel, "AMOUNT");
    configureLabel(randomnessLabel, "RANDOM");
    configureLabel(orderLabel, "ORDER");
    configureLabel(countMaxLabel, "COUNT");
    configureLabel(qMaxLabel, "Q MAX");
    configureLabel(motionMaxLabel, "MOTION");
    configureLabel(rateModeLabel, "RATE MODE");
    configureLabel(syncNoteLabel, "SYNC NOTE");
    configureLabel(syncModifierLabel, "SYNC MOD");
    configureLabel(seedLabel, "SEED");

    addAndMakeVisible(amountLabel);
    addAndMakeVisible(randomnessLabel);
    addAndMakeVisible(orderLabel);
    addAndMakeVisible(countMaxLabel);
    addAndMakeVisible(qMaxLabel);
    addAndMakeVisible(motionMaxLabel);
    addAndMakeVisible(rateModeLabel);
    addAndMakeVisible(syncNoteLabel);
    addAndMakeVisible(syncModifierLabel);
    addAndMakeVisible(seedLabel);

    configureSlider(amountSlider, true);
    configureSlider(randomnessSlider, true);
    configureSlider(countMaxSlider, true);
    configureSlider(qMaxSlider, true);
    configureSlider(motionMaxSlider, true);
    configureSeedSlider();

    countMaxSlider.setRange(1, 24, 1);

    addAndMakeVisible(amountSlider);
    addAndMakeVisible(randomnessSlider);
    addAndMakeVisible(countMaxSlider);
    addAndMakeVisible(qMaxSlider);
    addAndMakeVisible(motionMaxSlider);
    addAndMakeVisible(seedSlider);

    orderCombo.addItem("EQ -> RES", 1);
    orderCombo.addItem("RES -> EQ", 2);
    rateModeCombo.addItem("SYNC", 1);
    rateModeCombo.addItem("FREE", 2);
    syncNoteCombo.addItem("1/1", 1);
    syncNoteCombo.addItem("1/2", 2);
    syncNoteCombo.addItem("1/4", 3);
    syncNoteCombo.addItem("1/8", 4);
    syncNoteCombo.addItem("1/16", 5);
    syncModifierCombo.addItem("STRAIGHT", 1);
    syncModifierCombo.addItem("DOTTED", 2);
    syncModifierCombo.addItem("TRIPLET", 3);

    orderCombo.setJustificationType(juce::Justification::centred);
    rateModeCombo.setJustificationType(juce::Justification::centred);
    syncNoteCombo.setJustificationType(juce::Justification::centred);
    syncModifierCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(orderCombo);
    addAndMakeVisible(rateModeCombo);
    addAndMakeVisible(syncNoteCombo);
    addAndMakeVisible(syncModifierCombo);

    addAndMakeVisible(rerollButton);

    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "amount", amountSlider);
    randomnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "randomness", randomnessSlider);
    countMaxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "countMax", countMaxSlider);
    qMaxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "qMax", qMaxSlider);
    motionMaxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "motionMax", motionMaxSlider);
    seedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, "seed", seedSlider);
    rerollAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "reroll", rerollButton);
    seedSlider.onValueChange = [this] {
        audioProcessor.applySeedRandomization(static_cast<int>(seedSlider.getValue()));
        updateSyncModifierFromParameters();
        refreshFormattedSliderText();
    };
    configureValueFormatters();
    refreshFormattedSliderText();

    orderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(parameters, "order", orderCombo);
    rateModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(parameters, "rateMode", rateModeCombo);
    syncNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(parameters, "syncNote", syncNoteCombo);

    syncModifierCombo.onChange = [this] {
        const auto selected = syncModifierCombo.getSelectedId();
        setBoolParameter("syncDotted", selected == 2);
        setBoolParameter("syncTriplet", selected == 3);
    };
    updateSyncModifierFromParameters();

    rerollButton.setClickingTogglesState(true);
    rerollButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2f78ff));
    rerollButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    configureTooltips();
}

ResonancePanelComponent::~ResonancePanelComponent() = default;

void ResonancePanelComponent::syncControlsToParameters()
{
    updateSyncModifierFromParameters();
    refreshFormattedSliderText();
}

void ResonancePanelComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(8).toFloat();
    g.setColour(juce::Colour(0xff0b1017));
    g.fillRoundedRectangle(bounds, 12.0f);

    juce::ColourGradient gradient(juce::Colour(0xff151e2a), bounds.getTopLeft(),
                                  juce::Colour(0xff0f151d), bounds.getBottomLeft(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.reduced(1.0f), 11.0f);

    g.setColour(juce::Colour(0xff3bb8ff).withAlpha(0.48f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 12.0f, 1.2f);
}

void ResonancePanelComponent::resized()
{
    auto area = getLocalBounds().reduced(18, 12);
    titleLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(4);

    auto topRow = area.removeFromTop(124);
    auto bottomRow = area.removeFromTop(72);

    const auto placeKnob = [](juce::Rectangle<int> cell, juce::Label& label, juce::Component& control) {
        label.setBounds(cell.removeFromTop(labelHeight));
        cell.removeFromTop(3);
        control.setBounds(cell.reduced(2, 0));
    };

    const auto placeBox = [](juce::Rectangle<int> cell, juce::Label& label, juce::Component& control) {
        label.setBounds(cell.removeFromTop(labelHeight));
        cell.removeFromTop(8);
        control.setBounds(cell.removeFromTop(comboHeight).reduced(2, 0));
    };

    const int available = topRow.getWidth();
    const int wideBoxWidth = juce::jlimit(118, 150, available / 7);
    const int knobWidth = juce::jmax(82, (available - wideBoxWidth - controlGap * 5) / 5);

    placeKnob(topRow.removeFromLeft(knobWidth), amountLabel, amountSlider);
    topRow.removeFromLeft(controlGap);
    placeKnob(topRow.removeFromLeft(knobWidth), randomnessLabel, randomnessSlider);
    topRow.removeFromLeft(controlGap);
    placeBox(topRow.removeFromLeft(wideBoxWidth), orderLabel, orderCombo);
    topRow.removeFromLeft(controlGap);
    placeKnob(topRow.removeFromLeft(knobWidth), countMaxLabel, countMaxSlider);
    topRow.removeFromLeft(controlGap);
    placeKnob(topRow.removeFromLeft(knobWidth), qMaxLabel, qMaxSlider);
    topRow.removeFromLeft(controlGap);
    placeKnob(topRow.removeFromLeft(knobWidth), motionMaxLabel, motionMaxSlider);

    const int smallBoxWidth = juce::jlimit(120, 150, bottomRow.getWidth() / 7);
    const int modifierWidth = juce::jlimit(140, 170, bottomRow.getWidth() / 6);
    const int rerollWidth = 126;

    auto rateCell = bottomRow.removeFromLeft(smallBoxWidth);
    placeBox(rateCell, rateModeLabel, rateModeCombo);
    bottomRow.removeFromLeft(controlGap);

    auto noteCell = bottomRow.removeFromLeft(smallBoxWidth);
    placeBox(noteCell, syncNoteLabel, syncNoteCombo);
    bottomRow.removeFromLeft(controlGap);

    auto modifierCell = bottomRow.removeFromLeft(modifierWidth);
    placeBox(modifierCell, syncModifierLabel, syncModifierCombo);
    bottomRow.removeFromLeft(controlGap);

    auto rerollCell = bottomRow.removeFromRight(rerollWidth);
    rerollCell.removeFromTop(labelHeight + 5);
    rerollButton.setBounds(rerollCell.removeFromTop(40).reduced(2, 0));
    bottomRow.removeFromRight(controlGap);

    seedLabel.setBounds(bottomRow.removeFromTop(labelHeight));
    bottomRow.removeFromTop(5);
    seedSlider.setBounds(bottomRow.removeFromTop(comboHeight).reduced(2, 0));
}

void ResonancePanelComponent::configureSlider(juce::Slider& slider, bool rotary)
{
    slider.setSliderStyle(rotary ? juce::Slider::RotaryHorizontalVerticalDrag : juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(rotary ? juce::Slider::TextBoxBelow : juce::Slider::TextBoxRight,
                           false,
                           rotary ? 84 : 112,
                           valueHeight);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111821));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2f9de8));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
}

void ResonancePanelComponent::configureSeedSlider()
{
    seedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    seedSlider.setRange(0.0, static_cast<double>(std::numeric_limits<int>::max()), 1.0);
    seedSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    seedSlider.setNumDecimalPlacesToDisplay(0);
    seedSlider.setVelocityBasedMode(false);
    seedSlider.setScrollWheelEnabled(false);
    seedSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff3bb8ff));
    seedSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff111821));
    seedSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    seedSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111821));
    seedSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2f9de8));
    seedSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
}

void ResonancePanelComponent::configureValueFormatters()
{
    const auto twoDecimals = [](double value) { return juce::String(value, 2); };

    amountSlider.textFromValueFunction = twoDecimals;
    randomnessSlider.textFromValueFunction = twoDecimals;
    qMaxSlider.textFromValueFunction = twoDecimals;
    motionMaxSlider.textFromValueFunction = twoDecimals;
    countMaxSlider.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value));
    };
}

void ResonancePanelComponent::refreshFormattedSliderText()
{
    auto refresh = [](juce::Slider& slider) {
        slider.setValue(slider.getValue(), juce::dontSendNotification);
        slider.updateText();
        slider.repaint();
    };

    refresh(amountSlider);
    refresh(randomnessSlider);
    refresh(countMaxSlider);
    refresh(qMaxSlider);
    refresh(motionMaxSlider);
    repaint();
}

void ResonancePanelComponent::configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
}

void ResonancePanelComponent::configureTooltips()
{
    setLabelTooltip(amountLabel, "Dry/Wet mix. Controls how much processed signal is blended with the original.");
    setLabelTooltip(randomnessLabel, "Macro chaos control. Increases resonance variation, motion, Q spread and pattern intensity.");
    setLabelTooltip(orderLabel, "Changes processing order: EQ before resonances or resonances before EQ.");
    setLabelTooltip(countMaxLabel, "Maximum number of generated resonance peaks.");
    setLabelTooltip(qMaxLabel, "Maximum resonance sharpness. Higher values create narrower, stronger peaks.");
    setLabelTooltip(motionMaxLabel, "Controls how much resonance frequencies, gains and Q values move over time.");
    setLabelTooltip(rateModeLabel, "Sync follows host tempo. Free uses an independent movement rate.");
    setLabelTooltip(syncNoteLabel, "Tempo-synced movement division.");
    setLabelTooltip(syncModifierLabel, "Straight, dotted or triplet timing modifier.");
    setLabelTooltip(seedLabel, "Changes the generated resonance pattern.");
    rerollButton.setTooltip("Generates a new seed and new resonance pattern.");
}

void ResonancePanelComponent::setLabelTooltip(juce::Label& label, const juce::String& text)
{
    label.setTooltip(text);
}

void ResonancePanelComponent::updateSyncModifierFromParameters()
{
    const auto dotted = parameters.getRawParameterValue("syncDotted");
    const auto triplet = parameters.getRawParameterValue("syncTriplet");
    const bool isDotted = dotted != nullptr && dotted->load() >= 0.5f;
    const bool isTriplet = triplet != nullptr && triplet->load() >= 0.5f;

    syncModifierCombo.setSelectedId(isDotted ? 2 : (isTriplet ? 3 : 1), juce::dontSendNotification);
}

void ResonancePanelComponent::setBoolParameter(const juce::String& id, bool value)
{
    if (auto* parameter = parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(value ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}
