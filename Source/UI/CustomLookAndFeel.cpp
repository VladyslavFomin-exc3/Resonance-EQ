#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff3bb8ff));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1f5a7c));
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111821));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff5f6c7a));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f2430));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3bb8ff));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Label::textColourId, juce::Colours::white);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff111821));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2f9de8));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff7ed7ff));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0d1219));
    setColour(juce::PopupMenu::textColourId, juce::Colours::white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff1f6fb8));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xee0b1017));
    setColour(juce::TooltipWindow::outlineColourId, juce::Colour(0xff3bb8ff));
    setColour(juce::TooltipWindow::textColourId, juce::Colours::white.withAlpha(0.92f));
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x + 2.0f, y + 2.0f, width - 4.0f, height - 4.0f);
    if (slider.getTextBoxPosition() == juce::Slider::TextBoxBelow)
        bounds.removeFromBottom(24.0f);

    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 8.0f;
    const auto radius = diameter * 0.5f;
    const auto centreX = bounds.getCentreX();
    const auto centreY = bounds.getCentreY();
    const auto rx = centreX - radius;
    const auto ry = centreY - radius;
    const auto rw = radius * 2.0f;

    juce::Graphics::ScopedSaveState saveState(g);
    g.reduceClipRegion(juce::Rectangle<int>(juce::roundToInt(rx - 3.0f),
                                            juce::roundToInt(ry - 3.0f),
                                            juce::roundToInt(rw + 6.0f),
                                            juce::roundToInt(rw + 6.0f)));

    g.setColour(juce::Colour(0xff1c2533));
    g.fillEllipse(rx, ry, rw, rw);

    g.setColour(juce::Colour(0xff2c4562));
    g.drawEllipse(rx, ry, rw, rw, 1.6f);

    const float lineThickness = juce::jmax(3.0f, radius * 0.12f);
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius * 0.84f, radius * 0.84f, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(juce::Colour(0xff3bb8ff));
    g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool toggled = button.getToggleState();
    const bool isAccent = backgroundColour.getBrightness() > 0.34f || toggled;
    const auto base = (isAccent ? backgroundColour : juce::Colour(0xff111821))
                          .interpolatedWith(juce::Colours::black, shouldDrawButtonAsDown ? 0.05f : 0.18f);

    drawDarkPanel(g, bounds, 6.0f);

    g.setColour(base.withAlpha(isAccent ? 0.92f : 0.72f));
    g.fillRoundedRectangle(bounds.reduced(2.0f), 5.0f);

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colours::white.withAlpha(shouldDrawButtonAsDown ? 0.13f : 0.08f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 5.0f);
    }

    drawBlueAccentBorder(g, bounds, 6.0f, shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown || isAccent);
}

void CustomLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    const auto colour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                                  : juce::TextButton::textColourOffId)
                            .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f);
    drawPixelLikeText(g, button.getButtonText().toUpperCase(), button.getLocalBounds().reduced(6, 2),
                      juce::Justification::centred, colour, (float)button.getHeight() * 0.42f, 1);
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return getArcadeFont((float)buttonHeight * 0.42f);
}

void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    const auto alpha = label.isEnabled() ? 1.0f : 0.5f;
    const auto bounds = label.getLocalBounds();
    const auto background = label.findColour(juce::Label::backgroundColourId);

    if (! background.isTransparent())
    {
        auto panelBounds = bounds.toFloat().reduced(0.5f);
        drawDarkPanel(g, panelBounds, 5.0f);
        drawBlueAccentBorder(g, panelBounds, 5.0f, label.hasKeyboardFocus(false));
    }

    if (! label.isBeingEdited())
    {
        auto textArea = getLabelBorderSize(label).subtractedFrom(bounds).reduced(2, 0);
        const auto size = juce::jmax(9.0f, label.getFont().getHeight());
        drawPixelLikeText(g, label.getText().toUpperCase(), textArea, label.getJustificationType(),
                          label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha), size,
                          juce::jmax(1, (int)((float)textArea.getHeight() / size)));
    }
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH);

    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
    const bool hot = box.isMouseOverOrDragging() || box.hasKeyboardFocus(false) || isButtonDown;

    drawDarkPanel(g, bounds, 6.0f);

    if (hot)
    {
        g.setColour(juce::Colour(0xff3bb8ff).withAlpha(0.13f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 5.0f);
    }

    drawBlueAccentBorder(g, bounds, 6.0f, hot);

    auto arrowZone = juce::Rectangle<float>((float)width - 28.0f, 0.0f, 22.0f, (float)height);
    const auto centre = arrowZone.getCentre();
    juce::Path chevron;
    chevron.startNewSubPath(centre.x - 5.0f, centre.y - 2.0f);
    chevron.lineTo(centre.x, centre.y + 3.5f);
    chevron.lineTo(centre.x + 5.0f, centre.y - 2.0f);

    g.setColour((hot ? juce::Colours::white : box.findColour(juce::ComboBox::arrowColourId))
                    .withAlpha(box.isEnabled() ? 1.0f : 0.35f));
    g.strokePath(chevron, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return getArcadeFont(juce::jmin(14.0f, (float)box.getHeight() * 0.42f));
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& labelToPosition)
{
    labelToPosition.setBounds(8, 1, box.getWidth() - 36, box.getHeight() - 2);
    labelToPosition.setFont(getComboBoxFont(box));
    labelToPosition.setJustificationType(juce::Justification::centred);
    labelToPosition.setColour(juce::Label::textColourId, juce::Colours::white);
    labelToPosition.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    labelToPosition.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return getArcadeFont(14.0f);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(juce::Colour(0xff0d1219));

    g.setColour(juce::Colour(0xff1f5a7c).withAlpha(0.25f));
    for (int y = 0; y < height; y += 6)
        g.fillRect(0, y, width, 1);

    g.setColour(juce::Colour(0xff3bb8ff).withAlpha(0.75f));
    g.drawRect(0, 0, width, height, 1);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText, const juce::Drawable* icon,
                                          const juce::Colour* textColour)
{
    juce::ignoreUnused(shortcutKeyText);

    if (isSeparator)
    {
        g.setColour(juce::Colour(0xff3bb8ff).withAlpha(0.32f));
        g.fillRect(area.reduced(8, 0).withHeight(1).withY(area.getCentreY()));
        return;
    }

    auto itemArea = area.reduced(3, 2);
    if (isHighlighted && isActive)
    {
        g.setColour(juce::Colour(0xff1f6fb8));
        g.fillRoundedRectangle(itemArea.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(itemArea.toFloat().reduced(0.5f), 4.0f, 1.0f);
    }

    auto content = area.reduced(10, 0);
    auto iconArea = content.removeFromLeft(18);
    content.removeFromLeft(4);

    const auto colour = (textColour != nullptr ? *textColour : juce::Colours::white)
                            .withAlpha(isActive ? 1.0f : 0.42f);
    g.setColour(colour);

    if (icon != nullptr)
        icon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    else if (isTicked)
    {
        juce::Path tick;
        tick.startNewSubPath((float)iconArea.getX() + 3.0f, (float)iconArea.getCentreY());
        tick.lineTo((float)iconArea.getCentreX() - 1.0f, (float)iconArea.getBottom() - 5.0f);
        tick.lineTo((float)iconArea.getRight() - 3.0f, (float)iconArea.getY() + 5.0f);
        g.strokePath(tick, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    if (hasSubMenu)
    {
        auto arrow = content.removeFromRight(14).toFloat();
        juce::Path path;
        path.startNewSubPath(arrow.getX() + 3.0f, arrow.getCentreY() - 5.0f);
        path.lineTo(arrow.getRight() - 3.0f, arrow.getCentreY());
        path.lineTo(arrow.getX() + 3.0f, arrow.getCentreY() + 5.0f);
        g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    drawPixelLikeText(g, text.toUpperCase(), content, juce::Justification::centredLeft, colour, 14.0f, 1);
}

void CustomLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight, int& idealWidth,
                                                  int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 80;
        idealHeight = 8;
        return;
    }

    const auto font = getPopupMenuFont();
    idealHeight = juce::jmax(30, standardMenuItemHeight);
    idealWidth = juce::GlyphArrangement::getStringWidthInt(font, text.toUpperCase()) + idealHeight * 2;
}

juce::Rectangle<int> CustomLookAndFeel::getTooltipBounds(const juce::String& tipText,
                                                         juce::Point<int> screenPos,
                                                         juce::Rectangle<int> parentArea)
{
    const auto font = juce::Font(juce::FontOptions{15.0f});
    constexpr float maxTextWidth = 340.0f;
    constexpr int horizontalPadding = 20;
    constexpr int verticalPadding = 14;

    juce::AttributedString attributed;
    attributed.setText(tipText);
    attributed.setFont(font);
    attributed.setColour(findColour(juce::TooltipWindow::textColourId));
    attributed.setJustification(juce::Justification::centredLeft);
    attributed.setWordWrap(juce::AttributedString::byWord);

    const auto singleLineWidth = juce::TextLayout::getStringWidth(attributed);
    const auto textWidth = juce::jlimit(80.0f, maxTextWidth, singleLineWidth);

    juce::TextLayout layout;
    layout.createLayout(attributed, textWidth);

    const auto width = juce::roundToInt(layout.getWidth()) + horizontalPadding;
    const auto height = juce::roundToInt(layout.getHeight()) + verticalPadding;
    const auto x = screenPos.x > parentArea.getCentreX() ? screenPos.x - width - 12 : screenPos.x + 14;
    const auto y = screenPos.y > parentArea.getCentreY() ? screenPos.y - height - 12 : screenPos.y + 14;

    return juce::Rectangle<int>(x, y, width, height).constrainedWithin(parentArea);
}

void CustomLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);

    g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(findColour(juce::TooltipWindow::outlineColourId).withAlpha(0.82f));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    auto textArea = juce::Rectangle<int>(width, height).reduced(9, 6);
    g.setColour(findColour(juce::TooltipWindow::textColourId));
    g.setFont(juce::Font(juce::FontOptions{15.0f, juce::Font::plain}));
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 4, 0.9f);
}

juce::Font CustomLookAndFeel::getArcadeFont(float size) const
{
    return juce::Font(juce::FontOptions{size, juce::Font::bold})
        .withHorizontalScale(0.94f)
        .withExtraKerningFactor(0.04f);
}

void CustomLookAndFeel::drawPixelLikeText(juce::Graphics& g, const juce::String& text,
                                          juce::Rectangle<int> area, juce::Justification justification,
                                          juce::Colour colour, float fontSize, int maxLines) const
{
    auto font = getArcadeFont(fontSize);
    g.setFont(font);

    const auto clippedText = text.toUpperCase();
    const auto shadowArea = area.translated(1, 1);
    g.setColour(juce::Colours::black.withAlpha(0.72f * colour.getFloatAlpha()));
    g.drawFittedText(clippedText, shadowArea, justification, maxLines, 0.78f);

    g.setColour(colour);
    g.drawFittedText(clippedText, area, justification, maxLines, 0.78f);
}

void CustomLookAndFeel::drawDarkPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize) const
{
    g.setColour(juce::Colour(0xff0b1017));
    g.fillRoundedRectangle(bounds, cornerSize);

    juce::ColourGradient gradient(juce::Colour(0xff1c2533), bounds.getTopLeft(),
                                  juce::Colour(0xff0f151d), bounds.getBottomLeft(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.reduced(1.0f), juce::jmax(0.0f, cornerSize - 1.0f));
}

void CustomLookAndFeel::drawBlueAccentBorder(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float cornerSize, bool bright) const
{
    const auto accent = juce::Colour(0xff3bb8ff);

    if (bright)
    {
        g.setColour(accent.withAlpha(0.18f));
        g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 2.0f);
    }

    g.setColour(accent.withAlpha(bright ? 0.95f : 0.58f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, bright ? 1.7f : 1.2f);

    auto topArc = bounds.reduced(4.0f, 3.0f);
    juce::Path highlight;
    highlight.addRoundedRectangle(topArc.getX(), topArc.getY(), topArc.getWidth(), topArc.getHeight(),
                                  cornerSize, cornerSize, true, true, false, false);
    g.setColour(juce::Colours::white.withAlpha(bright ? 0.18f : 0.09f));
    g.strokePath(highlight, juce::PathStrokeType(1.0f));
}
