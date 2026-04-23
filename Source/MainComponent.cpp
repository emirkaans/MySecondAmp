#include "MainComponent.h"

namespace
{
    void setupSectionLabel(juce::Label& label, juce::Component& parent, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(225, 215, 255));
        label.setFont(juce::Font(16.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centredLeft);
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
}

MainComponent::MainComponent()
{
    setSize(1180, 760);
    setAudioChannels(2, 2);

    controlsViewport.setViewedComponent(&controlsContent, false);
    controlsViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(controlsViewport);

    audioSettingsButton.onClick = [this]()
    {
        openAudioSettingsWindow();
    };
    addAndMakeVisible(audioSettingsButton);

    setupSlider(gainSlider, gainLabel, "Input Gain", 0.0, 4.0, 0.01, 1.0);
    setupSlider(toneSlider, toneLabel, "Tone", 0.0, 1.0, 0.01, 0.55);
    setupSlider(driveSlider, driveLabel, "Drive", 0.0, 1.0, 0.01, 0.22);

    setupSlider(bassSlider, bassLabel, "Bass", -18.0, 18.0, 0.1, 0.0);
    setupSlider(midSlider, midLabel, "Mid", -18.0, 18.0, 0.1, 0.0);
    setupSlider(trebleSlider, trebleLabel, "Treble", -18.0, 18.0, 0.1, 0.0);

    setupSlider(masterSlider, masterLabel, "Master", 0.0, 1.2, 0.01, 0.85);
    setupSlider(gateSlider, gateLabel, "Noise Gate", 0.0, 0.12, 0.001, 0.025);

    setupSlider(reverbSlider, reverbLabel, "Reverb", 0.0, 1.0, 0.01, 0.12);
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time (ms)", 50.0, 900.0, 1.0, 320.0);
    setupSlider(delayMixSlider, delayMixLabel, "Delay Mix", 0.0, 0.65, 0.01, 0.18);

    setupSectionLabel(inputSectionLabel, controlsContent, "INPUT");
    setupSectionLabel(ampSectionLabel, controlsContent, "AMP");
    setupSectionLabel(eqSectionLabel, controlsContent, "EQ");
    setupSectionLabel(fxSectionLabel, controlsContent, "FX");

    meterLabel.setText("Input Level: 0.00", juce::dontSendNotification);
    meterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    meterLabel.setJustificationType(juce::Justification::centredLeft);
    controlsContent.addAndMakeVisible(meterLabel);

    audioSettingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(90, 70, 130));
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
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
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType(juce::Justification::centredLeft);
    controlsContent.addAndMakeVisible(label);

    slider.setRange(minValue, maxValue, interval);
    slider.setValue(startValue);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 24);
    slider.addListener(this);
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

        const float envelopeAttack = 0.20f;
        const float envelopeRelease = 0.005f;

        if (rectifiedSample > gateEnvelope)
            gateEnvelope += envelopeAttack * (rectifiedSample - gateEnvelope);
        else
            gateEnvelope += envelopeRelease * (rectifiedSample - gateEnvelope);

        const float targetGateGain = (gateEnvelope > gateValue) ? 1.0f : 0.0f;
        const float gateSmoothing = 0.02f;
        gateGain += gateSmoothing * (targetGateGain - gateGain);

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

void MainComponent::drawLevelMeter(juce::Graphics& graphics, juce::Rectangle<int> meterArea) const
{
    graphics.setColour(juce::Colour::fromRGB(70, 64, 88));
    graphics.fillRoundedRectangle(meterArea.toFloat(), 8.0f);

    const float normalizedLevel = juce::jlimit(0.0f, 1.0f, currentLevel);
    const int filledHeight = static_cast<int>(meterArea.getHeight() * normalizedLevel);

    juce::Rectangle<int> filledArea(
        meterArea.getX(),
        meterArea.getBottom() - filledHeight,
        meterArea.getWidth(),
        filledHeight
    );

    graphics.setColour(juce::Colour::fromRGB(180, 120, 255));
    graphics.fillRoundedRectangle(filledArea.toFloat(), 8.0f);

    graphics.setColour(juce::Colours::white.withAlpha(0.9f));
    graphics.drawRoundedRectangle(meterArea.toFloat(), 8.0f, 1.0f);
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(16, 16, 20));

    auto bounds = getLocalBounds().reduced(12);
    auto headerArea = bounds.removeFromTop(70);
    auto mainPanelArea = bounds.reduced(6);

    const bool isCompactHeader = getWidth() < 760;

    graphics.setColour(juce::Colour::fromRGB(40, 34, 52));
    graphics.fillRoundedRectangle(mainPanelArea.toFloat(), 18.0f);

    auto titleArea = headerArea.removeFromLeft(isCompactHeader ? headerArea.getWidth() : headerArea.getWidth() / 2);
    auto subtitleArea = headerArea;

    graphics.setColour(juce::Colours::white);
    graphics.setFont(isCompactHeader ? 24.0f : 30.0f);
    graphics.drawText("MySecondAmp", titleArea, juce::Justification::centredLeft);

    if (!isCompactHeader)
    {
        graphics.setColour(juce::Colour::fromRGB(210, 190, 255));
        graphics.setFont(15.0f);
        graphics.drawText("Mor ve Otesi modu: Drive + Delay + Reverb",
                          subtitleArea,
                          juce::Justification::centredRight);
    }

    drawLevelMeter(graphics, levelMeterBounds);
}

void MainComponent::layoutControlsSingleColumn(int contentWidth)
{
    constexpr int leftPadding = 16;
    constexpr int topPadding = 16;
    constexpr int sectionHeight = 24;
    constexpr int labelHeight = 24;
    constexpr int sliderHeight = 42;
    constexpr int sectionGap = 14;
    constexpr int controlGapAfterLabel = 26;
    constexpr int controlGapAfterSlider = 54;
    constexpr int meterHeight = 190;
    constexpr int meterWidth = 18;

    const int usableWidth = juce::jmax(220, contentWidth - 32);
    const int sliderWidth = usableWidth - 34;

    int currentY = topPadding;

    auto placeSection = [&](juce::Label& sectionLabel)
    {
        sectionLabel.setBounds(leftPadding, currentY, usableWidth, sectionHeight);
        currentY += sectionHeight + 8;
    };

    auto placeControl = [&](juce::Label& label, juce::Slider& slider)
    {
        label.setBounds(leftPadding, currentY, sliderWidth, labelHeight);
        currentY += controlGapAfterLabel;

        slider.setBounds(leftPadding, currentY, sliderWidth, sliderHeight);
        currentY += controlGapAfterSlider;
    };

    placeSection(inputSectionLabel);
    placeControl(gainLabel, gainSlider);
    placeControl(gateLabel, gateSlider);

    currentY += sectionGap;
    placeSection(ampSectionLabel);
    placeControl(driveLabel, driveSlider);
    placeControl(toneLabel, toneSlider);
    placeControl(masterLabel, masterSlider);

    currentY += sectionGap;
    placeSection(eqSectionLabel);
    placeControl(bassLabel, bassSlider);
    placeControl(midLabel, midSlider);
    placeControl(trebleLabel, trebleSlider);

    currentY += sectionGap;
    placeSection(fxSectionLabel);
    placeControl(reverbLabel, reverbSlider);
    placeControl(delayTimeLabel, delayTimeSlider);
    placeControl(delayMixLabel, delayMixSlider);

    meterLabel.setBounds(leftPadding, currentY, sliderWidth, 28);
    currentY += 36;

    levelMeterBounds = juce::Rectangle<int>(leftPadding, currentY, meterWidth, meterHeight);
    currentY += meterHeight + 20;

    controlsContentHeight = juce::jmax(currentY, controlsViewport.getHeight() + 20);
    controlsContent.setSize(contentWidth, controlsContentHeight);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto headerArea = bounds.removeFromTop(70);
    auto contentArea = bounds.reduced(6);

    const int buttonWidth = getWidth() < 700 ? 130 : 150;
    const int buttonHeight = 34;

    audioSettingsButton.setBounds(
        headerArea.getRight() - buttonWidth,
        headerArea.getY() + 10,
        buttonWidth,
        buttonHeight
    );

    controlsViewport.setBounds(contentArea);

    auto viewportBounds = controlsViewport.getLocalBounds().reduced(18);
    const int contentWidth = juce::jmax(260, viewportBounds.getWidth() - 12);

    layoutControlsSingleColumn(contentWidth);
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
        gateValue = static_cast<float>(gateSlider.getValue());
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
    meterLabel.setText("Input Level: " + juce::String(currentLevel, 2),
                       juce::dontSendNotification);

    repaint(levelMeterBounds.expanded(6));
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