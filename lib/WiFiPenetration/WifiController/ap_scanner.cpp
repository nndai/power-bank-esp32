#include "WifiController/ap_scanner.h"


#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "esp_wifi.h"

static const char* TAG = "wifi_controller/ap_scanner";
/**
 * @brief Stores last scanned AP records into linked list.
 * 
 */
static wifictl_ap_records_t ap_records;

void wifictl_scan_nearby_aps(){
    ATTACK_LOG_D(TAG, "Scanning nearby APs...");

    ap_records.count = CONFIG_SCAN_MAX_AP;

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
    };
    
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    
    // Fix: count is uint16_t, esp_wifi_scan_get_ap_records expects uint16_t* which matches
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_records.count, ap_records.records));
    ATTACK_LOG_I(TAG, "Found %u APs.", ap_records.count);
    ATTACK_LOG_D(TAG, "Scan done.");
}

const wifictl_ap_records_t *wifictl_get_ap_records() {
    return &ap_records;
}

const wifi_ap_record_t *wifictl_get_ap_record(unsigned index) {
    if(index > ap_records.count){
        ATTACK_LOG_E(TAG, "Idx %u > max %u", ap_records.count, index);
        return NULL;
    }
    return &ap_records.records[index];
}
