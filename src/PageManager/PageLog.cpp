/**
 * @file PageLog.cpp
 * @brief PageManager logging implementation
 */
#include "PageLog.h"
#include <stdio.h>
#include <stdarg.h>

void PageLog(int level, const char* func, const char* format, ...)
{
    const char* prefix = "INFO";
    if (level == PAGE_LOG_LEVEL_WARN) prefix = "WARN";
    if (level == PAGE_LOG_LEVEL_ERROR) prefix = "ERROR";

    printf("[Page][%s][%s] ", prefix, func);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\r\n");
}