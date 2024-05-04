#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef ESP_PLATFORM
#include <esp_log.h>
#endif // ESP_PLATFORM

#include <string>
#include <memory>
#include <mutex>
#include <vector>

#ifndef CHICKEN_LOG_TO_SYSTEM
// Enable logging to system device (printf for native, serial for embedded)
#define CHICKEN_LOG_TO_SYSTEM 1
#endif // CHICKEN_LOG_TO_SYSTEM

#define CHICKEN_LOG_LEVEL_DEBUG   (4)
#define CHICKEN_LOG_LEVEL_INFO    (3)
#define CHICKEN_LOG_LEVEL_WARNING (2)
#define CHICKEN_LOG_LEVEL_ERROR   (1)

#define CHICKEN_LOG_COLOR_RED    "\033[1;31m"
#define CHICKEN_LOG_COLOR_GREEN  "\033[1;32m"
#define CHICKEN_LOG_COLOR_YELLOW "\033[1;33m"
#define CHICKEN_LOG_COLOR_CYAN   "\033[1;36m"
#define CHICKEN_LOG_COLOR_WHITE  "\033[1;37m"
#define CHICKEN_LOG_COLOR_RESET  "\033[1;0m"

#define CHICKEN_LOG_COLOR_E CHICKEN_LOG_COLOR_RED
#define CHICKEN_LOG_COLOR_W CHICKEN_LOG_COLOR_YELLOW
#define CHICKEN_LOG_COLOR_I CHICKEN_LOG_COLOR_CYAN
#define CHICKEN_LOG_COLOR_D CHICKEN_LOG_COLOR_GREEN

#ifndef CHICKEN_LOG_LEVEL
#define CHICKEN_LOG_LEVEL 1
#endif

// Macro that can be used for debug-only code other than logs
#define CHICKEN_DO_DEBUG

std::string log_time();
// Extracts the class name from a string generated with __PRETTY_FUNCTION__
std::string log_classname(const char * prettyfunc);

// Log format
#define CHICKEN_BASIC_LOG_FORMAT(letter, format) CHICKEN_LOG_COLOR_##letter #letter " [%s] %s: " format CHICKEN_LOG_COLOR_RESET

// Logs without adding a newline, useful to begin a line with complex logging
#define _log(type, format, ...) { \
    auto __timeString = log_time(); \
    auto __className = log_classname(__PRETTY_FUNCTION__); \
    Logger::getLogger()->log(CHICKEN_BASIC_LOG_FORMAT(type, format), __timeString.c_str(), __className.c_str(), ##__VA_ARGS__); \
}

// Appends to the current line. Does not append a '\n' automatically
#define _logappend(format, ...) Logger::getLogger()->log(format, ##__VA_ARGS__)

// Logs with automatic newline
#if CHICKEN_LOG_LEVEL >= CHICKEN_LOG_LEVEL_DEBUG
#define _logd(format, ...) _log(D, format "\n", ##__VA_ARGS__)
#else
#define _logd(format, ...)
#endif

#if CHICKEN_LOG_LEVEL >= CHICKEN_LOG_LEVEL_INFO
#define _logi(format, ...) _log(I, format "\n", ##__VA_ARGS__)
#else
#define _logi(format, ...)
#endif

#if CHICKEN_LOG_LEVEL >= CHICKEN_LOG_LEVEL_WARNING
#define _logw(format, ...) _log(W, format "\n", ##__VA_ARGS__)
#else
#define _logw(format, ...)
#endif

#if CHICKEN_LOG_LEVEL >= CHICKEN_LOG_LEVEL_ERROR
#define _loge(format, ...) _log(E, format "\n", ##__VA_ARGS__)
#else
#define _loge(format, ...)
#endif

// @brief std::string_view subclass with data ownership
// @details Automatically deallocates the referenced data on destruction
class LoggerEntry: public std::string_view
{
    public:
    // @brief allocates the memory necessary for the formatted string and formats it
    LoggerEntry(const char* format, va_list args): std::string_view(formattedString(format, args)) {};
    ~LoggerEntry();

    private:
    char * formattedString(const char * format, va_list args);
};

typedef std::shared_ptr<LoggerEntry> SLoggerEntry;
#define MakeLoggerEntry(...) std::make_shared<LoggerEntry>(__VA_ARGS__);

class LoggerListener; // forward declaration
typedef std::shared_ptr<LoggerListener> SLoggerListener;

// @brief Basic logging class with capabilities to output to different destinations
// @details once initialized, the logger captures all logs from the system, and redirects them
// to a specific output (for example, serial or a web page). It's multithreaded, and lockless (the
// log buffer is swapped when reading/writing)
class Logger
{
    public:
        typedef int (*SystemLogger)(const char*, va_list);

        // @brief prepare system resources to handle logging
        static void initLogs();

        // @return logger singleton
        static Logger* getLogger() { return _logger; }

        // @brief printf-like logging function
        int log(const char *fmt, ...)
        {
            va_list a;
            va_start(a, fmt);
            int count = variadicLog(fmt, a);
            va_end(a);
            return count;
        }

        // @brief log to Logger va_list style
        // @details Logs to serial if CHICKEN_LOG_TO_SYSTEM is specified, then also passes the
        // formatted data to all registered listeners
        int variadicLog(const char* fmt, va_list args);

        void addListener(SLoggerListener listener);
        void removeListener(SLoggerListener listener);

        // @brief formats the output string and notifies all the listeners
        // @return the number of chars in the formatted string
        int logToListeners(const char * fmt, va_list args);

    private:
        Logger();

        // Singleton
        static Logger *_logger;

        // TODO: could probably do without mutex, as long as all listeners are added
        // during the initialization phase
        std::mutex _mutex;
        std::vector<SLoggerListener> _listeners;

#if CHICKEN_LOG_TO_SYSTEM
        // Handler to capture all logs
        SystemLogger _systemLogger;
#endif //CHICKEN_LOG_TO_SYSTEM

        static int staticVariadicLog(const char * fmt, va_list args) { return getLogger()->variadicLog(fmt, args); }
};

// @brief Listener to logging events
// @details Can be used to route system logs to different destinations, like f.e. telnet or web pages
class LoggerListener
{
    public:

    // @brief notification that a new log entry is available
    // @warning do not call logging macros/functions (f.e. _logi) from within this method
    virtual void newLogEntry(SLoggerEntry entry) = 0;
};

#endif //__LOGGER_H__