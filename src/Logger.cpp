#ifdef NATIVE_PLATFORM
#include <execinfo.h>
#endif // NATIVE_PLATFORM

#include <algorithm>
#include <memory>
#include <chrono>
#include <iomanip>

#include <Logger.h>

#define BACKTRACE_DEPTH (10)

void logBacktrace()
{
#ifdef CHICKEN_LOG_BACKTRACES
#ifdef NATIVE_PLATFORM
    void * backtraceEntries[BACKTRACE_DEPTH];
    char ** backtraceLines;

    int depth = backtrace(backtraceEntries, BACKTRACE_DEPTH);
    backtraceLines = backtrace_symbols(backtraceEntries, depth);

    if (backtraceLines == NULL)
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        Logger::getLogger()->log("%s\n", backtraceLines[i]);
    }

    free (backtraceLines);
#elif defined (ESP_PLATFORM)
    esp_backtrace_print(BACKTRACE_DEPTH);
#endif // ESP_PLATFORM
#endif // CHICKEN_LOG_BACKTRACES
}

// The return string is move constructed
std::string logTime()
{
    auto timePointNow = std::chrono::system_clock::now();
    auto timeTNow = std::chrono::system_clock::to_time_t(timePointNow);

    // For some undoubtedly wise reasons the geniuses that defin the standard decided that second accuracy is good
    // for everyone.
    auto milliSeconds = duration_cast<std::chrono::milliseconds>(timePointNow.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeTNow), "%X") << '.' << milliSeconds.count();
    return ss.str();
}

// TODO: this is still work in progress
std::string logClassname(const char * prettyfunc)
{
    std::string prettyStr(prettyfunc);

    // Check if it's a template first
    size_t templateMark = prettyStr.find("<");

    // Remove everything up to "Chicken::" if present
    size_t begin = prettyStr.find("Chicken::");

    if (begin == prettyStr.npos || begin > templateMark)
    {
        // A whitespace should be present at the end of the return type, except for structors
        begin = prettyStr.find(" ") + 1;
    }
    else
    {
        begin += 9;
    }

    // Remove everything after the begin fo the argument list
    size_t end = templateMark == prettyStr.npos ? prettyStr.find("::", begin) : templateMark;
    
    if (end == prettyStr.npos)
    {
        end = prettyStr.find("(");
    }

    // the " <" is to avoid a false detection of a space at the end of a template
    if (begin == prettyStr.npos || begin > end)
    {
        begin = 0;
    }

    if (end == prettyStr.npos)
    {
        // This should probably never happen
        end = prettyStr.length();
    }

    size_t whitespacePos = prettyStr.find(" ", begin);

    // Weed out possible additional whitespaces
    while (whitespacePos != prettyStr.npos && whitespacePos < end)
    {
        begin = whitespacePos + 1;
        whitespacePos = prettyStr.find(" ", begin);
    }

    return prettyStr.substr(begin, end - begin);
}

Logger *Logger::_logger;

Logger::Logger()
{
#ifdef ESP_PLATFORM
#if CHICKEN_LOG_TO_SYSTEM
    _systemLogger = 
#endif //CHICKEN_LOG_TO_SYSTEM
    // Capture system logs
    esp_log_set_vprintf(staticVariadicLog);
#else // Not embedded platform
    _systemLogger = vprintf;
#endif // ESP_PLATFORM
}

int Logger::variadicLog(const char* fmt, va_list args)
{
    int systemRet = 0;

#if CHICKEN_LOG_TO_SYSTEM
    // Log to system if necessary
    va_list copyArgs;
    va_copy(copyArgs, args);
    systemRet = _systemLogger(fmt, copyArgs);
    va_end(copyArgs);
#endif //CHICKEN_LOG_TO_SYSTEM

    // Notify listeners of the new log
    int listenersRet = logToListeners(fmt, args);

    if (systemRet < 0 || listenersRet < 0)
    {
        return -1;
    }

    if (listenersRet > 0)
    {
        return listenersRet;
    }

    return systemRet;
}

void Logger::addListener(SLoggerListener listener)
{
    std::unique_lock<std::mutex> lock(_mutex);

    _listeners.push_back(listener);
}

void Logger::removeListener(SLoggerListener listener)
{
    std::unique_lock<std::mutex> lock(_mutex);

    _listeners.erase(std::find(_listeners.begin(), _listeners.end(), listener));
}

int Logger::logToListeners(const char * fmt, va_list args)
{
    if (_listeners.size() == 0)
    {
        return 0;
    }

    auto entry = MakeLoggerEntry(fmt, args);

    std::unique_lock<std::mutex> lock(_mutex);
    for (auto listener: _listeners)
    {
        listener->newLogEntry(entry);
    }

    return entry->size();
}

void Logger::initLogs()
{
    _logger = new Logger();
}

char * LoggerEntry::formattedString(const char * format, va_list args)
{
    char * outputString = NULL;
    int stringSize = vasprintf(&outputString, format, args);

    if (stringSize > 0)
    {
        return outputString;
    }

    return NULL;
}

LoggerEntry::~LoggerEntry()
{
    if (data())
    {
        free((void *) data());
    }
}