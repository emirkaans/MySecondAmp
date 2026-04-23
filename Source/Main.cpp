#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class Application : public juce::JUCEApplication
{
public:
    Application() = default;

    const juce::String getApplicationName() override
    {
        return "MyApp";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& windowName)
            : juce::DocumentWindow(
                windowName,
                juce::Desktop::getInstance()
                    .getDefaultLookAndFeel()
                    .findColour(juce::ResizableWindow::backgroundColourId),
                juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);

            setResizeLimits(720, 540, 1920, 1280);

            auto* mainComponent = new MainComponent();
            setContentOwned(mainComponent, true);

            centreWithSize(1180, 760);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(Application)