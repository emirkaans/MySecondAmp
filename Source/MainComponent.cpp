#include "MainComponent.h"

// NAM includes - must come after JuceHeader to avoid macro conflicts
#include "NAM/get_dsp.h"
#include "NAM/dsp.h"

namespace
{
    void setupSectionLabel(juce::Label& label, juce::Component& parent, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(214, 183, 102));
        label.setFont(juce::Font(juce::FontOptions(15.5f).withStyle("Bold")));
        label.setJustificationType(juce::Justification::centred);
        parent.addAndMakeVisible(label);
    }

    class AudioSettingsContent final : public juce::Component
    {
    public:
        explicit AudioSettingsContent(juce::AudioDeviceManager& mgr)
            : audioSetupComponent(mgr, 0, 2, 0, 2, true, false, true, false)
        {
            addAndMakeVisible(audioSetupComponent);
            setSize(560, 420);
        }
        void resized() override { audioSetupComponent.setBounds(getLocalBounds().reduced(12)); }
    private:
        juce::AudioDeviceSelectorComponent audioSetupComponent;
    };

    float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }

    void styleValueLabel(juce::Label& label)
    {
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(238, 230, 210));
        label.setFont(juce::Font(juce::FontOptions(13.5f).withStyle("Bold")));
        label.setJustificationType(juce::Justification::centred);
    }

    void styleRotarySliderTextbox(juce::Slider& slider)
    {
        slider.setColour(juce::Slider::textBoxTextColourId,       juce::Colour::fromRGB(245, 238, 220));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(28, 28, 30));
        slider.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colour::fromRGB(110, 100, 80));
    }

    // Asymmetric soft-clip: positive half clips harder (tube character)
    inline float asymClip(float x)
    {
        if (x >= 0.0f) return 1.0f - std::exp(-x);
        return -1.0f + std::exp(x * 0.7f);
    }
} // namespace

// =============================================================================
// AmpLookAndFeel
// =============================================================================

void AmpLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                      int x, int y, int width, int height,
                                      float sliderPosProportional,
                                      float rotaryStartAngle, float rotaryEndAngle,
                                      juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    const auto bounds = juce::Rectangle<float>((float)x,(float)y,(float)width,(float)height).reduced(8.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knobBounds = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());

    g.setColour(juce::Colour::fromRGB(20,20,22));
    g.fillEllipse(knobBounds.translated(2.5f, 4.0f));

    juce::ColourGradient kg(juce::Colour::fromRGB(72,72,76), knobBounds.getCentreX(), knobBounds.getY(),
                            juce::Colour::fromRGB(24,24,26), knobBounds.getCentreX(), knobBounds.getBottom(), false);
    g.setGradientFill(kg); g.fillEllipse(knobBounds);
    g.setColour(juce::Colour::fromRGB(120,120,126)); g.drawEllipse(knobBounds, 1.5f);

    const auto arcBounds = knobBounds.expanded(8.0f);
    juce::Path bgArc;
    bgArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                        arcBounds.getWidth()*0.5f, arcBounds.getHeight()*0.5f,
                        0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour::fromRGB(80,72,54));
    g.strokePath(bgArc, juce::PathStrokeType(3.0f));

    const float angle = rotaryStartAngle + sliderPosProportional*(rotaryEndAngle-rotaryStartAngle);
    juce::Path valArc;
    valArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                         arcBounds.getWidth()*0.5f, arcBounds.getHeight()*0.5f,
                         0.0f, rotaryStartAngle, angle, true);
    g.setColour(juce::Colour::fromRGB(230,190,90));
    g.strokePath(valArc, juce::PathStrokeType(3.2f));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.5f, -diameter*0.34f, 3.0f, diameter*0.34f, 1.5f);
    g.setColour(juce::Colour::fromRGB(245,220,160));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle)
                            .translated(knobBounds.getCentreX(), knobBounds.getCentreY()));
    g.setColour(juce::Colour::fromRGB(185,185,188));
    g.fillEllipse(knobBounds.getCentreX()-4.0f, knobBounds.getCentreY()-4.0f, 8.0f, 8.0f);
}

void AmpLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour&, bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour base = button.getToggleState()
                        ? juce::Colour::fromRGB(180,60,40)
                        : juce::Colour::fromRGB(55,55,58);
    if (isButtonDown)     base = base.brighter(0.18f);
    else if (isMouseOver) base = base.brighter(0.08f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(bounds.translated(0.0f,2.0f), 8.0f);
    juce::ColourGradient bg(base.brighter(0.12f), bounds.getCentreX(), bounds.getY(),
                            base.darker(0.18f),   bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bg); g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(juce::Colour::fromRGB(120,120,125));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

MainComponent::MainComponent()
{
    ampLookAndFeel = std::make_unique<AmpLookAndFeel>();
    setLookAndFeel(ampLookAndFeel.get());
    setSize(1280, 760);
    setAudioChannels(2, 2);

    controlsViewport.setViewedComponent(&controlsContent, false);
    controlsViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(controlsViewport);

    // Header buttons
    audioSettingsButton.onClick = [this] { openAudioSettingsWindow(); };
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250,245,235));
    addAndMakeVisible(audioSettingsButton);

    savePresetButton.onClick = [this] { savePreset(); };
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250,245,235));
    addAndMakeVisible(savePresetButton);

    loadPresetButton.onClick = [this] { loadPreset(); };
    loadPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250,245,235));
    addAndMakeVisible(loadPresetButton);

    loadIrButton.onClick = [this] { loadIrFile(); };
    loadIrButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250,245,235));
    addAndMakeVisible(loadIrButton);

    loadNamButton.onClick = [this] { loadNamFile(); };
    loadNamButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(250,245,235));
    addAndMakeVisible(loadNamButton);

    // Sliders
    setupSlider(gainSlider,      gainLabel,      "INPUT",    0.0,   4.0,  0.01,  1.0);
    setupSlider(gateSlider,      gateLabel,      "GATE",   -80.0, -20.0,  1.0, -55.0);
    setupSlider(driveSlider,     driveLabel,     "DRIVE",    0.0,   1.0,  0.01,  0.5);
    setupSlider(toneSlider,      toneLabel,      "TONE",     0.0,   1.0,  0.01,  0.55);
    setupSlider(masterSlider,    masterLabel,    "MASTER",   0.0,   1.2,  0.01,  0.85);
    setupSlider(presenceSlider,  presenceLabel,  "PRESENCE", 0.0,   1.0,  0.01,  0.5);
    setupSlider(bassSlider,      bassLabel,      "BASS",   -18.0,  18.0,  0.1,   0.0);
    setupSlider(midSlider,       midLabel,       "MID",    -18.0,  18.0,  0.1,   0.0);
    setupSlider(trebleSlider,    trebleLabel,    "TREBLE", -18.0,  18.0,  0.1,   0.0);
    setupSlider(reverbSlider,    reverbLabel,    "REVERB",   0.0,   1.0,  0.01,  0.12);
    setupSlider(delayTimeSlider, delayTimeLabel, "DELAY",   50.0, 900.0,  1.0, 320.0);
    setupSlider(delayMixSlider,  delayMixLabel,  "MIX",      0.0,   0.65, 0.01,  0.18);

    gateSlider.setTextValueSuffix(" dB");
    delayTimeSlider.setTextValueSuffix(" ms");

    // Section labels
    setupSectionLabel(inputSectionLabel,  controlsContent, "INPUT");
    setupSectionLabel(ampSectionLabel,    controlsContent, "AMP");
    setupSectionLabel(eqSectionLabel,     controlsContent, "EQ");
    setupSectionLabel(fxSectionLabel,     controlsContent, "FX");
    setupSectionLabel(bypassSectionLabel, controlsContent, "BYPASS");

    // Bypass buttons
    setupBypassButton(eqBypassButton,     controlsContent);
    setupBypassButton(reverbBypassButton, controlsContent);
    setupBypassButton(delayBypassButton,  controlsContent);
    setupBypassButton(cabBypassButton,    controlsContent);
    setupBypassButton(namBypassButton,    controlsContent);

    // Meter label
    meterLabel.setText("LEVEL", juce::dontSendNotification);
    meterLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(220,210,180));
    meterLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    meterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabel);

    // Clip indicator
    clipLabel.setText("CLIP", juce::dontSendNotification);
    clipLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    clipLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    clipLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    clipLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(clipLabel);

    // IR status
    irStatusLabel.setText("No IR loaded", juce::dontSendNotification);
    irStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(180,160,120));
    irStatusLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    irStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(irStatusLabel);

    // NAM status
    namStatusLabel.setText("No NAM loaded", juce::dontSendNotification);
    namStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(140,200,140));
    namStatusLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    namStatusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(namStatusLabel);

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();
    updatePreampFilters();
    updatePresenceFilter();
    updateCabFilters();

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    ampLookAndFeel.reset();
    shutdownAudio();
    // Ensure pending model is destroyed on message thread (safe)
    namModelPending.reset();
    namModel.reset();
}

// =============================================================================
// Setup helpers
// =============================================================================

void MainComponent::setupSlider(juce::Slider& slider, juce::Label& label,
                                const juce::String& text,
                                double minValue, double maxValue,
                                double interval, double startValue)
{
    label.setText(text, juce::dontSendNotification);
    styleValueLabel(label);
    controlsContent.addAndMakeVisible(label);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::degreesToRadians(210.0f), juce::degreesToRadians(510.0f), true);
    slider.setRange(minValue, maxValue, interval);
    slider.setValue(startValue);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.addListener(this);
    styleRotarySliderTextbox(slider);
    controlsContent.addAndMakeVisible(slider);
}

void MainComponent::setupBypassButton(juce::TextButton& button, juce::Component& parent)
{
    button.setClickingTogglesState(true);
    button.setToggleState(false, juce::dontSendNotification);
    button.addListener(this);
    button.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(220,210,180));
    button.setColour(juce::TextButton::textColourOnId,  juce::Colours::white);
    parent.addAndMakeVisible(button);
}

// =============================================================================
// Audio settings
// =============================================================================

void MainComponent::openAudioSettingsWindow()
{
    auto* content = new AudioSettingsContent(deviceManager);
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(content);
    opts.dialogTitle                  = "Audio Settings";
    opts.dialogBackgroundColour       = juce::Colour::fromRGB(28,28,34);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar            = true;
    opts.resizable                    = true;
    if (auto* parent = getTopLevelComponent()) opts.componentToCentreAround = parent;
    opts.launchAsync();
}

// =============================================================================
// Preset save / load
// =============================================================================

void MainComponent::savePreset()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result == juce::File{}) return;
            juce::XmlElement xml("MySecondAmpPreset");
            xml.setAttribute("gain",         gainSlider.getValue());
            xml.setAttribute("gate",         gateSlider.getValue());
            xml.setAttribute("drive",        driveSlider.getValue());
            xml.setAttribute("tone",         toneSlider.getValue());
            xml.setAttribute("master",       masterSlider.getValue());
            xml.setAttribute("presence",     presenceSlider.getValue());
            xml.setAttribute("bass",         bassSlider.getValue());
            xml.setAttribute("mid",          midSlider.getValue());
            xml.setAttribute("treble",       trebleSlider.getValue());
            xml.setAttribute("reverb",       reverbSlider.getValue());
            xml.setAttribute("delayTime",    delayTimeSlider.getValue());
            xml.setAttribute("delayMix",     delayMixSlider.getValue());
            xml.setAttribute("eqBypass",     (int)eqBypassed);
            xml.setAttribute("reverbBypass", (int)reverbBypassed);
            xml.setAttribute("delayBypass",  (int)delayBypassed);
            xml.setAttribute("cabBypass",    (int)cabBypassed);
            xml.setAttribute("namBypass",    (int)namBypassed);
            xml.writeTo(result.withFileExtension("xml"));
        });
}

void MainComponent::loadPreset()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result == juce::File{}) return;
            if (auto xml = juce::XmlDocument::parse(result))
                applyPreset(*xml);
        });
}

void MainComponent::loadIrFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Cabinet IR (WAV)",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.wav;*.WAV");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result == juce::File{}) return;
            convolution.loadImpulseResponse(result,
                                            juce::dsp::Convolution::Stereo::yes,
                                            juce::dsp::Convolution::Trim::yes,
                                            0);
            irLoaded = true;
            juce::MessageManager::callAsync([this, name = result.getFileNameWithoutExtension()]()
            {
                irStatusLabel.setText("IR: " + name, juce::dontSendNotification);
            });
        });
}

void MainComponent::loadNamFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load NAM Model",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.nam");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result == juce::File{}) return;

            // Load on a background thread — NAM parsing can take ~100-500ms
            juce::Thread::launch([this, result]()
            {
                std::unique_ptr<nam::DSP> newModel;
                bool success = false;
                juce::String errorMsg;

                try
                {
                    // Use wstring on Windows to handle Unicode paths correctly
                    const std::wstring namPathW(result.getFullPathName().toWideCharPointer());
                    const std::filesystem::path namPath(namPathW);

                    // Verify file exists before passing to NAM
                    if (!std::filesystem::exists(namPath))
                        throw std::runtime_error("File not found: " + result.getFullPathName().toStdString());

                    newModel = nam::get_dsp(namPath);

                    if (newModel != nullptr)
                    {
                        // Reset: sample rate + buffer size set eder, sonra prewarm çalıştırır
                        newModel->ResetAndPrewarm(currentSampleRate, currentBlockSize);
                        success = true;
                    }
                }
                catch (const std::exception& e)
                {
                    errorMsg = juce::String(e.what());
                }

                juce::MessageManager::callAsync([this, success, errorMsg,
                                                  name = result.getFileNameWithoutExtension(),
                                                  model = std::move(newModel)]() mutable
                {
                    if (success && model != nullptr)
                    {
                        // Hand the model to the audio thread via the pending slot
                        {
                            std::lock_guard<std::mutex> lock(namSwapMutex);
                            namModelPending = std::move(model);
                        }
                        namLoaded = true;
                        namStatusLabel.setText("NAM: " + name, juce::dontSendNotification);
                        namStatusLabel.setColour(juce::Label::textColourId,
                                                  juce::Colour::fromRGB(100, 220, 100));
                    }
                    else
                    {
                        namStatusLabel.setText("NAM load failed: " + errorMsg,
                                               juce::dontSendNotification);
                        namStatusLabel.setColour(juce::Label::textColourId,
                                                  juce::Colour::fromRGB(220, 80, 80));
                    }
                });
            });
        });
}

void MainComponent::applyPreset(const juce::XmlElement& xml)
{
    if (xml.getTagName() != "MySecondAmpPreset") return;
    gainSlider.setValue      (xml.getDoubleAttribute("gain",      1.0));
    gateSlider.setValue      (xml.getDoubleAttribute("gate",    -55.0));
    driveSlider.setValue     (xml.getDoubleAttribute("drive",     0.5));
    toneSlider.setValue      (xml.getDoubleAttribute("tone",      0.55));
    masterSlider.setValue    (xml.getDoubleAttribute("master",    0.85));
    presenceSlider.setValue  (xml.getDoubleAttribute("presence",  0.5));
    bassSlider.setValue      (xml.getDoubleAttribute("bass",      0.0));
    midSlider.setValue       (xml.getDoubleAttribute("mid",       0.0));
    trebleSlider.setValue    (xml.getDoubleAttribute("treble",    0.0));
    reverbSlider.setValue    (xml.getDoubleAttribute("reverb",    0.12));
    delayTimeSlider.setValue (xml.getDoubleAttribute("delayTime", 320.0));
    delayMixSlider.setValue  (xml.getDoubleAttribute("delayMix",  0.18));
    eqBypassButton.setToggleState    (xml.getIntAttribute("eqBypass",     0)!=0, juce::sendNotification);
    reverbBypassButton.setToggleState(xml.getIntAttribute("reverbBypass", 0)!=0, juce::sendNotification);
    delayBypassButton.setToggleState (xml.getIntAttribute("delayBypass",  0)!=0, juce::sendNotification);
    cabBypassButton.setToggleState   (xml.getIntAttribute("cabBypass",    0)!=0, juce::sendNotification);
    namBypassButton.setToggleState   (xml.getIntAttribute("namBypass",    0)!=0, juce::sendNotification);
}

// =============================================================================
// Audio callbacks
// =============================================================================

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlockExpected;

    toneStateLeft = toneStateRight = 0.0f;
    gateEnvelope  = 0.0f;
    gateGain      = 1.0f;

    updateToneCoefficient();
    updateEqFilters();
    updateReverbParameters();
    updatePreampFilters();
    updatePresenceFilter();
    updateCabFilters();
    reverbProcessor.reset();

    // Delay buffer
    delayBuffer.setSize(2, (int)(sampleRate * 2.0));
    delayBuffer.clear();
    delayWritePosition = 0;

    // Convolution
    dspSpec.sampleRate       = sampleRate;
    dspSpec.maximumBlockSize = (juce::uint32)samplesPerBlockExpected;
    dspSpec.numChannels      = 2;
    convolution.prepare(dspSpec);

    irWorkBuffer.setSize(2, samplesPerBlockExpected);

    // NAM scratch buffers — NAM_SAMPLE is double by default
    namInputBuf .resize((size_t)samplesPerBlockExpected);
    namOutputBuf.resize((size_t)samplesPerBlockExpected);
    namInputPtr  = namInputBuf.data();
    namOutputPtr = namOutputBuf.data();

    // Eğer NAM modeli zaten yüklüyse yeni sample rate ile reset et
    if (namModel != nullptr)
        namModel->ResetAndPrewarm(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* audioBuffer = bufferToFill.buffer;
    if (!audioBuffer) return;

    const int startSample = bufferToFill.startSample;
    const int numSamples  = bufferToFill.numSamples;
    const int numChannels = audioBuffer->getNumChannels();
    if (numChannels <= 0) return;

    auto* leftData  = audioBuffer->getWritePointer(0, startSample);
    auto* rightData = (numChannels > 1) ? audioBuffer->getWritePointer(1, startSample) : nullptr;

    // --- Swap in any pending NAM model (lock-free fast path when no swap needed) ---
    if (namModelPending != nullptr)
    {
        std::unique_ptr<nam::DSP> old;
        {
            std::lock_guard<std::mutex> lock(namSwapMutex);
            if (namModelPending != nullptr)
            {
                old            = std::move(namModel);
                namModel       = std::move(namModelPending);
            }
        }
        // old model destroyed here (outside lock, fine on audio thread)
    }

    // --- Step 1: Gate + input gain (per sample) ---
    for (int i = 0; i < numSamples; ++i)
    {
        float s = processGateSample(leftData[i]);
        s *= inputGain;

        if (namLoaded && !namBypassed && namModel != nullptr)
        {
            // NAM aktifken: drive knob = model input gain (0.5x - 3.0x)
            s *= (0.5f + driveValue * 2.5f);
        }
        else
        {
            // Classic preamp
            s = processPreampSample(s);
        }

        leftData[i] = s;
        if (rightData) rightData[i] = s;
    }

    // --- Step 2: NAM block (ham sinyal üzerinde, EQ/tone öncesi) ---
    if (namLoaded && !namBypassed && namModel != nullptr)
        processNamBlock(leftData, rightData ? rightData : leftData, numSamples);

    // --- Step 3: Post-amp EQ + tone + presence + master (per sample) ---
    float maxLevel = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float s = leftData[i];

        if (!eqBypassed)
        {
            s = bassFilterLeft.processSingleSampleRaw(s);
            s = midFilterLeft.processSingleSampleRaw(s);
            s = trebleFilterLeft.processSingleSampleRaw(s);
        }

        s = processToneSample(s, 0);
        s = presenceFilterLeft.processSingleSampleRaw(s);
        s *= masterValue;

        leftData[i] = s;

        if (rightData)
        {
            if (!eqBypassed)
            {
                bassFilterRight.processSingleSampleRaw(s);
                midFilterRight.processSingleSampleRaw(s);
                trebleFilterRight.processSingleSampleRaw(s);
            }
            processToneSample(s, 1);
            presenceFilterRight.processSingleSampleRaw(s);
            rightData[i] = s;
        }

        const float absSample = s < 0 ? -s : s;
        if (absSample > maxLevel) maxLevel = absSample;
    }

    // Cabinet
    if (!cabBypassed)
        processCabBlock(leftData, rightData ? rightData : leftData, numSamples);

    // Delay
    if (!delayBypassed)
        processDelayBlock(leftData, rightData ? rightData : leftData, numSamples);

    // Reverb
    if (!reverbBypassed)
    {
        if (rightData) reverbProcessor.processStereo(leftData, rightData, numSamples);
        else           reverbProcessor.processMono(leftData, numSamples);
    }

    currentLevel = maxLevel;
}

void MainComponent::releaseResources() {}

// =============================================================================
// Per-sample DSP
// =============================================================================

float MainComponent::processGateSample(float inputSample)
{
    const float rect = inputSample < 0 ? -inputSample : inputSample;
    if (rect > gateEnvelope) gateEnvelope += 0.08f   * (rect - gateEnvelope);
    else                     gateEnvelope += 0.0025f * (rect - gateEnvelope);

    const float envDb       = juce::Decibels::gainToDecibels(juce::jmax(gateEnvelope, 1.0e-6f), -120.0f);
    const float fullCloseDb = gateThresholdDb - 12.0f;
    const float fullOpenDb  = gateThresholdDb + 6.0f;

    float target = 1.0f;
    if (envDb <= fullCloseDb) target = 0.0f;
    else if (envDb < fullOpenDb)
    {
        const float t = (envDb - fullCloseDb) / (fullOpenDb - fullCloseDb);
        target = clamp01(t * t);
    }

    if (target > gateGain) gateGain += 0.06f  * (target - gateGain);
    else                   gateGain += 0.008f * (target - gateGain);

    return inputSample * gateGain;
}

float MainComponent::processPreampSample(float inputSample)
{
    // drive=0'da tamamen temiz sinyal — hiçbir distortion uygulanmaz
    if (driveValue < 0.01f)
        return inputSample;

    // Tight HP sadece drive aktifken — clean tonu bozmaz
    float s = preHpFilterLeft.processSingleSampleRaw(inputSample);

    // Pre-gain: drive arttıkça 6x -> 50x
    const float preGain = 6.0f + driveValue * 44.0f;
    s *= preGain;

    // 4-stage asymmetric clipping
    s = asymClip(s);
    s *= 1.8f + driveValue * 2.0f;
    s = asymClip(s);
    s = std::tanh(s * (1.0f + driveValue * 1.5f));
    s = asymClip(s * 1.2f);

    // Output trim — gain staging kompansasyonu
    const float trimmed = s * (0.18f - driveValue * 0.06f);

    // Drive düşükken orijinal sinyal ile blend et (smooth geçiş)
    const float blend = driveValue * 10.0f; // 0..1 arası (drive 0.1'de tam drive)
    const float blendClamped = blend < 1.0f ? blend : 1.0f;
    return inputSample * (1.0f - blendClamped) + trimmed * blendClamped;
}

float MainComponent::processToneSample(float inputSample, int channel)
{
    float& state = (channel == 0) ? toneStateLeft : toneStateRight;
    state += toneCoefficient * (inputSample - state);
    return state;
}

void MainComponent::processNamBlock(float* leftData, float* rightData, int numSamples)
{
    if (namModel == nullptr) return;

    // Resize scratch buffers if needed (should already be correct from prepareToPlay)
    if ((int)namInputBuf.size() < numSamples)
    {
        namInputBuf.resize((size_t)numSamples);
        namOutputBuf.resize((size_t)numSamples);
        namInputPtr  = namInputBuf.data();
        namOutputPtr = namOutputBuf.data();
    }

    // Copy float input to NAM_SAMPLE (double) buffer
    for (int i = 0; i < numSamples; ++i)
        namInputBuf[(size_t)i] = (double)leftData[i];

    // Process: nam::DSP::process(input**, output**, numFrames)
    namModel->process(&namInputPtr, &namOutputPtr, numSamples);

    // Copy NAM output back and apply to both channels
    for (int i = 0; i < numSamples; ++i)
    {
        const float out = (float)namOutputBuf[(size_t)i];
        leftData[i]  = out;
        if (rightData != leftData)
            rightData[i] = out;
    }
}

void MainComponent::processDelayBlock(float* leftData, float* rightData, int numSamples)
{
    const int bufLen = delayBuffer.getNumSamples();
    if (bufLen <= 1) return;

    const int   delaySamp = juce::jlimit(1, bufLen-1, (int)(currentSampleRate * delayTimeMs / 1000.0f));
    const float feedback  = 0.18f + delayMixValue * 0.45f;
    float* dlL = delayBuffer.getWritePointer(0);
    float* dlR = delayBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        const int readPos = (delayWritePosition + bufLen - delaySamp) % bufLen;
        const float dlyL = dlL[readPos];
        const float dlyR = dlR[readPos];

        const float outL = leftData[i]  * (1.0f - delayMixValue) + dlyL * delayMixValue;
        const float outR = rightData[i] * (1.0f - delayMixValue) + dlyR * delayMixValue;

        dlL[delayWritePosition] = leftData[i]  + dlyL * feedback;
        dlR[delayWritePosition] = rightData[i] + dlyR * feedback;

        leftData[i]  = outL;
        rightData[i] = outR;

        if (++delayWritePosition >= bufLen) delayWritePosition = 0;
    }
}

void MainComponent::processCabBlock(float* leftData, float* rightData, int numSamples)
{
    if (irLoaded)
    {
        irWorkBuffer.setSize(2, numSamples, false, false, true);
        irWorkBuffer.copyFrom(0, 0, leftData,  numSamples);
        irWorkBuffer.copyFrom(1, 0, rightData, numSamples);

        juce::dsp::AudioBlock<float> block(irWorkBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        convolution.process(ctx);

        juce::FloatVectorOperations::copy(leftData,  irWorkBuffer.getReadPointer(0), numSamples);
        juce::FloatVectorOperations::copy(rightData, irWorkBuffer.getReadPointer(1), numSamples);
    }
    else
    {
        // Fallback: Celestion Vintage 30 approximation
        for (int i = 0; i < numSamples; ++i)
        {
            float s = leftData[i];
            s = cabLoFilterLeft.processSingleSampleRaw(s);
            s = cabMidFilterLeft.processSingleSampleRaw(s);
            s = cabHiFilterLeft.processSingleSampleRaw(s);
            leftData[i]  = s;
            rightData[i] = s;
            cabLoFilterRight.processSingleSampleRaw(s);
            cabMidFilterRight.processSingleSampleRaw(s);
            cabHiFilterRight.processSingleSampleRaw(s);
        }
    }
}

// =============================================================================
// DSP parameter updates
// =============================================================================

void MainComponent::updateToneCoefficient()
{
    const float fc = 320.0f + toneValue * (8200.0f - 320.0f);
    const float x  = std::exp(-2.0f * juce::MathConstants<float>::pi * fc / (float)currentSampleRate);
    toneCoefficient = 1.0f - x;
}

void MainComponent::updateEqFilters()
{
    const auto bassC   = juce::IIRCoefficients::makeLowShelf  (currentSampleRate, 180.0,  0.707f, juce::Decibels::decibelsToGain(bassValue));
    const auto midC    = juce::IIRCoefficients::makePeakFilter(currentSampleRate, 900.0,  0.8f,   juce::Decibels::decibelsToGain(midValue));
    const auto trebleC = juce::IIRCoefficients::makeHighShelf (currentSampleRate, 2800.0, 0.707f, juce::Decibels::decibelsToGain(trebleValue));
    bassFilterLeft.setCoefficients(bassC);     bassFilterRight.setCoefficients(bassC);
    midFilterLeft.setCoefficients(midC);       midFilterRight.setCoefficients(midC);
    trebleFilterLeft.setCoefficients(trebleC); trebleFilterRight.setCoefficients(trebleC);
}

void MainComponent::updatePreampFilters()
{
    const auto hpC = juce::IIRCoefficients::makeHighPass(currentSampleRate, 80.0, 0.707);
    preHpFilterLeft.setCoefficients(hpC);
    preHpFilterRight.setCoefficients(hpC);
}

void MainComponent::updatePresenceFilter()
{
    const float presenceDb = presenceValue * 10.0f;
    const auto presC = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 3500.0, 0.707f,
                                                             juce::Decibels::decibelsToGain(presenceDb));
    presenceFilterLeft.setCoefficients(presC);
    presenceFilterRight.setCoefficients(presC);
}

void MainComponent::updateCabFilters()
{
    const auto loC  = juce::IIRCoefficients::makeLowShelf  (currentSampleRate, 100.0,  0.707f, juce::Decibels::decibelsToGain(-6.0f));
    const auto midC = juce::IIRCoefficients::makePeakFilter(currentSampleRate, 1000.0, 1.2f,   juce::Decibels::decibelsToGain(-3.0f));
    const auto hiC  = juce::IIRCoefficients::makeHighShelf (currentSampleRate, 5000.0, 0.707f, juce::Decibels::decibelsToGain(-12.0f));
    cabLoFilterLeft.setCoefficients(loC);   cabLoFilterRight.setCoefficients(loC);
    cabMidFilterLeft.setCoefficients(midC); cabMidFilterRight.setCoefficients(midC);
    cabHiFilterLeft.setCoefficients(hiC);   cabHiFilterRight.setCoefficients(hiC);
}

void MainComponent::updateReverbParameters()
{
    reverbParameters.roomSize   = 0.20f + reverbValue * 0.65f;
    reverbParameters.damping    = 0.30f + reverbValue * 0.35f;
    reverbParameters.wetLevel   = reverbValue * 0.28f;
    reverbParameters.dryLevel   = 1.0f - reverbValue * 0.18f;
    reverbParameters.width      = 1.0f;
    reverbParameters.freezeMode = 0.0f;
    reverbProcessor.setParameters(reverbParameters);
}

// =============================================================================
// Draw helpers
// =============================================================================

void MainComponent::drawAmpBackground(juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::ColourGradient grad(juce::Colour::fromRGB(30,22,14), area.getTopLeft().toFloat(),
                              juce::Colour::fromRGB(10,10,10), area.getBottomRight().toFloat(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(area.toFloat(), 26.0f);
    g.setColour(juce::Colour::fromRGB(86,58,26).withAlpha(0.85f));
    g.drawRoundedRectangle(area.toFloat().reduced(1.5f), 26.0f, 3.0f);
    auto inner = area.reduced(14);
    g.setColour(juce::Colour::fromRGB(24,24,24));
    g.fillRoundedRectangle(inner.toFloat(), 18.0f);
    for (int ly = inner.getY()+8; ly < inner.getBottom(); ly += 12)
    {
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.drawLine((float)(inner.getX()+8),(float)ly,(float)(inner.getRight()-8),(float)ly,1.0f);
    }
}

void MainComponent::drawAmpHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::ColourGradient brass(juce::Colour::fromRGB(205,168,88), area.getTopLeft().toFloat(),
                               juce::Colour::fromRGB(120,88,32),  area.getBottomRight().toFloat(), false);
    g.setGradientFill(brass);
    g.fillRoundedRectangle(area.toFloat(), 18.0f);
    auto hi = area.removeFromTop(16);
    g.setColour(juce::Colour::fromRGB(255,238,185).withAlpha(0.30f));
    g.fillRoundedRectangle(hi.toFloat(), 18.0f);
    g.setColour(juce::Colour::fromRGB(74,48,18));
    g.drawRoundedRectangle(headerBounds.toFloat(), 18.0f, 1.4f);
    g.setColour(juce::Colour::fromRGB(36,24,10));
    g.setFont(juce::Font(juce::FontOptions(30.0f).withStyle("Bold")));
    g.drawText("MySecondAmp", headerBounds.getX()+24, headerBounds.getY()+12,
               headerBounds.getWidth()-220, 34, juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(12.5f)));
    g.drawText("Custom Guitar Amplifier", headerBounds.getX()+28, headerBounds.getBottom()-28,
               220, 18, juce::Justification::centredLeft);
}

void MainComponent::drawAmpFooter(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour::fromRGB(18,18,18));
    g.fillRoundedRectangle(area.toFloat(), 14.0f);
    for (int idx = 0; idx < 6; ++idx)
    {
        const int x = area.getX() + 24 + idx*28;
        g.setColour(juce::Colour::fromRGB(40,40,40));
        g.fillEllipse((float)x,(float)(area.getCentreY()-5),10.0f,10.0f);
        g.setColour(juce::Colour::fromRGB(12,12,12));
        g.drawEllipse((float)x,(float)(area.getCentreY()-5),10.0f,10.0f,1.0f);
    }
    g.setColour(juce::Colour::fromRGB(160,150,120));
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    g.drawText("POWER - STANDBY - ANALOG RESPONSE",
               area.getX()+210, area.getY(), area.getWidth()-220, area.getHeight(),
               juce::Justification::centredLeft);
}

void MainComponent::drawLevelMeter(juce::Graphics& g, juce::Rectangle<int> meterArea) const
{
    g.setColour(juce::Colour::fromRGB(18,18,18));
    g.fillRoundedRectangle(meterArea.toFloat(), 8.0f);
    auto inner = meterArea.reduced(3);
    juce::ColourGradient bg(juce::Colour::fromRGB(20,20,20), inner.getTopLeft().toFloat(),
                            juce::Colour::fromRGB(12,18,12), inner.getBottomLeft().toFloat(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(inner.toFloat(), 6.0f);
    const float boosted = juce::jlimit(0.0f,1.0f, std::pow(currentLevel*2.2f, 0.68f));
    const int   fillW   = (int)(inner.getWidth() * boosted);
    juce::Rectangle<int> filled(inner.getX(), inner.getY(), fillW, inner.getHeight());
    juce::ColourGradient lg(juce::Colour::fromRGB(90,255,120), filled.getTopLeft().toFloat(),
                            juce::Colour::fromRGB(255,210,70), filled.getCentre().toFloat(), false);
    lg.addColour(1.0, juce::Colour::fromRGB(255,70,70));
    g.setGradientFill(lg);
    g.fillRoundedRectangle(filled.toFloat(), 5.0f);
    g.setColour(juce::Colour::fromRGB(95,95,95));
    g.drawRoundedRectangle(meterArea.toFloat(), 8.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    for (int idx = 1; idx < 10; ++idx)
    {
        const float fx = (float)inner.getX() + (float)inner.getWidth()/10.0f*(float)idx;
        g.drawLine(fx,(float)(inner.getY()+2),fx,(float)(inner.getBottom()-2),1.0f);
    }
}

// =============================================================================
// paint
// =============================================================================

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(12,12,12));
    drawAmpBackground(g, ampBodyBounds);
    drawAmpHeader(g, headerBounds);
    drawAmpFooter(g, footerBounds);
    drawLevelMeter(g, levelMeterBounds);

    g.setColour(juce::Colour::fromRGB(50,36,16));
    g.fillRoundedRectangle(juce::Rectangle<float>((float)(ampBodyBounds.getX()+18),
                                                  (float)(ampBodyBounds.getBottom()-22),
                                                  (float)(ampBodyBounds.getWidth()-36),8.0f),4.0f);
    g.setColour(juce::Colour::fromRGB(255,245,215).withAlpha(0.12f));
    g.fillRoundedRectangle(headerBounds.toFloat().translated(0.0f,2.0f).reduced(10.0f,8.0f),12.0f);

    const juce::Colour clipBg = clipping ? juce::Colour::fromRGB(220,50,40) : juce::Colour::fromRGB(35,35,35);
    g.setColour(clipBg);
    g.fillRoundedRectangle(clipLabel.getBounds().toFloat(), 4.0f);
    g.setColour(juce::Colour::fromRGB(80,80,80));
    g.drawRoundedRectangle(clipLabel.getBounds().toFloat(), 4.0f, 1.0f);
}

// =============================================================================
// resized
// =============================================================================

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    ampBodyBounds = bounds;
    auto inner = ampBodyBounds.reduced(20, 18);
    headerBounds = inner.removeFromTop(86);
    footerBounds = juce::Rectangle<int>(ampBodyBounds.getX()+22, ampBodyBounds.getBottom()-56,
                                        ampBodyBounds.getWidth()-44, 32);

    // Header buttons (right to left)
    audioSettingsButton.setBounds(headerBounds.getRight()-170, headerBounds.getY()+12, 150, 32);
    savePresetButton.setBounds   (headerBounds.getRight()-340, headerBounds.getY()+12, 150, 32);
    loadPresetButton.setBounds   (headerBounds.getRight()-510, headerBounds.getY()+12, 150, 32);
    loadIrButton.setBounds       (headerBounds.getRight()-660, headerBounds.getY()+12, 140, 32);
    loadNamButton.setBounds      (headerBounds.getRight()-810, headerBounds.getY()+12, 140, 32);

    // Status labels (bottom-left of header)
    irStatusLabel.setBounds (headerBounds.getX()+24, headerBounds.getBottom()-28, 280, 16);
    namStatusLabel.setBounds(headerBounds.getX()+24, headerBounds.getBottom()-12, 280, 16);

    // Meter zone
    const int meterLabelH  = 18, meterH = 14, meterW = 360;
    const int meterZoneBot = footerBounds.getY() - 16;
    const int meterZoneTop = meterZoneBot - (meterLabelH + 6 + meterH);
    const int meterX       = ampBodyBounds.getCentreX() - meterW/2;

    meterLabel.setBounds(meterX, meterZoneTop, meterW, meterLabelH);
    levelMeterBounds = juce::Rectangle<int>(meterX, meterZoneTop+meterLabelH+6, meterW, meterH);
    clipLabel.setBounds(meterX+meterW+10, levelMeterBounds.getY(), 38, meterH);

    // Controls viewport
    auto controlsArea = juce::Rectangle<int>(
        ampBodyBounds.getX()+26,
        headerBounds.getBottom()+14,
        ampBodyBounds.getWidth()-52,
        meterZoneTop-(headerBounds.getBottom()+14)-8
    );
    controlsViewport.setBounds(controlsArea);
    controlsContent.setBounds(controlsViewport.getLocalBounds());
    layoutAmpControls(controlsViewport.getLocalBounds());
}

// =============================================================================
// layoutAmpControls
// =============================================================================

void MainComponent::layoutAmpControls(juce::Rectangle<int> contentArea)
{
    const int secLabelH=26, knobW=106, knobH=126, vPad=8;
    const int topMargin=16, bottomMargin=12, lrMargin=20, rowGap=18, colGap=18;

    auto work = contentArea.reduced(lrMargin, topMargin);
    work.removeFromBottom(bottomMargin);

    auto firstRow  = work.removeFromTop(170); work.removeFromTop(rowGap);
    auto secondRow = work.removeFromTop(170); work.removeFromTop(rowGap);
    auto bypassRow = work;

    auto inputArea = firstRow.removeFromLeft(firstRow.getWidth()/2);
    auto ampArea   = firstRow;

    secondRow.removeFromLeft(60); secondRow.removeFromRight(60);
    auto eqArea = secondRow.removeFromLeft(secondRow.getWidth()/2);
    auto fxArea = secondRow;

    auto placeGroup = [&](juce::Rectangle<int> groupArea, juce::Label& secLabel,
                          std::initializer_list<std::pair<juce::Label*, juce::Slider*>> controls)
    {
        groupArea.reduce(4, vPad);
        secLabel.setBounds(groupArea.removeFromTop(secLabelH));
        const int count  = (int)controls.size();
        if (count == 0) return;
        const int totalW = count*knobW + juce::jmax(0,count-1)*colGap;
        int cx = groupArea.getX() + juce::jmax(0,(groupArea.getWidth()-totalW)/2);
        const int labelY = groupArea.getY()+2, sldrY = labelY+18;
        for (const auto& [lbl, sld] : controls)
        {
            lbl->setBounds(cx, labelY, knobW, 20);
            sld->setBounds(cx, sldrY,  knobW, knobH);
            cx += knobW + colGap;
        }
    };

    placeGroup(inputArea, inputSectionLabel, { {&gainLabel,&gainSlider}, {&gateLabel,&gateSlider} });
    placeGroup(ampArea,   ampSectionLabel,   { {&driveLabel,&driveSlider}, {&toneLabel,&toneSlider},
                                               {&presenceLabel,&presenceSlider}, {&masterLabel,&masterSlider} });
    placeGroup(eqArea,    eqSectionLabel,    { {&bassLabel,&bassSlider}, {&midLabel,&midSlider}, {&trebleLabel,&trebleSlider} });
    placeGroup(fxArea,    fxSectionLabel,    { {&reverbLabel,&reverbSlider}, {&delayTimeLabel,&delayTimeSlider}, {&delayMixLabel,&delayMixSlider} });

    // Bypass row — 5 buton
    {
        auto ba = bypassRow.reduced(4,4);
        bypassSectionLabel.setBounds(ba.removeFromTop(secLabelH));
        const int btnW=72, btnH=28, gap=10;
        const int totalBW = 5*btnW + 4*gap;
        int bx = ba.getCentreX() - totalBW/2;
        const int by = ba.getY();
        eqBypassButton.setBounds    (bx,              by, btnW, btnH);
        reverbBypassButton.setBounds(bx+  (btnW+gap), by, btnW, btnH);
        delayBypassButton.setBounds (bx+2*(btnW+gap), by, btnW, btnH);
        cabBypassButton.setBounds   (bx+3*(btnW+gap), by, btnW, btnH);
        namBypassButton.setBounds   (bx+4*(btnW+gap), by, btnW, btnH);
    }

    controlsContent.setSize(contentArea.getWidth(), contentArea.getHeight());
}

// =============================================================================
// Callbacks
// =============================================================================

void MainComponent::sliderValueChanged(juce::Slider* s)
{
    if      (s == &gainSlider)      inputGain       = (float)gainSlider.getValue();
    else if (s == &toneSlider)    { toneValue        = (float)toneSlider.getValue();     updateToneCoefficient(); }
    else if (s == &driveSlider)     driveValue      = (float)driveSlider.getValue();
    else if (s == &presenceSlider){ presenceValue    = (float)presenceSlider.getValue(); updatePresenceFilter(); }
    else if (s == &bassSlider)    { bassValue        = (float)bassSlider.getValue();      updateEqFilters(); }
    else if (s == &midSlider)     { midValue         = (float)midSlider.getValue();       updateEqFilters(); }
    else if (s == &trebleSlider)  { trebleValue      = (float)trebleSlider.getValue();    updateEqFilters(); }
    else if (s == &masterSlider)    masterValue     = (float)masterSlider.getValue();
    else if (s == &gateSlider)      gateThresholdDb = (float)gateSlider.getValue();
    else if (s == &reverbSlider)  { reverbValue      = (float)reverbSlider.getValue();    updateReverbParameters(); }
    else if (s == &delayTimeSlider) delayTimeMs     = (float)delayTimeSlider.getValue();
    else if (s == &delayMixSlider)  delayMixValue   = (float)delayMixSlider.getValue();
}

void MainComponent::buttonClicked(juce::Button* b)
{
    if      (b == &eqBypassButton)     eqBypassed     = eqBypassButton.getToggleState();
    else if (b == &reverbBypassButton) reverbBypassed = reverbBypassButton.getToggleState();
    else if (b == &delayBypassButton)  delayBypassed  = delayBypassButton.getToggleState();
    else if (b == &cabBypassButton)    cabBypassed    = cabBypassButton.getToggleState();
    else if (b == &namBypassButton)    namBypassed    = namBypassButton.getToggleState();
}

void MainComponent::timerCallback()
{
    if (currentLevel >= 1.0f)      { clipping = true; clipHoldTimer = 30; }
    else if (clipHoldTimer > 0)    { if (--clipHoldTimer == 0) clipping = false; }
    repaint(levelMeterBounds.expanded(4));
    repaint(clipLabel.getBounds().expanded(2));
}