#include <algorithm>
#include <memory>
#include <chrono>
#include <iomanip>

#include <Logger.h>

// The return string is move constructed
std::string log_time()
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
std::string log_classname(const char * prettyfunc)
{
    std::string prettyStr(prettyfunc);

    size_t begin = prettyStr.find(" ");

    if (begin == prettyStr.npos)
    {
        begin = 0;
    }

    size_t end = prettyStr.find("::", begin);

    if (begin != 0)
    {
        begin ++; // begins at the char after the space
    }

    if (end == prettyStr.npos)
    {
        end = begin;
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