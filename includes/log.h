#ifndef LOG_H
#define LOG_H
#include "helper.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#define RESET "\033[0m"
#define BOLDRED "\033[1m\033[31m"    /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"  /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m" /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"   /* Bold Blue */

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
} Log_Level;

#ifdef LOG
#define log_info(...)                          \
    do                                         \
    {                                          \
        log_with_level(LOG_INFO, __VA_ARGS__); \
    } while (0)

#define log_error(...)                          \
    do                                          \
    {                                           \
        log_with_level(LOG_ERROR, __VA_ARGS__); \
    } while (0)

#else
#define log_info(...)
#define log_error(...)
#endif

#endif

#ifdef LOG_IMPLEMENTATION
static void log_with_level(Log_Level level, const char *fmt, ...)
{
    switch (level)
    {
    case LOG_INFO:
        fprintf(stderr, BOLDBLUE "[INFO] " RESET);
        break;
    case LOG_WARN:
        fprintf(stderr, BOLDYELLOW "[WARNING] " RESET);
        break;
    case LOG_ERROR:
        fprintf(stderr, BOLDRED "[ERROR] " RESET);
        break;
    default:
        NOTREACHABLE;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
#endif