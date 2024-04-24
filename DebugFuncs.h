#ifndef __LOGFUNCS_H__
#define __LOGFUNCS_H__

// TAG must be defined before importing the header for this to work
#ifdef TAG
// Use the basic esp logging functions, but abstract them away for compatibility with other platforms
#include <esp_log.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

// Macro that can be used for debug-only code other than logs
#define DO_DEBUG

// Log format
#define BASIC_LOG_FORMAT(letter, format) LOG_COLOR_##letter #letter " (%s) %s: " format LOG_RESET_COLOR

// Logs without adding a newline, useful to begin a line with complex logging
#define logme(format, ...) Logger::logPrintf(BASIC_LOG_FORMAT(E, format), esp_log_system_timestamp(), TAG, ##__VA_ARGS__)

// Appends to the current line. Does not append a '\n' automatically
#define logappend(format, ...) Logger::logPrintf(format, ##__VAR_ARGS__)
#else // TAG
#define logme(...)
#define logappend(...)
#endif //TAG

// Verbose log functions
#if (LOG_LEVEL > 1)
#define vlogln(format, ...) logln(format, ##__VA_ARGS__)
#define vlog(format, ...) log(format, ##__VA_ARGS__)
#define vlogappend(format, ...) logappend(format, ##__VA_ARGS__)
#else
#define vlogln(...)
#define vlog(...)
#define vlogappend(...)
#endif

// Logs with automatic newline
#define logln(format, ...) logme(format "\n", ##__VA_ARGS__)

// defines the out label
#define _getout out:

// goto out after printing a message and setting the return value
#define bailout(retval, format, ...) {logln(format, ##__VA_ARGS__); err = retval; goto out;}

// goto out after printing a message if err != ESP_OK. The return value must be set before calling this
#define checkout(format, ...) {if (err != ESP_OK){logln(format " (%s)", ##__VA_ARGS__, esp_err_to_name(err)); goto out;}}

// bails out if the test condition is true
#define testout(test, retval, format, ...) {if (test)bailout(retval, format, ##__VA_ARGS__)}

#include <Logger.h>

#endif // __LOGFUNCS_H__