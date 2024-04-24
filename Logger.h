#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <esp_log.h>
#include <string>
#include <atomic>

class Logger;

// It's multithreaded, and lockless (the buffer is swapped when reading/writing)
class Logger
{
    public:
        static void initLogs();
        static Logger* getLogger() { return logger; }

        static int toSerial(const char * fmt, ...);
        static int logPrintf(const char *fmt, ...) {
            va_list a;
            va_start(a, fmt);
            int count = logVaList(fmt, a);
            va_end(a);
            return count;
        }

        // Caller acquires ownership of returned string, has to delete() it
        std::string * getLog();
        static int logVaList(const char*, va_list);

    private:
        Logger();
        static Logger *logger;
#if CHICKENWORLD_LOG_TO_SERIAL
        static vprintf_like_t serial_log;
#endif //CHICKENWORLD_LOG_TO_SERIAL
        std::atomic<std::string *> buffer;

        int _log(const char *fmt, va_list a);
};

#endif //__LOGGER_H__