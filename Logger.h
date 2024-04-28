#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <esp_log.h>
#include <string>
#include <atomic>
#include <memory>

#define CHICKEN_LOG_LEVEL_DEBUG   (4)
#define CHICKEN_LOG_LEVEL_INFO    (3)
#define CHICKEN_LOG_LEVEL_WARNING (2)
#define CHICKEN_LOG_LEVEL_ERROR   (1)

#ifndef CHICKEN_LOG_LEVEL
#define CHICKEN_LOG_LEVEL 1
#endif

// Macro that can be used for debug-only code other than logs
#define CHICKEN_DO_DEBUG

// TODO: redefine LOG_COLOR_X macros

// Log format
#define CHICKEN_BASIC_LOG_FORMAT(letter, format) LOG_COLOR_##letter #letter " (%s) %s: " format LOG_RESET_COLOR

// Extracts the class name from a string generated with __PRETTY_FUNCTION__
std::shared_ptr<char> __classname(const char * prettyfunc);

// Logs without adding a newline, useful to begin a line with complex logging
#define _log(type, format, ...) Logger::logPrintf(CHICKEN_BASIC_LOG_FORMAT(type, format), esp_log_system_timestamp(), __classname(__PRETTY_FUNCTION__), ##__VA_ARGS__)

// Appends to the current line. Does not append a '\n' automatically
#define _logappend(format, ...) Logger::logPrintf(format, ##__VAR_ARGS__)

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

class Logger;

// @brief Basic logging class with capabilities to output to different destinations
// @details once initialized, the logger captures all logs from the system, and redirects them
// to a specific output (for example, serial or a web page). It's multithreaded, and lockless (the
// log buffer is swapped when reading/writing)
class Logger
{
    public:
        // @brief capture system logs to intrnal buffering
        static void initLogs();
        static Logger* getLogger() { return logger; }

        // @brief Log to serial
        // @details if CHICKENWORLD_LOG_TO_SERIAL is defined, the input arguments are converted printf-like
        // and dumped to serial
        static int toSerial(const char * fmt, ...);

        // @brief printf-like logging function
        static int logPrintf(const char *fmt, ...) {
            va_list a;
            va_start(a, fmt);
            int count = logVaList(fmt, a);
            va_end(a);
            return count;
        }

        // @brief Returns the logging buffer and transfers its ownership
        // @details Caller acquires ownership of returned string, has to delete() it
        std::string * getLog();

        // @brief log to Logger va_list style
        // @details Logs to serial if CHICKENWORLD_LOG_TO_SERIAL is specified, then also stores
        // data into the internal logging buffer
        static int logVaList(const char*, va_list);

    private:
        Logger();

        // Singleton
        static Logger *logger;
#if CHICKENWORLD_LOG_TO_SERIAL
        static vprintf_like_t serial_log;
#endif //CHICKENWORLD_LOG_TO_SERIAL

        // Buffer containing the current log data
        std::atomic<std::string *> buffer;

        // @brief Log data to internal buffer
        int logToInternalBuffer(const char *fmt, va_list a);
};

#endif //__LOGGER_H__