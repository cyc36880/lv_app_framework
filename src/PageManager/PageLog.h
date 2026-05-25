/**
 * @file PageLog.h
 * @brief PageManager logging utilities
 */
#ifndef FRAMEWORK_PAGE_LOG_H
#define FRAMEWORK_PAGE_LOG_H

#define PAGE_LOG_LEVEL_INFO  0
#define PAGE_LOG_LEVEL_WARN 1
#define PAGE_LOG_LEVEL_ERROR 2
#define PAGE_LOG_LEVEL_OFF   3

#ifndef PAGE_LOG_LEVEL
#define PAGE_LOG_LEVEL PAGE_LOG_LEVEL_OFF
#endif

#if (PAGE_LOG_LEVEL < PAGE_LOG_LEVEL_OFF)

void PageLog(int level, const char* func, const char* format, ...);

#define PAGE_LOG_INFO(...)  PageLog(PAGE_LOG_LEVEL_INFO, __func__, __VA_ARGS__)
#define PAGE_LOG_WARN(...)  PageLog(PAGE_LOG_LEVEL_WARN, __func__, __VA_ARGS__)
#define PAGE_LOG_ERROR(...) PageLog(PAGE_LOG_LEVEL_ERROR, __func__, __VA_ARGS__)

#else
#define PAGE_LOG_INFO(...)
#define PAGE_LOG_WARN(...)
#define PAGE_LOG_ERROR(...)
#endif

#endif  // FRAMEWORK_PAGE_LOG_H