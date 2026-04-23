#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <memory>

class AmpLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& graphics,
                          int x, int y, int width, int height,
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
                      public juce::Button::Listener,
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
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

private:
    void setupSlider(juce::Slider& slider, juce::Label& label,
                     const juce::String& text,
                     double minValue, double maxValue,
                     double interval, double startValue);
    void setupBypassButton(juce::TextButton& button, juce::Component& parent);

    void updateToneCoefficient();
    void updateEqFilters();
    void updateReverbParameters();
    void updatePreampFilters();
    void updatePresenceFilter();
    void updateCabFilters();

    float processGateSample   (float inputSample);
    float processPreampSample (float inputSample);
    float processToneSample   (float inputSample, int channel);
    void  processDelayBlock   (float* leftData, float* rightData, int numSamples);
    void  processCabBlock     (float* leftData, float* rightData, int numSamples);

    void layoutAmpControls (juce::Rectangle<int> contentArea);
    void drawLevelMeter    (juce::Graphics& g, juce::Rectangle<int> meterArea) const;
    void openAudioSettingsWindow();
    void drawAmpBackground (juce::Graphics& g, juce::Rectangle<int> area);
    void drawAmpHeader     (juce::Graphics& g, juce::Rectangle<int> area);
    void drawAmpFooter     (juce::Graphics& g, juce::Rectangle<int> area);

    void savePreset();
    void loadPreset();
    void loadIrFile();
    void applyPreset(const juce::XmlElement& xml);

    // =========================================================================
    std::unique_ptr<AmpLookAndFeel> ampLookAndFeel;

    juce::Viewport  controlsViewport;
    juce::Component controlsContent;

    juce::TextButton audioSettingsButton { "Audio Settings" };
    juce::TextButton savePresetButton    { "Save Preset"    };
    juce::TextButton loadPresetButton    { "Load Preset"    };
    juce::TextButton loadIrButton        { "Load IR"        };

    juce::Slider gainSlider;     juce::Label gainLabel;
    juce::Slider gateSlider;     juce::Label gateLabel;
    juce::Slider driveSlider;    juce::Label driveLabel;
    juce::Slider toneSlider;     juce::Label toneLabel;
    juce::Slider masterSlider;   juce::Label masterLabel;
    juce::Slider presenceSlider; juce::Label presenceLabel;
    juce::Slider bassSlider;     juce::Label bassLabel;
    juce::Slider midSlider;      juce::Label midLabel;
    juce::Slider trebleSlider;   juce::Label trebleLabel;
    juce::Slider reverbSlider;    juce::Label reverbLabel;
    juce::Slider delayTimeSlider; juce::Label delayTimeLabel;
    juce::Slider delayMixSlider;  juce::Label delayMixLabel;

    juce::TextButton eqBypassButton     { "EQ"  };
    juce::TextButton reverbBypassButton { "REV" };
    juce::TextButton delayBypassButton  { "DLY" };
    juce::TextButton cabBypassButton    { "CAB" };

    juce::Label irStatusLabel;
    juce::Label meterLabel;
    juce::Label clipLabel;
    juce::Label inputSectionLabel;
    juce::Label ampSectionLabel;
    juce::Label eqSectionLabel;
    juce::Label fxSectionLabel;
    juce::Label bypassSectionLabel;

    // =========================================================================
    float inputGain       = 1.0f;
    float toneValue       = 0.55f;
    float driveValue      = 0.5f;
    float presenceValue   = 0.5f;
    float bassValue       = 0.0f;
    float midValue        = 0.0f;
    float trebleValue     = 0.0f;
    float masterValue     = 0.85f;
    float gateThresholdDb = -55.0f;
    float reverbValue     = 0.12f;
    float delayTimeMs     = 320.0f;
    float delayMixValue   = 0.18f;

    bool eqBypassed     = false;
    bool reverbBypassed = false;
    bool delayBypassed  = false;
    bool cabBypassed    = false;

    float currentLevel  = 0.0f;
    bool  clipping      = false;
    int   clipHoldTimer = 0;

    double currentSampleRate = 44100.0;

    float toneCoefficient = 0.15f;
    float toneStateLeft   = 0.0f;
    float toneStateRight  = 0.0f;

    float gateEnvelope = 0.0f;
    float gateGain     = 1.0f;

    juce::IIRFilter preHpFilterLeft,      preHpFilterRight;
    juce::IIRFilter bassFilterLeft,       bassFilterRight;
    juce::IIRFilter midFilterLeft,        midFilterRight;
    juce::IIRFilter trebleFilterLeft,     trebleFilterRight;
    juce::IIRFilter presenceFilterLeft,   presenceFilterRight;
    juce::IIRFilter cabLoFilterLeft,      cabLoFilterRight;
    juce::IIRFilter cabHiFilterLeft,      cabHiFilterRight;
    juce::IIRFilter cabMidFilterLeft,     cabMidFilterRight;

    juce::dsp::ProcessSpec   dspSpec;
    juce::dsp::Convolution   convolution;
    juce::AudioBuffer<float> irWorkBuffer;
    bool irLoaded = false;

    juce::Reverb             reverbProcessor;
    juce::Reverb::Parameters reverbParameters;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Rectangle<int> levelMeterBounds;
    juce::Rectangle<int> ampBodyBounds;
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};