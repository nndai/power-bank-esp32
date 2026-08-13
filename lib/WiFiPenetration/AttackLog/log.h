#ifndef WIFI_PENETRATION_LOG_H
#define WIFI_PENETRATION_LOG_H

#include <stdint.h>
#include "esp_log.h"


/**
 * @brief Log entry structure
 */
typedef struct {
    uint32_t ts;
    esp_log_level_t level;
    char tag[8];
    char msg[64];
} log_t;

extern QueueHandle_t g_log_queue;
extern bool is_log_enabled;
extern bool is_log_esp_enabled; // true : esp_log, false : queue log

/**
 * @brief Set log enabled state
 * 
 * @param enabled Log enabled state
 */
void set_log_enabled(bool enabled);

/**
 * @brief Initialize logger queue
 */
void log_init();

/**
 * @brief Set log esp enabled state
 * 
 * @param enabled Log esp enabled state
 */
void set_log_queue_enabled(bool enabled);

/**
 * @brief Pushes log entry into log queue with specified level.
 * 
 * @param level Log level
 * @param tag Log tag
 * @param msg Log message
 */
void log_push(esp_log_level_t level, const char* tag, const char* msg, ...);

/**
 * @brief Pushes log entry into log queue with INFO level.
 * 
 * @param tag Log tag
 * @param msg Log message
 */
void log_push(const char* tag, const char* msg, ...);


/**
 * @brief Log macros
 */

#define LOG_NONE     0
#define LOG_ERROR    1
#define LOG_WARN     2
#define LOG_INFO     3
#define LOG_DEBUG    4
#define LOG_VERBOSE  5

#ifdef CORE_DEBUG_LEVEL
#define ATTACK_LOG CORE_DEBUG_LEVEL
#else
#define ATTACK_LOG LOG_INFO
#endif

#if ATTACK_LOG >= LOG_INFO
#define ATTACK_LOG_I(tag, format, ...) log_push(ESP_LOG_INFO, tag, format, ##__VA_ARGS__)
#else
#define ATTACK_LOG_I(tag, format, ...)
#endif

#if ATTACK_LOG >= LOG_DEBUG
#define ATTACK_LOG_D(tag, format, ...) log_push(ESP_LOG_DEBUG, tag, format, ##__VA_ARGS__)
#else
#define ATTACK_LOG_D(tag, format, ...)
#endif

#if ATTACK_LOG >= LOG_WARN
#define ATTACK_LOG_W(tag, format, ...) log_push(ESP_LOG_WARN, tag, format, ##__VA_ARGS__)
#else
#define ATTACK_LOG_W(tag, format, ...)
#endif

#if ATTACK_LOG >= LOG_ERROR
#define ATTACK_LOG_E(tag, format, ...) log_push(ESP_LOG_ERROR, tag, format, ##__VA_ARGS__)
#else
#define ATTACK_LOG_E(tag, format, ...)
#endif

#if ATTACK_LOG >= LOG_VERBOSE
#define ATTACK_LOG_V(tag, format, ...) log_push(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
#else
#define ATTACK_LOG_V(tag, format, ...)
#endif

#endif