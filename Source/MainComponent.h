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
    // --- Setup helpers ---
    void setupSlider(juce::Slider& slider,
                     juce::Label& label,
                     const juce::String& text,
                     double minValue, double maxValue,
                     double interval, double startValue);

    void setupBypassButton(juce::TextButton& button, juce::Component& parent);

    // --- DSP updates ---
    void updateToneCoefficient();
    void updateEqFilters();
    void updateReverbParameters();

    // --- Per-sample / block DSP ---
    float processGateSample  (float inputSample);
    float processDriveSample (float inputSample) const;
    float processToneSample  (float inputSample, int channel);
    void  processDelayBlock  (float* leftData, float* rightData, int numSamples);

    // --- Layout / draw ---
    void layoutAmpControls (juce::Rectangle<int> contentArea);
    void drawLevelMeter    (juce::Graphics& graphics, juce::Rectangle<int> meterArea) const;
    void openAudioSettingsWindow();
    void drawAmpBackground (juce::Graphics& graphics, juce::Rectangle<int> area);
    void drawAmpHeader     (juce::Graphics& graphics, juce::Rectangle<int> area);
    void drawAmpFooter     (juce::Graphics& graphics, juce::Rectangle<int> area);

    // --- Preset ---
    void savePreset();
    void loadPreset();
    void applyPreset(const juce::XmlElement& xml);

    // =========================================================================
    // Components
    // =========================================================================
    std::unique_ptr<AmpLookAndFeel> ampLookAndFeel;

    juce::Viewport  controlsViewport;
    juce::Component controlsContent;

    juce::TextButton audioSettingsButton { "Audio Settings" };
    juce::TextButton savePresetButton    { "Save Preset"    };
    juce::TextButton loadPresetButton    { "Load Preset"    };

    // Input
    juce::Slider gainSlider;   juce::Label gainLabel;
    juce::Slider gateSlider;   juce::Label gateLabel;

    // Amp
    juce::Slider driveSlider;  juce::Label driveLabel;
    juce::Slider toneSlider;   juce::Label toneLabel;
    juce::Slider masterSlider; juce::Label masterLabel;

    // EQ
    juce::Slider bassSlider;   juce::Label bassLabel;
    juce::Slider midSlider;    juce::Label midLabel;
    juce::Slider trebleSlider; juce::Label trebleLabel;

    // FX
    juce::Slider reverbSlider;    juce::Label reverbLabel;
    juce::Slider delayTimeSlider; juce::Label delayTimeLabel;
    juce::Slider delayMixSlider;  juce::Label delayMixLabel;

    // Bypass buttons (toggle)
    juce::TextButton eqBypassButton     { "EQ"  };
    juce::TextButton reverbBypassButton { "REV" };
    juce::TextButton delayBypassButton  { "DLY" };

    // Labels
    juce::Label meterLabel;
    juce::Label clipLabel;
    juce::Label inputSectionLabel;
    juce::Label ampSectionLabel;
    juce::Label eqSectionLabel;
    juce::Label fxSectionLabel;
    juce::Label bypassSectionLabel;

    // =========================================================================
    // DSP state
    // =========================================================================
    float inputGain       = 1.0f;
    float toneValue       = 0.55f;
    float driveValue      = 0.22f;
    float bassValue       = 0.0f;
    float midValue        = 0.0f;
    float trebleValue     = 0.0f;
    float masterValue     = 0.85f;
    float gateThresholdDb = -55.0f;
    float reverbValue     = 0.12f;
    float delayTimeMs     = 320.0f;
    float delayMixValue   = 0.18f;

    // Bypass flags (audio thread reads these atomically enough for this use)
    bool eqBypassed     = false;
    bool reverbBypassed = false;
    bool delayBypassed  = false;

    // Level / clip
    float currentLevel  = 0.0f;
    bool  clipping      = false;
    int   clipHoldTimer = 0;       // countdown in timer ticks (30 Hz)

    double currentSampleRate = 44100.0;

    // Tone (1-pole LP per channel)
    float toneCoefficient = 0.15f;
    float toneStateLeft   = 0.0f;
    float toneStateRight  = 0.0f;

    // Noise gate
    float gateEnvelope = 0.0f;
    float gateGain     = 1.0f;

    // EQ filters – true stereo
    juce::IIRFilter bassFilterLeft,   bassFilterRight;
    juce::IIRFilter midFilterLeft,    midFilterRight;
    juce::IIRFilter trebleFilterLeft, trebleFilterRight;

    // Reverb
    juce::Reverb             reverbProcessor;
    juce::Reverb::Parameters reverbParameters;

    // Delay
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;

    // File chooser (must outlive the async callback)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // =========================================================================
    // Layout bounds (MainComponent coordinate space)
    // =========================================================================
    juce::Rectangle<int> levelMeterBounds;
    juce::Rectangle<int> ampBodyBounds;
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> footerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};