/**
 * @file DataBrokerLog.h
 * @brief DataBroker logging utilities
 */
#ifndef FRAMEWORK_DATA_BROKER_LOG_H
#define FRAMEWORK_DATA_BROKER_LOG_H

#define DATA_BROKER_LOG_LEVEL_INFO  0
#define DATA_BROKER_LOG_LEVEL_WARN  1
#define DATA_BROKER_LOG_LEVEL_ERROR  2
#define DATA_BROKER_LOG_LEVEL_OFF    3

#ifndef DATA_BROKER_LOG_LEVEL
#define DATA_BROKER_LOG_LEVEL DATA_BROKER_LOG_LEVEL_OFF
#endif

#if (DATA_BROKER_LOG_LEVEL < DATA_BROKER_LOG_LEVEL_OFF)
#include <stdio.h>
#define _DN_LOG(format, ...) printf("[DN] " format "\r\n", ##__VA_ARGS__)
#endif

#if (DATA_BROKER_LOG_LEVEL <= DATA_BROKER_LOG_LEVEL_INFO)
#define DN_LOG_INFO(format, ...)  _DN_LOG("[Info] " format, ##__VA_ARGS__)
#define DN_LOG_WARN(format, ...) _DN_LOG("[Warn] " format, ##__VA_ARGS__)
#define DN_LOG_ERROR(format, ...) _DN_LOG("[Error] " format, ##__VA_ARGS__)
#elif (DATA_BROKER_LOG_LEVEL <= DATA_BROKER_LOG_LEVEL_WARN)
#define DN_LOG_INFO(format, ...)
#define DN_LOG_WARN(format, ...) _DN_LOG("[Warn] " format, ##__VA_ARGS__)
#define DN_LOG_ERROR(format, ...) _DN_LOG("[Error] " format, ##__VA_ARGS__)
#elif (DATA_BROKER_LOG_LEVEL <= DATA_BROKER_LOG_LEVEL_ERROR)
#define DN_LOG_INFO(format, ...)
#define DN_LOG_WARN(format, ...)
#define DN_LOG_ERROR(format, ...) _DN_LOG("[Error] " format, ##__VA_ARGS__)
#else
#define DN_LOG_INFO(...)
#define DN_LOG_WARN(...)
#define DN_LOG_ERROR(...)
#endif

#endif  // FRAMEWORK_DATA_BROKER_LOG_H