#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <memory>

class AmpLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& graphics,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& graphics,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;
};

class MainComponent : public juce::AudioAppComponent,
                      public juce::Slider::Listener,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* changedSlider) override;
    void timerCallback() override;

private:
    void setupSlider(juce::Slider& slider,
                     juce::Label& label,
                     const juce::String& text,
                     double minValue,
                     double maxValue,
                     double interval,
                     double startValue);

    void updateToneCoefficient();
    void updateEqFilters();
    void updateReverbParameters();

    float processDriveSample(float inputSample) const;
    float processToneSample(float inputSample, int channel);

    void layoutAmpControls(juce::Rectangle<int> contentArea);
    void drawLevelMeter(juce::Graphics& graphics, juce::Rectangle<int> meterArea) const;
    void openAudioSettingsWindow();

    void drawAmpBackground(juce::Graphics& graphics, juce::Rectangle<int> area);
    void drawAmpHeader(juce::Graphics& graphics, juce::Rectangle<int> area);
    void drawAmpFooter(juce::Graphics& graphics, juce::Rectangle<int> area);

    std::unique_ptr<AmpLookAndFeel> ampLookAndFeel;

    juce::Viewport controlsViewport;
    juce::Component controlsContent;

    juce::TextButton audioSettingsButton { "Audio Settings" };

    juce::Slider gainSlider;
    juce::Label gainLabel;

    juce::Slider toneSlider;
    juce::Label toneLabel;

    juce::Slider driveSlider;
    juce::Label driveLabel;

    juce::Slider bassSlider;
    juce::Label bassLabel;

    juce::Slider midSlider;
    juce::Label midLabel;

    juce::Slider trebleSlider;
    juce::Label trebleLabel;

    juce::Slider masterSlider;
    juce::Label masterLabel;

    juce::Slider gateSlider;
    juce::Label gateLabel;

    juce::Slider reverbSlider;
    juce::Label reverbLabel;

    juce::Slider delayTimeSlider;
    juce::Label delayTimeLabel;

    juce::Slider delayMixSlider;
    juce::Label delayMixLabel;

    juce::Label meterLabel;

    juce::Label inputSectionLabel;
    juce::Label ampSectionLabel;
    juce::Label eqSectionLabel;
    juce::Label fxSectionLabel;

    float inputGain = 1.0f;
    float toneValue = 0.55f;
    float driveValue = 0.22f;
    float bassValue = 0.0f;
    float midValue = 0.0f;
    float trebleValue = 0.0f;
    float masterValue = 0.85f;

    float gateThresholdDb = -55.0f;
    float reverbValue = 0.12f;
    float delayTimeMs = 320.0f;
    float delayMixValue = 0.18f;
    float currentLevel = 0.0f;

    double currentSampleRate = 44100.0;

    float toneCoefficient = 0.15f;
    float toneStateLeft = 0.0f;
    float toneStateRight = 0.0f;

    float gateEnvelope = 0.0f;
    float gateGain = 1.0f;

    juce::IIRFilter bassFilterLeft;
    juce::IIRFilter bassFilterRight;

    juce::IIRFilter midFilterLeft;
    juce::IIRFilter midFilterRight;

    juce::IIRFilter trebleFilterLeft;
    juce::IIRFilter trebleFilterRight;

    juce::Reverb reverbProcessor;
    juce::Reverb::Parameters reverbParameters;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;
    float delayFeedback = 0.32f;

    juce::Rectangle<int> levelMeterBounds;
    juce::Rectangle<int> ampBodyBounds;
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};