#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <memory>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <esp_log.h>
#include <Logger.h>

// Drop oldest logs once buffer reaches this size
#define LOGGER_MAX_BUFFER_SIZE (512)

// TODO: replace  with std::lock
class SpinLock
{
    portMUX_TYPE spinLock;
    public:
        SpinLock() { spinLock = portMUX_INITIALIZER_UNLOCKED; }
        void lock(){ taskENTER_CRITICAL(&spinLock); }
        void unlock(){ taskEXIT_CRITICAL(&spinLock); }
};

Logger *Logger::logger;
#if CHICKENWORLD_LOG_TO_SERIAL
vprintf_like_t Logger::serial_log;
#endif //CHICKENWORLD_LOG_TO_SERIAL

int Logger::logVaList(const char* fmt, va_list origArgs)
{
#if CHICKENWORLD_LOG_TO_SERIAL
    va_list copyArgs;
    va_copy(copyArgs, origArgs);
    serial_log(fmt, copyArgs);
    va_end(copyArgs);
#endif //CHICKENWORLD_LOG_TO_SERIAL
    return logger->logToInternalBuffer(fmt, origArgs);
}

void Logger::initLogs()
{
    logger = new Logger();
#if CHICKENWORLD_LOG_TO_SERIAL
    serial_log =
#endif //CHICKENWORLD_LOG_TO_SERIAL
    esp_log_set_vprintf(logVaList);
}

int Logger::toSerial(const char * fmt, ...)
{
#if CHICKENWORLD_LOG_TO_SERIAL
    va_list a;
    va_start(a, fmt);
    int count = serial_log(fmt, a);
    va_end(a);
    return count;
#else
    return 0;
#endif
}

Logger::Logger()
{
    buffer = NULL;
}

std::string * Logger::getLog()
{
    return buffer.exchange(NULL);
}

// TODO: use Chicken::Str in place of std::string
int Logger::logToInternalBuffer(const char *fmt, va_list args)
{
    char * tmpString = NULL;

    int writtenSize = vasprintf(&tmpString, fmt, args);

    if (writtenSize <= 0) {
        return writtenSize;
    }

    std::string * targetString = buffer.exchange(NULL);

    if (targetString == NULL) {
        targetString = new std::string();
    }

    targetString->append(tmpString);
    free(tmpString);

    if (targetString->size() > LOGGER_MAX_BUFFER_SIZE) {
        targetString->erase(0, targetString->size() - LOGGER_MAX_BUFFER_SIZE);
        targetString->replace(0, 3, "[.]");
    }

    buffer.exchange(targetString);

    return writtenSize;
}

// TODO: this is still work in progress
std::shared_ptr<char> __classname(const char * prettyfunc)
{
    char * begin = strstr(prettyfunc, " "); // this to remove possible return values
    char * end = strstr(prettyfunc, "::"); // remove the function prototype to reduce clutter

    if (begin == NULL || begin >= end)
    {
        begin = (char *) prettyfunc;
    }
    else
    {
        begin ++;
    }

    std::shared_ptr<char> className = nullptr;

    if (end == NULL)
    {
        end = begin;
    }

    className = std::shared_ptr<char>(new char[end - begin + 1]);
    memcpy(className.get(), begin, end - begin);
    className.get()[end - begin] = '\0';

    return className;
}