#include "MainComponent.h"

MainComponent::MainComponent()
    : audioSetupComponent(deviceManager,
        0, 2,
        0, 2,
        true,
        false,
        true,
        false)
{
    setSize(1360, 820);
    setAudioChannels(2, 2);

    addAndMakeVisible(audioSetupComponent);

    controlsViewport.setViewedComponent(&controlsContent, false);
    controlsViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(controlsViewport);

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

    meterLabel.setText("Input Level: 0.00", juce::dontSendNotification);
    meterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    meterLabel.setJustificationType(juce::Justification::centredLeft);
    controlsContent.addAndMakeVisible(meterLabel);

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

        // Smooth noise gate
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

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(16, 16, 20));

    auto bounds = getLocalBounds().reduced(12);
    auto headerArea = bounds.removeFromTop(56);
    auto leftArea = bounds.removeFromLeft(760);
    auto rightArea = bounds.reduced(12, 0);

    g.setColour(juce::Colour::fromRGB(30, 30, 36));
    g.fillRoundedRectangle(leftArea.toFloat(), 18.0f);

    g.setColour(juce::Colour::fromRGB(40, 34, 52));
    g.fillRoundedRectangle(rightArea.toFloat(), 18.0f);

    g.setColour(juce::Colours::white);
    g.setFont(30.0f);
    g.drawText("MySecondAmp",
        headerArea.removeFromLeft(340),
        juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(210, 190, 255));
    g.setFont(15.0f);
    g.drawText("Mor ve Otesi modu: Drive + Delay + Reverb",
        headerArea,
        juce::Justification::centredRight);

    auto viewportBounds = controlsViewport.getBounds().reduced(18, 18);

    const int meterX = viewportBounds.getRight() - 44;
    const int meterY = viewportBounds.getY() + 460;
    const int meterWidth = 18;
    const int meterHeight = 220;

    g.setColour(juce::Colour::fromRGB(70, 64, 88));
    g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(meterY),
        static_cast<float>(meterWidth), static_cast<float>(meterHeight), 8.0f);

    const int filledHeight = static_cast<int>(meterHeight * juce::jlimit(0.0f, 1.0f, currentLevel));
    const int filledY = meterY + (meterHeight - filledHeight);

    g.setColour(juce::Colour::fromRGB(180, 120, 255));
    g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(filledY),
        static_cast<float>(meterWidth), static_cast<float>(filledHeight), 8.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawRoundedRectangle(static_cast<float>(meterX), static_cast<float>(meterY),
        static_cast<float>(meterWidth), static_cast<float>(meterHeight), 8.0f, 1.0f);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(24);
    bounds.removeFromTop(64);

    auto leftPanel = bounds.removeFromLeft(760);
    auto rightPanel = bounds.reduced(12, 0);

    audioSetupComponent.setBounds(leftPanel.reduced(12));
    controlsViewport.setBounds(rightPanel);

    auto contentArea = rightPanel.reduced(24);
    const int contentWidth = contentArea.getWidth() - 14;

    int currentY = 12;

    auto placeControl = [&contentWidth, &currentY](juce::Label& label, juce::Slider& slider)
        {
            label.setBounds(12, currentY, contentWidth - 24, 24);
            currentY += 26;

            slider.setBounds(12, currentY, contentWidth - 24, 42);
            currentY += 56;
        };

    placeControl(gainLabel, gainSlider);
    placeControl(toneLabel, toneSlider);
    placeControl(driveLabel, driveSlider);

    currentY += 4;

    placeControl(bassLabel, bassSlider);
    placeControl(midLabel, midSlider);
    placeControl(trebleLabel, trebleSlider);

    currentY += 4;

    placeControl(masterLabel, masterSlider);
    placeControl(gateLabel, gateSlider);
    placeControl(reverbLabel, reverbSlider);
    placeControl(delayTimeLabel, delayTimeSlider);
    placeControl(delayMixLabel, delayMixSlider);

    currentY += 4;
    meterLabel.setBounds(12, currentY, contentWidth - 24, 28);
    currentY += 40;

    controlsContentHeight = juce::jmax(currentY + 20, controlsViewport.getHeight() + 40);
    controlsContent.setSize(contentWidth, controlsContentHeight);
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &gainSlider)
    {
        inputGain = static_cast<float>(gainSlider.getValue());
    }
    else if (slider == &toneSlider)
    {
        toneValue = static_cast<float>(toneSlider.getValue());
        updateToneCoefficient();
    }
    else if (slider == &driveSlider)
    {
        driveValue = static_cast<float>(driveSlider.getValue());
    }
    else if (slider == &bassSlider)
    {
        bassValue = static_cast<float>(bassSlider.getValue());
        updateEqFilters();
    }
    else if (slider == &midSlider)
    {
        midValue = static_cast<float>(midSlider.getValue());
        updateEqFilters();
    }
    else if (slider == &trebleSlider)
    {
        trebleValue = static_cast<float>(trebleSlider.getValue());
        updateEqFilters();
    }
    else if (slider == &masterSlider)
    {
        masterValue = static_cast<float>(masterSlider.getValue());
    }
    else if (slider == &gateSlider)
    {
        gateValue = static_cast<float>(gateSlider.getValue());
    }
    else if (slider == &reverbSlider)
    {
        reverbValue = static_cast<float>(reverbSlider.getValue());
        updateReverbParameters();
    }
    else if (slider == &delayTimeSlider)
    {
        delayTimeMs = static_cast<float>(delayTimeSlider.getValue());
    }
    else if (slider == &delayMixSlider)
    {
        delayMixValue = static_cast<float>(delayMixSlider.getValue());
    }
}

void MainComponent::timerCallback()
{
    meterLabel.setText("Input Level: " + juce::String(currentLevel, 2),
        juce::dontSendNotification);

    repaint();
}

float MainComponent::processDriveSample(float inputSample) const
{
    // Tight metal-style pre-emphasis:
    // lows biraz sıkılaşsın, pick attack öne gelsin
    const float tightInput = inputSample * (1.0f + driveValue * 6.0f);

    // Stage 1: soft saturation
    const float stageOne = std::tanh(tightInput * (1.8f + driveValue * 4.0f));

    // Stage 2: more aggressive clipping
    const float stageTwoInput = stageOne * (1.5f + driveValue * 6.0f);
    const float stageTwo = std::tanh(stageTwoInput);

    // Blend dry çok az kalsın, ama tamamen de yok olmasın
    const float dryBlend = 0.10f;
    const float wetBlend = 1.0f - dryBlend;
    const float mixedSample = (inputSample * dryBlend) + (stageTwo * wetBlend);

    // Output trim
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