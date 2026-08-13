#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "AttackLog/log.h"

QueueHandle_t g_log_queue;
bool is_log_enabled = false;
bool is_log_queue_enabled = true;

/**
 * @brief Initialize logger queue
 */
void log_init() {
    if (!g_log_queue) {
        g_log_queue = xQueueCreate(50, sizeof(log_t));
    }
}

void set_log_enabled(bool enabled)
{
    is_log_enabled = enabled;
}

void set_log_queue_enabled(bool enabled)
{
    is_log_queue_enabled = enabled;
}

void log_push(const char* tag, const char* format, ...)
{
    if (!is_log_enabled) return;

    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_INFO, tag, format, args);
    va_end(args);
    return;

    if (!is_log_queue_enabled) {
        return;
    }

    if (!g_log_queue) return;

    log_t log = {
        .ts = xTaskGetTickCount(),
        .level = ESP_LOG_INFO
    };

    strncpy(log.tag, tag, sizeof(log.tag) - 1);
    log.tag[sizeof(log.tag) - 1] = '\0';


    va_start(args, format);
    vsnprintf(log.msg, sizeof(log.msg), format, args);
    va_end(args);

    xQueueSend(g_log_queue, &log, 0);
}

void log_push(esp_log_level_t level, const char* tag, const char* format, ...)
{
    if (!is_log_enabled) return;

    va_list args;
    va_start(args, format);
    esp_log_writev(level, tag, format, args);
    va_end(args);

    if (!is_log_queue_enabled) {
        return;
    }

    if (!g_log_queue) return;

    log_t log = {
        .ts = xTaskGetTickCount(),
        .level = level
    };

    strncpy(log.tag, tag, sizeof(log.tag) - 1);
    log.tag[sizeof(log.tag) - 1] = '\0';


    va_start(args, format);
    vsnprintf(log.msg, sizeof(log.msg), format, args);
    va_end(args);

    xQueueSend(g_log_queue, &log, 0);
}
