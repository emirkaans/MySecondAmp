#include "MainComponent.h"

namespace
{
    void setupSectionLabel(juce::Label& label, juce::Component& parent, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(214, 183, 102));
        label.setFont(juce::Font(15.5f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        parent.addAndMakeVisible(label);
    }

    class AudioSettingsContent final : public juce::Component
    {
    public:
        explicit AudioSettingsContent(juce::AudioDeviceManager& audioDeviceManager)
            : audioSetupComponent(audioDeviceManager,
                                  0, 2,
                                  0, 2,
                                  true,
                                  false,
                                  true,
                                  false)
        {
            addAndMakeVisible(audioSetupComponent);
            setSize(560, 420);
        }

        void resized() override
        {
            audioSetupComponent.setBounds(getLocalBounds().reduced(12));
        }

    private:
        juce::AudioDeviceSelectorComponent audioSetupComponent;
    };

    float clamp01(float value)
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }

    void styleValueLabel(juce::Label& label)
    {
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(238, 230, 210));
        label.setFont(juce::Font(13.5f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
    }

    void styleRotarySliderTextbox(juce::Slider& slider)
    {
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(245, 238, 220));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(28, 28, 30));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(110, 100, 80));
    }
}

void AmpLookAndFeel::drawRotarySlider(juce::Graphics& graphics,
                                      int x,
                                      int y,
                                      int width,
                                      int height,
                                      float sliderPosProportional,
                                      float rotaryStartAngle,
                                      float rotaryEndAngle,
                                      juce::Slider& slider)
{
    juce::ignoreUnused(slider);

    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                               static_cast<float>(y),
                                               static_cast<float>(width),
                                               static_cast<float>(height)).reduced(8.0f);

    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knobBounds = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());

    graphics.setColour(juce::Colour::fromRGB(20, 20, 22));
    graphics.fillEllipse(knobBounds.translated(2.5f, 4.0f));

    juce::ColourGradient knobGradient(juce::Colour::fromRGB(72, 72, 76),
                                      knobBounds.getCentreX(),
                                      knobBounds.getY(),
                                      juce::Colour::fromRGB(24, 24, 26),
                                      knobBounds.getCentreX(),
                                      knobBounds.getBottom(),
                                      false);
    graphics.setGradientFill(knobGradient);
    graphics.fillEllipse(knobBounds);

    graphics.setColour(juce::Colour::fromRGB(120, 120, 126));
    graphics.drawEllipse(knobBounds, 1.5f);

    const auto arcBounds = knobBounds.expanded(8.0f);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(arcBounds.getCentreX(),
                                arcBounds.getCentreY(),
                                arcBounds.getWidth() * 0.5f,
                                arcBounds.getHeight() * 0.5f,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    graphics.setColour(juce::Colour::fromRGB(80, 72, 54));
    graphics.strokePath(backgroundArc, juce::PathStrokeType(3.0f));

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Path valueArc;
    valueArc.addCentredArc(arcBounds.getCentreX(),
                           arcBounds.getCentreY(),
                           arcBounds.getWidth() * 0.5f,
                           arcBounds.getHeight() * 0.5f,
                           0.0f,
                           rotaryStartAngle,
                           angle,
                           true);

    graphics.setColour(juce::Colour::fromRGB(230, 190, 90));
    graphics.strokePath(valueArc, juce::PathStrokeType(3.2f));

    juce::Path pointer;
    const float pointerLength = diameter * 0.34f;
    const float pointerThickness = 3.0f;

    pointer.addRoundedRectangle(-pointerThickness * 0.5f,
                                -pointerLength,
                                pointerThickness,
                                pointerLength,
                                1.5f);

    graphics.setColour(juce::Colour::fromRGB(245, 220, 160));
    graphics.fillPath(pointer,
                      juce::AffineTransform::rotation(angle)
                          .translated(knobBounds.getCentreX(), knobBounds.getCentreY()));

    graphics.setColour(juce::Colour::fromRGB(185, 185, 188));
    graphics.fillEllipse(knobBounds.getCentreX() - 4.0f,
                         knobBounds.getCentreY() - 4.0f,
                         8.0f,
                         8.0f);
}

void AmpLookAndFeel::drawButtonBackground(juce::Graphics& graphics,
                                          juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool isMouseOverButton,
                                          bool isButtonDown)
{
    juce::ignoreUnused(backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    juce::Colour baseColour = juce::Colour::fromRGB(55, 55, 58);

    if (isButtonDown)
        baseColour = baseColour.brighter(0.18f);
    else if (isMouseOverButton)
        baseColour = baseColour.brighter(0.08f);

    graphics.setColour(juce::Colours::black.withAlpha(0.35f));
    graphics.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 8.0f);

    juce::ColourGradient buttonGradient(baseColour.brighter(0.12f),
                                        bounds.getCentreX(),
                                        bounds.getY(),
                                        baseColour.darker(0.18f),
                                        bounds.getCentreX(),
                                        bounds.getBottom(),
                                        false);
    graphics.setGradientFill(buttonGradient);
    graphics.fillRoundedRectangle(bounds, 8.0f);

    graphics.setColour(juce::Colour::fromRGB(120, 120, 125));
    graphics.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

MainComponent::MainComponent()
{
    ampLookAndFeel = std::make_unique<AmpLookAndFeel>();
    setLookAndFeel(ampLookAndFeel.get());

    setSize(1280, 760);
    setAudioChannels(2, 2);

    controlsViewport.setViewedComponent(&controlsContent, false);
    controlsViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(controlsViewport);

    audioSettingsButton.onClick = [this]()
    {
        openAudioSettingsWindow();
    };
    addAndMakeVisible(audioSettingsButton);

    setupSlider(gainSlider, gainLabel, "INPUT", 0.0, 4.0, 0.01, 1.0);
    setupSlider(toneSlider, toneLabel, "TONE", 0.0, 1.0, 0.01, 0.55);
    setupSlider(driveSlider, driveLabel, "DRIVE", 0.0, 1.0, 0.01, 0.22);

    setupSlider(bassSlider, bassLabel, "BASS", -18.0, 18.0, 0.1, 0.0);
    setupSlider(midSlider, midLabel, "MID", -18.0, 18.0, 0.1, 0.0);
    setupSlider(trebleSlider, trebleLabel, "TREBLE", -18.0, 18.0, 0.1, 0.0);

    setupSlider(masterSlider, masterLabel, "MASTER", 0.0, 1.2, 0.01, 0.85);
    setupSlider(gateSlider, gateLabel, "GATE", -80.0, -20.0, 1.0, -55.0);

    setupSlider(reverbSlider, reverbLabel, "REVERB", 0.0, 1.0, 0.01, 0.12);
    setupSlider(delayTimeSlider, delayTimeLabel, "DELAY", 50.0, 900.0, 1.0, 320.0);
    setupSlider(delayMixSlider, delayMixLabel, "MIX", 0.0, 0.65, 0.01, 0.18);

    gateSlider.setTextValueSuffix(" dB");
    delayTimeSlider.setTextValueSuffix(" ms");

    setupSectionLabel(inputSectionLabel, controlsContent, "INPUT");
    setupSectionLabel(ampSectionLabel, controlsContent, "AMP");
    setupSectionLabel(eqSectionLabel, controlsContent, "EQ");
    setupSectionLabel(fxSectionLabel, controlsContent, "FX");

    meterLabel.setText("LEVEL", juce::dontSendNotification);
    meterLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(220, 210, 180));
    meterLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    meterLabel.setJustificationType(juce::Justification::centred);
    // meterLabel artık controlsContent'e değil, doğrudan MainComponent'e ekleniyor
    addAndMakeVisible(meterLabel);

    audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250, 245, 235));

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    ampLookAndFeel.reset();
    shutdownAudio();
}

void MainComponent::setupSlider(juce::Slider& slider,
                                juce::Label& label,
                                const juce::String& text,
                                double minValue,
                                double maxValue,
                                double interval,
                                double startValue)
{
    label.setText(text, juce::dontSendNotification);
    styleValueLabel(label);
    controlsContent.addAndMakeVisible(label);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::degreesToRadians(210.0f),
                               juce::degreesToRadians(510.0f),
                               true);
    slider.setRange(minValue, maxValue, interval);
    slider.setValue(startValue);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.addListener(this);
    styleRotarySliderTextbox(slider);
    controlsContent.addAndMakeVisible(slider);
}

void MainComponent::openAudioSettingsWindow()
{
    auto* settingsContent = new AudioSettingsContent(deviceManager);

    juce::DialogWindow::LaunchOptions dialogOptions;
    dialogOptions.content.setOwned(settingsContent);
    dialogOptions.dialogTitle = "Audio Settings";
    dialogOptions.dialogBackgroundColour = juce::Colour::fromRGB(28, 28, 34);
    dialogOptions.escapeKeyTriggersCloseButton = true;
    dialogOptions.useNativeTitleBar = true;
    dialogOptions.resizable = true;

    if (auto* parentComponent = getTopLevelComponent())
        dialogOptions.componentToCentreAround = parentComponent;

    dialogOptions.launchAsync();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);

    currentSampleRate = sampleRate;
    toneStateLeft = 0.0f;
    toneStateRight = 0.0f;

    gateEnvelope = 0.0f;
    gateGain = 1.0f;

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();

    reverbProcessor.reset();

    const int delayBufferSize = static_cast<int>(currentSampleRate * 2.0);
    delayBuffer.setSize(2, delayBufferSize);
    delayBuffer.clear();
    delayWritePosition = 0;
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* audioBuffer = bufferToFill.buffer;

    if (audioBuffer == nullptr)
        return;

    const int startSample = bufferToFill.startSample;
    const int numSamples = bufferToFill.numSamples;
    const int numChannels = audioBuffer->getNumChannels();

    if (numChannels <= 0)
        return;

    auto* leftChannelData = audioBuffer->getWritePointer(0, startSample);
    float* rightChannelData = (numChannels > 1) ? audioBuffer->getWritePointer(1, startSample) : nullptr;

    const int delayBufferLength = delayBuffer.getNumSamples();

    if (delayBufferLength <= 1)
        return;

    const int delaySamples = juce::jlimit(
        1,
        delayBufferLength - 1,
        static_cast<int>(currentSampleRate * (delayTimeMs / 1000.0f))
    );

    const float currentDelayFeedback = 0.18f + (delayMixValue * 0.45f);

    float* delayLeftData = delayBuffer.getWritePointer(0);
    float* delayRightData = delayBuffer.getWritePointer(1);

    float maxLevel = 0.0f;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        float inputSample = leftChannelData[sampleIndex];

        const float rectifiedSample = std::abs(inputSample);

        const float envelopeAttack = 0.08f;
        const float envelopeRelease = 0.0025f;

        if (rectifiedSample > gateEnvelope)
            gateEnvelope += envelopeAttack * (rectifiedSample - gateEnvelope);
        else
            gateEnvelope += envelopeRelease * (rectifiedSample - gateEnvelope);

        const float safeEnvelope = juce::jmax(gateEnvelope, 1.0e-6f);
        const float envelopeDb = juce::Decibels::gainToDecibels(safeEnvelope, -120.0f);

        const float fullCloseDb = gateThresholdDb - 12.0f;
        const float fullOpenDb = gateThresholdDb + 6.0f;

        float targetGateGain = 1.0f;

        if (envelopeDb <= fullCloseDb)
        {
            targetGateGain = 0.0f;
        }
        else if (envelopeDb < fullOpenDb)
        {
            const float normalizedOpenAmount =
                (envelopeDb - fullCloseDb) / (fullOpenDb - fullCloseDb);

            const float curvedOpenAmount = normalizedOpenAmount * normalizedOpenAmount;
            targetGateGain = clamp01(curvedOpenAmount);
        }

        const float gateOpenSmoothing = 0.06f;
        const float gateCloseSmoothing = 0.008f;

        if (targetGateGain > gateGain)
            gateGain += gateOpenSmoothing * (targetGateGain - gateGain);
        else
            gateGain += gateCloseSmoothing * (targetGateGain - gateGain);

        inputSample *= gateGain;

        inputSample *= inputGain;
        inputSample = processDriveSample(inputSample);

        inputSample = bassFilterLeft.processSingleSampleRaw(inputSample);
        inputSample = midFilterLeft.processSingleSampleRaw(inputSample);
        inputSample = trebleFilterLeft.processSingleSampleRaw(inputSample);

        inputSample = processToneSample(inputSample, 0);

        const int readPosition = (delayWritePosition + delayBufferLength - delaySamples) % delayBufferLength;

        const float delayedLeftSample = delayLeftData[readPosition];
        const float monoOutputSample = (inputSample * (1.0f - delayMixValue))
                                     + (delayedLeftSample * delayMixValue);

        delayLeftData[delayWritePosition] = inputSample + (delayedLeftSample * currentDelayFeedback);
        delayRightData[delayWritePosition] = delayLeftData[delayWritePosition];

        const float finalSample = monoOutputSample * masterValue;

        leftChannelData[sampleIndex] = finalSample;

        if (rightChannelData != nullptr)
            rightChannelData[sampleIndex] = finalSample;

        const float absoluteValue = std::abs(finalSample);
        if (absoluteValue > maxLevel)
            maxLevel = absoluteValue;

        ++delayWritePosition;
        if (delayWritePosition >= delayBufferLength)
            delayWritePosition = 0;
    }

    if (rightChannelData != nullptr)
        reverbProcessor.processStereo(leftChannelData, rightChannelData, numSamples);
    else
        reverbProcessor.processMono(leftChannelData, numSamples);

    currentLevel = maxLevel;
}

void MainComponent::releaseResources()
{
}

void MainComponent::drawAmpBackground(juce::Graphics& graphics, juce::Rectangle<int> area)
{
    juce::ColourGradient outerGradient(juce::Colour::fromRGB(30, 22, 14),
                                       area.getTopLeft().toFloat(),
                                       juce::Colour::fromRGB(10, 10, 10),
                                       area.getBottomRight().toFloat(),
                                       false);
    graphics.setGradientFill(outerGradient);
    graphics.fillRoundedRectangle(area.toFloat(), 26.0f);

    graphics.setColour(juce::Colour::fromRGB(86, 58, 26).withAlpha(0.85f));
    graphics.drawRoundedRectangle(area.toFloat().reduced(1.5f), 26.0f, 3.0f);

    auto innerArea = area.reduced(14);

    graphics.setColour(juce::Colour::fromRGB(24, 24, 24));
    graphics.fillRoundedRectangle(innerArea.toFloat(), 18.0f);

    for (int lineY = innerArea.getY() + 8; lineY < innerArea.getBottom(); lineY += 12)
    {
        graphics.setColour(juce::Colours::black.withAlpha(0.18f));
        graphics.drawLine(static_cast<float>(innerArea.getX() + 8),
                          static_cast<float>(lineY),
                          static_cast<float>(innerArea.getRight() - 8),
                          static_cast<float>(lineY),
                          1.0f);
    }
}

void MainComponent::drawAmpHeader(juce::Graphics& graphics, juce::Rectangle<int> area)
{
    juce::ColourGradient brassGradient(juce::Colour::fromRGB(205, 168, 88),
                                       area.getTopLeft().toFloat(),
                                       juce::Colour::fromRGB(120, 88, 32),
                                       area.getBottomRight().toFloat(),
                                       false);
    graphics.setGradientFill(brassGradient);
    graphics.fillRoundedRectangle(area.toFloat(), 18.0f);

    auto highlightArea = area.removeFromTop(16);
    graphics.setColour(juce::Colour::fromRGB(255, 238, 185).withAlpha(0.30f));
    graphics.fillRoundedRectangle(highlightArea.toFloat(), 18.0f);

    graphics.setColour(juce::Colour::fromRGB(74, 48, 18));
    graphics.drawRoundedRectangle(headerBounds.toFloat(), 18.0f, 1.4f);

    graphics.setColour(juce::Colour::fromRGB(36, 24, 10));
    graphics.setFont(juce::Font(30.0f, juce::Font::bold));
    graphics.drawText("MySecondAmp",
                      headerBounds.getX() + 24,
                      headerBounds.getY() + 12,
                      headerBounds.getWidth() - 220,
                      34,
                      juce::Justification::centredLeft);

    graphics.setFont(juce::Font(12.5f, juce::Font::plain));
    graphics.drawText("Custom Guitar Amplifier",
                      headerBounds.getX() + 28,
                      headerBounds.getBottom() - 28,
                      220,
                      18,
                      juce::Justification::centredLeft);
}

void MainComponent::drawAmpFooter(juce::Graphics& graphics, juce::Rectangle<int> area)
{
    graphics.setColour(juce::Colour::fromRGB(18, 18, 18));
    graphics.fillRoundedRectangle(area.toFloat(), 14.0f);

    for (int index = 0; index < 6; ++index)
    {
        const int x = area.getX() + 24 + index * 28;
        graphics.setColour(juce::Colour::fromRGB(40, 40, 40));
        graphics.fillEllipse(static_cast<float>(x), static_cast<float>(area.getCentreY() - 5), 10.0f, 10.0f);

        graphics.setColour(juce::Colour::fromRGB(12, 12, 12));
        graphics.drawEllipse(static_cast<float>(x), static_cast<float>(area.getCentreY() - 5), 10.0f, 10.0f, 1.0f);
    }

    graphics.setColour(juce::Colour::fromRGB(160, 150, 120));
    graphics.setFont(juce::Font(12.0f, juce::Font::bold));
    graphics.drawText("POWER - STANDBY - ANALOG RESPONSE",
                      area.getX() + 210,
                      area.getY(),
                      area.getWidth() - 220,
                      area.getHeight(),
                      juce::Justification::centredLeft);
}

void MainComponent::drawLevelMeter(juce::Graphics& graphics, juce::Rectangle<int> meterArea) const
{
    graphics.setColour(juce::Colour::fromRGB(18, 18, 18));
    graphics.fillRoundedRectangle(meterArea.toFloat(), 8.0f);

    auto innerMeterArea = meterArea.reduced(3);

    juce::ColourGradient meterBackground(juce::Colour::fromRGB(20, 20, 20),
                                         innerMeterArea.getTopLeft().toFloat(),
                                         juce::Colour::fromRGB(12, 18, 12),
                                         innerMeterArea.getBottomLeft().toFloat(),
                                         false);
    graphics.setGradientFill(meterBackground);
    graphics.fillRoundedRectangle(innerMeterArea.toFloat(), 6.0f);

    const float boostedLevel = juce::jlimit(0.0f, 1.0f, std::pow(currentLevel * 2.2f, 0.68f));
    const int filledWidth = static_cast<int>(innerMeterArea.getWidth() * boostedLevel);

    juce::Rectangle<int> filledArea(
        innerMeterArea.getX(),
        innerMeterArea.getY(),
        filledWidth,
        innerMeterArea.getHeight()
    );

    juce::ColourGradient levelGradient(juce::Colour::fromRGB(90, 255, 120),
                                       filledArea.getTopLeft().toFloat(),
                                       juce::Colour::fromRGB(255, 210, 70),
                                       filledArea.getCentre().toFloat(),
                                       false);
    levelGradient.addColour(1.0, juce::Colour::fromRGB(255, 70, 70));

    graphics.setGradientFill(levelGradient);
    graphics.fillRoundedRectangle(filledArea.toFloat(), 5.0f);

    graphics.setColour(juce::Colour::fromRGB(95, 95, 95));
    graphics.drawRoundedRectangle(meterArea.toFloat(), 8.0f, 1.0f);

    graphics.setColour(juce::Colours::white.withAlpha(0.12f));
    for (int index = 1; index < 10; ++index)
    {
        const float x = static_cast<float>(innerMeterArea.getX())
                      + (static_cast<float>(innerMeterArea.getWidth()) / 10.0f) * static_cast<float>(index);

        graphics.drawLine(x,
                          static_cast<float>(innerMeterArea.getY() + 2),
                          x,
                          static_cast<float>(innerMeterArea.getBottom() - 2),
                          1.0f);
    }
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(12, 12, 12));

    drawAmpBackground(graphics, ampBodyBounds);
    drawAmpHeader(graphics, headerBounds);
    drawAmpFooter(graphics, footerBounds);
    drawLevelMeter(graphics, levelMeterBounds);

    graphics.setColour(juce::Colour::fromRGB(50, 36, 16));
    graphics.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(ampBodyBounds.getX() + 18),
                                                         static_cast<float>(ampBodyBounds.getBottom() - 22),
                                                         static_cast<float>(ampBodyBounds.getWidth() - 36),
                                                         8.0f),
                                  4.0f);

    graphics.setColour(juce::Colour::fromRGB(255, 245, 215).withAlpha(0.12f));
    graphics.fillRoundedRectangle(headerBounds.toFloat().translated(0.0f, 2.0f).reduced(10.0f, 8.0f), 12.0f);
}

void MainComponent::layoutAmpControls(juce::Rectangle<int> contentArea)
{
    const int sectionLabelHeight = 26;
    const int knobWidth = 106;
    const int knobHeight = 126;
    const int blockVerticalPadding = 8;
    const int topMargin = 16;
    const int bottomMargin = 12;
    const int leftRightMargin = 20;
    const int rowGap = 18;
    const int columnGap = 18;

    auto workArea = contentArea.reduced(leftRightMargin, topMargin);
    workArea.removeFromBottom(bottomMargin);

    // İki knob satırı
    auto firstRow = workArea.removeFromTop(170);
    workArea.removeFromTop(rowGap);
    auto secondRow = workArea.removeFromTop(170);

    auto inputArea = firstRow.removeFromLeft(firstRow.getWidth() / 2);
    auto ampArea = firstRow;

    secondRow.removeFromLeft(60);
    secondRow.removeFromRight(60);

    auto eqArea = secondRow.removeFromLeft(secondRow.getWidth() / 2);
    auto fxArea = secondRow;

    auto placeGroup = [&](juce::Rectangle<int> groupArea,
                          juce::Label& sectionLabel,
                          std::initializer_list<std::pair<juce::Label*, juce::Slider*>> controls)
    {
        groupArea.reduce(4, blockVerticalPadding);

        auto titleArea = groupArea.removeFromTop(sectionLabelHeight);
        sectionLabel.setBounds(titleArea);

        const int controlCount = static_cast<int>(controls.size());
        if (controlCount <= 0)
            return;

        const int totalControlWidth = controlCount * knobWidth;
        const int totalGapWidth = juce::jmax(0, controlCount - 1) * columnGap;
        const int totalUsedWidth = totalControlWidth + totalGapWidth;

        int currentX = groupArea.getX() + juce::jmax(0, (groupArea.getWidth() - totalUsedWidth) / 2);
        const int labelY = groupArea.getY() + 2;
        const int sliderY = labelY + 18;

        for (const auto& controlPair : controls)
        {
            auto* currentLabel = controlPair.first;
            auto* currentSlider = controlPair.second;

            currentLabel->setBounds(currentX, labelY, knobWidth, 20);
            currentSlider->setBounds(currentX, sliderY, knobWidth, knobHeight);

            currentX += knobWidth + columnGap;
        }
    };

    placeGroup(inputArea, inputSectionLabel, {
        { &gainLabel, &gainSlider },
        { &gateLabel, &gateSlider }
    });

    placeGroup(ampArea, ampSectionLabel, {
        { &driveLabel, &driveSlider },
        { &toneLabel, &toneSlider },
        { &masterLabel, &masterSlider }
    });

    placeGroup(eqArea, eqSectionLabel, {
        { &bassLabel, &bassSlider },
        { &midLabel, &midSlider },
        { &trebleLabel, &trebleSlider }
    });

    placeGroup(fxArea, fxSectionLabel, {
        { &reverbLabel, &reverbSlider },
        { &delayTimeLabel, &delayTimeSlider },
        { &delayMixLabel, &delayMixSlider }
    });

    controlsContent.setSize(contentArea.getWidth(), contentArea.getHeight());
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(18);

    ampBodyBounds = bounds;

    auto innerBounds = ampBodyBounds.reduced(20, 18);
    headerBounds = innerBounds.removeFromTop(86);

    footerBounds = juce::Rectangle<int>(ampBodyBounds.getX() + 22,
                                        ampBodyBounds.getBottom() - 56,
                                        ampBodyBounds.getWidth() - 44,
                                        32);

    audioSettingsButton.setBounds(headerBounds.getRight() - 170,
                                  headerBounds.getY() + 12,
                                  150,
                                  32);

    // Footer'ın üstünde, knob'ların altında kalan boşluğu meter bölgesine ayır
    // controlsArea: knob'lar için
    // meterArea: controlsArea'nın altında, footer'ın üstünde
    const int meterZoneHeight = 46;     // label + meter için toplam yükseklik
    const int meterLabelHeight = 18;
    const int meterHeight = 14;
    const int meterWidth = 360;
    const int meterBottomMargin = 16;   // footer'dan yukarı boşluk

    // meter bölgesi: footer'ın hemen üstünde
    const int meterZoneBottom = footerBounds.getY() - meterBottomMargin;
    const int meterZoneTop = meterZoneBottom - meterZoneHeight;

    const int meterCentreX = ampBodyBounds.getCentreX();
    const int meterLabelX = meterCentreX - meterWidth / 2;

    meterLabel.setBounds(meterLabelX,
                         meterZoneTop,
                         meterWidth,
                         meterLabelHeight);

    levelMeterBounds = juce::Rectangle<int>(meterLabelX,
                                            meterZoneTop + meterLabelHeight + 6,
                                            meterWidth,
                                            meterHeight);

    // controlsArea: header'dan hemen sonra, meter zone'un hemen üstüne kadar
    auto controlsArea = juce::Rectangle<int>(
        ampBodyBounds.getX() + 26,
        headerBounds.getBottom() + 14,
        ampBodyBounds.getWidth() - 52,
        meterZoneTop - (headerBounds.getBottom() + 14) - 8
    );

    controlsViewport.setBounds(controlsArea);
    controlsContent.setBounds(controlsViewport.getLocalBounds());

    layoutAmpControls(controlsViewport.getLocalBounds());
}

void MainComponent::sliderValueChanged(juce::Slider* changedSlider)
{
    if (changedSlider == &gainSlider)
    {
        inputGain = static_cast<float>(gainSlider.getValue());
    }
    else if (changedSlider == &toneSlider)
    {
        toneValue = static_cast<float>(toneSlider.getValue());
        updateToneCoefficient();
    }
    else if (changedSlider == &driveSlider)
    {
        driveValue = static_cast<float>(driveSlider.getValue());
    }
    else if (changedSlider == &bassSlider)
    {
        bassValue = static_cast<float>(bassSlider.getValue());
        updateEqFilters();
    }
    else if (changedSlider == &midSlider)
    {
        midValue = static_cast<float>(midSlider.getValue());
        updateEqFilters();
    }
    else if (changedSlider == &trebleSlider)
    {
        trebleValue = static_cast<float>(trebleSlider.getValue());
        updateEqFilters();
    }
    else if (changedSlider == &masterSlider)
    {
        masterValue = static_cast<float>(masterSlider.getValue());
    }
    else if (changedSlider == &gateSlider)
    {
        gateThresholdDb = static_cast<float>(gateSlider.getValue());
    }
    else if (changedSlider == &reverbSlider)
    {
        reverbValue = static_cast<float>(reverbSlider.getValue());
        updateReverbParameters();
    }
    else if (changedSlider == &delayTimeSlider)
    {
        delayTimeMs = static_cast<float>(delayTimeSlider.getValue());
    }
    else if (changedSlider == &delayMixSlider)
    {
        delayMixValue = static_cast<float>(delayMixSlider.getValue());
    }
}

void MainComponent::timerCallback()
{
    repaint(levelMeterBounds.expanded(4));
}

float MainComponent::processDriveSample(float inputSample) const
{
    const float tightInput = inputSample * (1.0f + driveValue * 6.0f);

    const float stageOne = std::tanh(tightInput * (1.8f + driveValue * 4.0f));

    const float stageTwoInput = stageOne * (1.5f + driveValue * 6.0f);
    const float stageTwo = std::tanh(stageTwoInput);

    const float dryBlend = 0.10f;
    const float wetBlend = 1.0f - dryBlend;
    const float mixedSample = (inputSample * dryBlend) + (stageTwo * wetBlend);

    const float outputTrim = 0.55f - (driveValue * 0.12f);

    return mixedSample * outputTrim;
}

float MainComponent::processToneSample(float inputSample, int channel)
{
    float& toneState = (channel == 0) ? toneStateLeft : toneStateRight;
    toneState += toneCoefficient * (inputSample - toneState);
    return toneState;
}

void MainComponent::updateToneCoefficient()
{
    const float minimumCutoff = 320.0f;
    const float maximumCutoff = 8200.0f;
    const float cutoffFrequency = minimumCutoff + (toneValue * (maximumCutoff - minimumCutoff));

    const float x = std::exp(-2.0f * juce::MathConstants<float>::pi
                             * cutoffFrequency / static_cast<float>(currentSampleRate));
    toneCoefficient = 1.0f - x;
}

void MainComponent::updateEqFilters()
{
    const auto bassCoefficients = juce::IIRCoefficients::makeLowShelf(
        currentSampleRate, 180.0, 0.707f, juce::Decibels::decibelsToGain(bassValue));

    const auto midCoefficients = juce::IIRCoefficients::makePeakFilter(
        currentSampleRate, 900.0, 0.8f, juce::Decibels::decibelsToGain(midValue));

    const auto trebleCoefficients = juce::IIRCoefficients::makeHighShelf(
        currentSampleRate, 2800.0, 0.707f, juce::Decibels::decibelsToGain(trebleValue));

    bassFilterLeft.setCoefficients(bassCoefficients);
    bassFilterRight.setCoefficients(bassCoefficients);

    midFilterLeft.setCoefficients(midCoefficients);
    midFilterRight.setCoefficients(midCoefficients);

    trebleFilterLeft.setCoefficients(trebleCoefficients);
    trebleFilterRight.setCoefficients(trebleCoefficients);
}

void MainComponent::updateReverbParameters()
{
    reverbParameters.roomSize = 0.20f + (reverbValue * 0.65f);
    reverbParameters.damping = 0.30f + (reverbValue * 0.35f);
    reverbParameters.wetLevel = reverbValue * 0.28f;
    reverbParameters.dryLevel = 1.0f - (reverbValue * 0.18f);
    reverbParameters.width = 1.0f;
    reverbParameters.freezeMode = 0.0f;

    reverbProcessor.setParameters(reverbParameters);
}