#pragma once

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <locale>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>

namespace WingsOfSteel
{

class ILogTarget;

//////////////////////////////////////////////////////////////////////////
// Log
// Contains any number of ILogTargets, which are responsible for actually
// logging the message in various ways.
// This class is thread safe.
//////////////////////////////////////////////////////////////////////////

using LogTargetSharedPtr = std::shared_ptr<ILogTarget>;

class Log
{
public:
    enum class Level
    {
        Info,
        Warning,
        Error,

        Count
    };

    class Stream
    {
    public:
        Stream(Level level);
        ~Stream();

        template <typename T>
        Stream& operator<<(T const& value)
        {
            m_Collector << value;
            return *this;
        }

#if defined(TARGET_PLATFORM_WINDOWS)
        Stream& operator<<(const std::filesystem::path& value)
        {
            // Cleanup all the slashes and display them in Windows' standard format ('\').
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            m_Collector << std::regex_replace(converter.to_bytes(value), std::regex("(\\\\|/)"), "\\");
            return *this;
        }

        Stream& operator<<(const std::wstring& value)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            m_Collector << converter.to_bytes(value);
            return *this;
        }
#endif

    private:
        Level m_Level;
        std::ostringstream m_Collector;
    };

    static Stream Info();
    static Stream Warning();
    static Stream Error();

    static void AddLogTarget(LogTargetSharedPtr pLogTarget);
    static void RemoveLogTarget(LogTargetSharedPtr pLogTarget);

private:
    using LogTargetList = std::list<LogTargetSharedPtr>;

    static void LogInternal(const std::string& text, Log::Level level);
    static void AbortOnError(Log::Level level);

    static std::mutex m_Mutex;
    static LogTargetList m_Targets;
};

//////////////////////////////////////////////////////////////////////////
// ILogTarget
// Any ILogTarget must implement Log().
//////////////////////////////////////////////////////////////////////////

class ILogTarget
{
public:
    virtual ~ILogTarget() {}
    virtual void Log(const std::string& text, Log::Level level) = 0;

protected:
    static const std::string& GetPrefix(Log::Level level);
};

//////////////////////////////////////////////////////////////////////////
// StdOutLogger
// Prints the message to std::out
//////////////////////////////////////////////////////////////////////////

class StdOutLogger : public ILogTarget
{
public:
    virtual void Log(const std::string& text, Log::Level type) override;
};

//////////////////////////////////////////////////////////////////////////
// FileLogger - Native only
// Dumps the logging into file given in "filename". It is flushed
// after every entry.
//////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_NATIVE)
class FileLogger : public ILogTarget
{
public:
    FileLogger(const std::filesystem::path& filePath);
    virtual ~FileLogger() override;
    virtual void Log(const std::string& text, Log::Level type) override;

private:
    std::ofstream m_File;
};
#endif

//////////////////////////////////////////////////////////////////////////
// MessageBoxLogger - Native only
// Creates a message box whenever the log message is above LogLevel::Info.
//////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_NATIVE)
class MessageBoxLogger : public ILogTarget
{
public:
    virtual void Log(const std::string& text, Log::Level type) override;
};
#endif

} // namespace WingsOfSteel