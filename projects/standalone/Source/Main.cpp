/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"


REGISTER_STATIC_DSP_LIBRARIES()
{
	REGISTER_STATIC_DSP_FACTORY(HiseCoreDspFactory);
}

AudioProcessor* StandaloneProcessor::createProcessor()
{
	return new BackendProcessor(deviceManager, callback);
}


//==============================================================================
class HISEStandaloneApplication  : public JUCEApplication
{
public:
    //==============================================================================
    HISEStandaloneApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        if (commandLine.startsWith("export"))
		{
			CompileExporter::ErrorCodes result = CompileExporter::compileFromCommandLine(commandLine);

			if (result != CompileExporter::OK)
			{
				std::cout << std::endl << "==============================================================================" << std::endl;
				std::cout << "EXPORT ERROR: " << CompileExporter::getCompileResult(result) << std::endl;
				std::cout << "==============================================================================" << std::endl << std::endl;

				exit((int)result);
			}
			
			quit();
			return;
		}
		else if (commandLine.startsWith("--help"))
		{
			std::cout << std::endl;
			std::cout << "HISE Command Line Tool" << std::endl;
			std::cout << "----------------------" << std::endl << std::endl;
			std::cout << "Usage: " << std::endl << std::endl;
			std::cout << "HISE export \"File.hip\" -t:TYPE [-p:PLUGIN_TYPE] [-a:ARCHITECTURE]" << std::endl << std::endl;
			std::cout << "Options: " << std::endl << std::endl;
			std::cout << "-h:{TEXT} sets the HISE path. Use this if you don't have compiler settings set." << std::endl;
			std::cout << "-t:{TEXT} sets the project type (standalone | instrument | effect)" << std::endl;
			std::cout << "-p:{TEXT} sets the plugin type (VST | AU | VST_AU | AAX)" << std::endl;
			std::cout << "          (Leave empty for standalone export)" << std::endl;
			std::cout << "-a:{TEXT} sets the architecture (x86, x64, x86x64)." << std::endl;
			std::cout << "          (Leave empty on OSX for Universal binary.)" << std::endl << std::endl;

			quit();
			return;
		}
		else
		{
			mainWindow = new MainWindow(commandLine);
			mainWindow->setUsingNativeTitleBar(true);
			mainWindow->toFront(true);
		}
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const String& ) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainContentComponent class.
    */
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow(const String &commandLine)  : DocumentWindow ("HISE Backend Standalone",
                                        Colours::lightgrey,
										DocumentWindow::TitleBarButtons::closeButton | DocumentWindow::maximiseButton | DocumentWindow::TitleBarButtons::minimiseButton)
        {
            setContentOwned (new MainContentComponent(commandLine), true);

#if JUCE_IOS
            
            Rectangle<int> area = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
            
            setSize(area.getWidth(), area.getHeight());
            
#else
            
			setResizable(true, true);

			setUsingNativeTitleBar(true);


            
            centreWithSize (getWidth(), getHeight() - 28);
            
#endif
            
            setVisible (true);
			
        }

        void closeButtonPressed()
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    ScopedPointer<MainWindow> mainWindow;
};



//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (HISEStandaloneApplication)
