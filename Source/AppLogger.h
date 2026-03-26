#pragma once

#include <JuceHeader.h>

namespace AppLogging
{
    // AppLogger is not realtime-safe: it uses heap allocations, string formatting,
    // locking, and file I/O. Do not call from processBlock() or other audio-thread DSP code.
    enum class LogLevel
    {
        Debug = 0,
        Info,
        Warning,
        Error,
        Critical
    };

    class AppLogger
    {
    public:
        static void initialize();
        static void shutdown();

        static void setMinimumLevel(LogLevel level);
        static LogLevel getMinimumLevel();

        static void log(LogLevel level,
                        const juce::String& component,
                        const juce::String& message,
                        const juce::String& context = {});

        static void debug(const juce::String& component,
                          const juce::String& message,
                          const juce::String& context = {});

        static void info(const juce::String& component,
                         const juce::String& message,
                         const juce::String& context = {});

        static void warning(const juce::String& component,
                            const juce::String& message,
                            const juce::String& context = {});

        static void error(const juce::String& component,
                          const juce::String& message,
                          const juce::String& context = {});

        static void critical(const juce::String& component,
                             const juce::String& message,
                             const juce::String& context = {});

        static juce::String makeErrorId();

    private:
        static juce::String formatLogLine(LogLevel level,
                                         const juce::String& component,
                                         const juce::String& message,
                                         const juce::String& context);

        static LogLevel parseLogLevel(const juce::String& text);
        static juce::String formatLogLevel(LogLevel level);

        static juce::File getConfigFile();
        static LogLevel loadLogLevelFromConfig();

        static std::unique_ptr<juce::FileLogger> fileLogger;
        static LogLevel minimumLevel;
        static bool initialized;
        static juce::CriticalSection writeLock;
    };
}
