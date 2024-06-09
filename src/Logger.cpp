#ifdef ESP_PLATFORM
#include "esp_debug_helpers.h"
#include "esp_log.h"
#elif defined(NATIVE_PLATFORM)
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
const char * logTime()
{
    static char timeBuffer[15];

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm currentLocalTime = *localtime(&tv.tv_sec);

    strftime(timeBuffer, 15, "%H:%M:%S.", &currentLocalTime);
    snprintf(timeBuffer + 9, 4, "%03lu", tv.tv_usec / 1000);
    return timeBuffer;
}

// TODO: this is still work in progress
std::string logClassnameOrFunction(const char * prettyfunc)
{
    std::string prettyStr(prettyfunc);

    // Remove everything up to "Chicken::" if present
    size_t namespaceBegin = prettyStr.find("Chicken::");
    size_t retBegin = 0;

    if (namespaceBegin == prettyStr.npos)
    {
        // A whitespace should be present at the end of the return type, except for structors
        retBegin = prettyStr.find(" ") + 1;

        if (retBegin == prettyStr.npos)
        {
            retBegin = 0;
        }
    }
    else
    {
        retBegin = namespaceBegin + 9;
    }

    // If this is a class method, the name of the class will end at the next "::"
    size_t retEnd = prettyStr.find("::", retBegin);

    // Check if it's a template, remove the template arguments
    size_t templateArgsBegin = prettyStr.find("<", retBegin);
    if (templateArgsBegin < retEnd)
    {
        retEnd = templateArgsBegin;
    }

    // This should handle simple functions (no class name) and excise the arguments list from them
    size_t argumentsStart = prettyStr.find("(", retBegin);
    
    if (argumentsStart < retEnd)
    {
        // If no end was found, cut the string to where the arguments begin
        retEnd = argumentsStart;
    }

    // Begin at beginning of pretty function if no begin was found
    if (retBegin == prettyStr.npos || retBegin > retEnd)
    {
        retBegin = 0;
    }

    if (retEnd == prettyStr.npos)
    {
        // This should probably never happen
        retEnd = prettyStr.length();
    }

    // Cut away stray tokens from the beginning
    size_t whitespacePos = prettyStr.find(" ", retBegin);

    // Weed out possible additional whitespaces
    while (whitespacePos != prettyStr.npos && whitespacePos < retEnd)
    {
        retBegin = whitespacePos + 1;
        whitespacePos = prettyStr.find(" ", retBegin);
    }

    return prettyStr.substr(retBegin, retEnd - retBegin);// + "[" + prettyStr + "]";
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