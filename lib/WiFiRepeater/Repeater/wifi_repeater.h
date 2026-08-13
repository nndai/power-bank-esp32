#ifndef WIFI_REPEATER_H
#define WIFI_REPEATER_H

#include <Arduino.h>



/**
 * @brief Initialize the WiFi Repeater.
 * 
 * Loads config from NVS, starts WiFi in STA+AP mode,
 * enables NAPT forwarding, and starts the web server.
 * 
 * This is the single entry point — call this from setup()
 * when the repeater mode is active.
 */
void repeater_init();

/**
 * @brief Process background tasks for repeater (Captive Portal DNS requests)
 */
void repeater_tick();

/**
 * @brief Check if the repeater's STA is connected to upstream WiFi
 */
bool repeater_is_sta_connected();

/**
 * @brief Get number of clients connected to the repeater's AP
 */
uint16_t repeater_get_client_count();

/**
 * @brief Get the STA IP address (upstream network)
 * @return IP as uint32_t (network byte order), 0 if not connected
 */
uint32_t repeater_get_sta_ip();

/**
 * @brief Get the AP IP address
 * @return IP as uint32_t (network byte order)
 */
uint32_t repeater_get_ap_ip();

/**
 * @brief Get uptime in seconds since repeater_init()
 */
uint32_t repeater_get_uptime_seconds();

/**
 * @brief Stop the repeater (disconnect STA, stop AP, stop web server)
 */
void repeater_stop();

#endif // WIFI_REPEATER_H
