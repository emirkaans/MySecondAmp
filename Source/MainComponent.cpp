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

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();

    reverbProcessor.reset();

    const int delayBufferSize = (int)(currentSampleRate * 2.0);
    delayBuffer.setSize(2, delayBufferSize);
    delayBuffer.clear();
    delayWritePosition = 0;
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;

    if (buffer == nullptr)
        return;

    const int startSample = bufferToFill.startSample;
    const int numSamples = bufferToFill.numSamples;
    const int numChannels = juce::jmin(2, buffer->getNumChannels());

    if (numChannels <= 0)
        return;

    float maxLevel = 0.0f;

    const int delayBufferLength = delayBuffer.getNumSamples();
    const int delaySamples = juce::jlimit(1, delayBufferLength - 1,
        (int)(currentSampleRate * (delayTimeMs / 1000.0f)));

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const int readPosition = (delayWritePosition + delayBufferLength - delaySamples) % delayBufferLength;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* channelData = buffer->getWritePointer(channel, startSample);
            float* delayData = delayBuffer.getWritePointer(channel);

            float processedSample = channelData[sampleIndex];

            if (std::abs(processedSample) < gateValue)
                processedSample *= 0.08f;

            processedSample *= inputGain;
            processedSample = processDriveSample(processedSample);

            if (channel == 0)
            {
                processedSample = bassFilterLeft.processSingleSampleRaw(processedSample);
                processedSample = midFilterLeft.processSingleSampleRaw(processedSample);
                processedSample = trebleFilterLeft.processSingleSampleRaw(processedSample);
            }
            else
            {
                processedSample = bassFilterRight.processSingleSampleRaw(processedSample);
                processedSample = midFilterRight.processSingleSampleRaw(processedSample);
                processedSample = trebleFilterRight.processSingleSampleRaw(processedSample);
            }

            processedSample = processToneSample(processedSample, channel);

            const float delayedSample = delayData[readPosition];
            const float outputSample = (processedSample * (1.0f - delayMixValue)) + (delayedSample * delayMixValue);

            delayData[delayWritePosition] = processedSample + (delayedSample * delayFeedback);

            const float finalSample = outputSample * masterValue;
            channelData[sampleIndex] = finalSample;

            const float absoluteValue = std::abs(finalSample);
            if (absoluteValue > maxLevel)
                maxLevel = absoluteValue;
        }

        ++delayWritePosition;
        if (delayWritePosition >= delayBufferLength)
            delayWritePosition = 0;
    }

    if (numChannels >= 2)
    {
        float* leftChannel = buffer->getWritePointer(0, startSample);
        float* rightChannel = buffer->getWritePointer(1, startSample);
        reverbProcessor.processStereo(leftChannel, rightChannel, numSamples);
    }
    else
    {
        float* monoChannel = buffer->getWritePointer(0, startSample);
        reverbProcessor.processMono(monoChannel, numSamples);
    }

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
    g.fillRoundedRectangle((float)meterX, (float)meterY, (float)meterWidth, (float)meterHeight, 8.0f);

    const int filledHeight = (int)(meterHeight * juce::jlimit(0.0f, 1.0f, currentLevel));
    const int filledY = meterY + (meterHeight - filledHeight);

    g.setColour(juce::Colour::fromRGB(180, 120, 255));
    g.fillRoundedRectangle((float)meterX, (float)filledY, (float)meterWidth, (float)filledHeight, 8.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawRoundedRectangle((float)meterX, (float)meterY, (float)meterWidth, (float)meterHeight, 8.0f, 1.0f);
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
        inputGain = (float)gainSlider.getValue();
    }
    else if (slider == &toneSlider)
    {
        toneValue = (float)toneSlider.getValue();
        updateToneCoefficient();
    }
    else if (slider == &driveSlider)
    {
        driveValue = (float)driveSlider.getValue();
    }
    else if (slider == &bassSlider)
    {
        bassValue = (float)bassSlider.getValue();
        updateEqFilters();
    }
    else if (slider == &midSlider)
    {
        midValue = (float)midSlider.getValue();
        updateEqFilters();
    }
    else if (slider == &trebleSlider)
    {
        trebleValue = (float)trebleSlider.getValue();
        updateEqFilters();
    }
    else if (slider == &masterSlider)
    {
        masterValue = (float)masterSlider.getValue();
    }
    else if (slider == &gateSlider)
    {
        gateValue = (float)gateSlider.getValue();
    }
    else if (slider == &reverbSlider)
    {
        reverbValue = (float)reverbSlider.getValue();
        updateReverbParameters();
    }
    else if (slider == &delayTimeSlider)
    {
        delayTimeMs = (float)delayTimeSlider.getValue();
    }
    else if (slider == &delayMixSlider)
    {
        delayMixValue = (float)delayMixSlider.getValue();
    }
}

void MainComponent::timerCallback()
{
    meterLabel.setText("Input Level: " + juce::String(currentLevel, 2),
        juce::dontSendNotification);

    delayFeedback = 0.18f + (delayMixValue * 0.45f);
    repaint();
}

float MainComponent::processDriveSample(float inputSample) const
{
    const float preGain = 1.0f + (driveValue * 42.0f);
    const float clippedSample = std::tanh(inputSample * preGain);

    const float hardEdge = juce::jlimit(-0.95f, 0.95f, clippedSample * (1.15f + driveValue));
    const float blend = 0.15f + (driveValue * 0.85f);
    const float mixed = (inputSample * (1.0f - blend)) + (hardEdge * blend);

    const float outputTrim = 1.0f / (1.0f + driveValue * 4.2f);
    return mixed * outputTrim * 1.55f;
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
        * cutoffFrequency / (float)currentSampleRate);
    toneCoefficient = 1.0f - x;
}

void MainComponent::updateEqFilters()
{
    const auto bassCoefficients = juce::IIRCoefficients::makeLowShelf(currentSampleRate, 180.0, 0.707f, juce::Decibels::decibelsToGain(bassValue));
    const auto midCoefficients = juce::IIRCoefficients::makePeakFilter(currentSampleRate, 900.0, 0.8f, juce::Decibels::decibelsToGain(midValue));
    const auto trebleCoefficients = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 2800.0, 0.707f, juce::Decibels::decibelsToGain(trebleValue));

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