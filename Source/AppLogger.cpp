#include "AppLogger.h"

namespace AppLogging
{
    // AppLogger is not realtime-safe. Must not be called from audio-thread DSP paths.
    // It uses locking, string formatting, and file logging.
    juce::CriticalSection AppLogger::writeLock;
    std::unique_ptr<juce::FileLogger> AppLogger::fileLogger;
    LogLevel AppLogger::minimumLevel = LogLevel::Info;
    bool AppLogger::initialized = false;

    void AppLogger::initialize()
    {
        if (initialized)
            return;

        const auto logLevel = loadLogLevelFromConfig();
        minimumLevel = logLevel;

        const auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                     .getChildFile("ResonanceEQ");
        appDataDir.createDirectory();

        const auto logsDir = appDataDir.getChildFile("Logs");
        logsDir.createDirectory();

        fileLogger = std::unique_ptr<juce::FileLogger>(juce::FileLogger::createDefaultAppLogger("ResonanceEQ", logsDir.getFullPathName(), "ResonanceEQ log", 10));

        if (fileLogger)
            fileLogger->logMessage("[AppLogger] log initialized at level " + formatLogLevel(minimumLevel));

        juce::Logger::writeToLog("[AppLogger] initialized with min level " + formatLogLevel(minimumLevel));

        initialized = true;
    }

    void AppLogger::shutdown()
    {
        if (!initialized)
            return;

        if (fileLogger)
            fileLogger->logMessage("[AppLogger] shutdown");

        juce::Logger::writeToLog("[AppLogger] shutdown");

        fileLogger.reset();
        initialized = false;
    }

    void AppLogger::setMinimumLevel(LogLevel level)
    {
        minimumLevel = level;
        info("AppLogger", "Minimum log level set to " + formatLogLevel(level));
    }

    LogLevel AppLogger::getMinimumLevel() { return minimumLevel; }

    void AppLogger::log(LogLevel level,
                        const juce::String& component,
                        const juce::String& message,
                        const juce::String& context)
    {
        if (!initialized)
            return;

        if (level < minimumLevel)
            return;

        const auto line = formatLogLine(level, component, message, context);

        const juce::ScopedLock lock(writeLock);

        // Non-realtime logging only. Must never be called from processBlock() or other realtime DSP paths.
        juce::Logger::writeToLog(line);

        if (fileLogger)
            fileLogger->logMessage(line);
    }

    void AppLogger::debug(const juce::String& component,
                          const juce::String& message,
                          const juce::String& context)
    {
        log(LogLevel::Debug, component, message, context);
    }

    void AppLogger::info(const juce::String& component,
                         const juce::String& message,
                         const juce::String& context)
    {
        log(LogLevel::Info, component, message, context);
    }

    void AppLogger::warning(const juce::String& component,
                            const juce::String& message,
                            const juce::String& context)
    {
        log(LogLevel::Warning, component, message, context);
    }

    void AppLogger::error(const juce::String& component,
                          const juce::String& message,
                          const juce::String& context)
    {
        log(LogLevel::Error, component, message, context);
    }

    void AppLogger::critical(const juce::String& component,
                             const juce::String& message,
                             const juce::String& context)
    {
        log(LogLevel::Critical, component, message, context);
    }

    juce::String AppLogger::makeErrorId()
    {
        return juce::Uuid().toString();
    }

    juce::String AppLogger::formatLogLine(LogLevel level,
                                          const juce::String& component,
                                          const juce::String& message,
                                          const juce::String& context)
    {
        const auto ts = juce::Time::getCurrentTime().toString(true, true);

        juce::String line = "[" + ts + "] [" + formatLogLevel(level) + "] [" + component + "] " + message;
        if (context.isNotEmpty())
            line += " (" + context + ")";

        return line;
    }

    LogLevel AppLogger::parseLogLevel(const juce::String& text)
    {
        const auto t = text.trim().toUpperCase();

        if (t == "DEBUG")
            return LogLevel::Debug;
        if (t == "INFO")
            return LogLevel::Info;
        if (t == "WARNING" || t == "WARN")
            return LogLevel::Warning;
        if (t == "ERROR")
            return LogLevel::Error;
        if (t == "CRITICAL")
            return LogLevel::Critical;

        return LogLevel::Info;
    }

    juce::String AppLogger::formatLogLevel(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Debug:
                return "DEBUG";
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warning:
                return "WARNING";
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Critical:
                return "CRITICAL";
            default:
                return "INFO";
        }
    }

    juce::File AppLogger::getConfigFile()
    {
        const auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                     .getChildFile("ResonanceEQ");
        appDataDir.createDirectory();

        return appDataDir.getChildFile("loglevel.txt");
    }

    LogLevel AppLogger::loadLogLevelFromConfig()
    {
        const auto f = getConfigFile();

        if (!f.existsAsFile())
        {
            // Non-fatal: use default level
            return LogLevel::Info;
        }

        const auto text = f.loadFileAsString().trim();
        if (text.isEmpty())
            return LogLevel::Info;

        return parseLogLevel(text);
    }
}
