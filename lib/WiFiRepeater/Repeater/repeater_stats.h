#ifndef REPEATER_STATS_H
#define REPEATER_STATS_H

#include <Arduino.h>
#include <esp_wifi.h>
#include "esp_netif.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "dhcpserver/dhcpserver.h"

typedef struct {
    uint8_t mac[6];
    char mac_str[18];
    char ip_str[16];
    int8_t rssi;
    bool has_ip;
} rpt_client_t;

typedef struct {
    uint64_t rx_bytes_total;
    uint64_t tx_bytes_total;
    uint32_t rx_speed_bps; // bytes per second
    uint32_t tx_speed_bps; // bytes per second
} rpt_traffic_stats_t;

/**
 * @brief Initialize netif hooks for byte counting
 */
void rpt_stats_init();

/**
 * @brief Periodic tick to calculate current speed (call every tick/loop)
 */
void rpt_stats_tick();

/**
 * @brief Get current traffic statistics (total bytes and live speeds)
 */
void rpt_stats_get_traffic(rpt_traffic_stats_t* stats);

/**
 * @brief Get list of currently connected AP clients
 * @param clients Array to store client info
 * @param max_clients Capacity of array
 * @return Number of connected clients
 */
int rpt_stats_get_clients(rpt_client_t* clients, int max_clients);

/**
 * @brief Format byte count as human-readable string (e.g. "1.2 MB")
 */
void rpt_format_bytes(uint64_t bytes, char* buf, size_t buf_size);

/**
 * @brief Format speed as human-readable string (e.g. "250 KB/s")
 */
void rpt_format_speed(uint32_t bytes_per_sec, char* buf, size_t buf_size);

#endif // REPEATER_STATS_H
