#ifndef REPEATER_NVS_H
#define REPEATER_NVS_H

#include <Arduino.h>

/**
 * @brief Runtime config struct for repeater settings
 */
typedef struct {
    char sta_ssid[33];      // Upstream WiFi SSID
    char sta_pass[65];      // Upstream WiFi password
    char ap_ssid[33];       // Repeater AP SSID
    char ap_pass[65];       // Repeater AP password
} repeater_config_t;

/**
 * @brief Load all repeater settings from NVS
 * @param config Pointer to config struct to fill
 */
void rpt_nvs_load(repeater_config_t* config);

/**
 * @brief Save STA (upstream) credentials to NVS
 */
void rpt_nvs_save_sta(const char* ssid, const char* pass);

/**
 * @brief Save AP (repeater) settings to NVS
 */
void rpt_nvs_save_ap(const char* ssid, const char* pass);

/**
 * @brief Erase all repeater config from NVS
 */
void rpt_nvs_erase_all();

#endif // REPEATER_NVS_H
