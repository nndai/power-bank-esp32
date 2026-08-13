#include "WifiController/sniffer.h"

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "Config/config.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "sniffer"; 

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

/**
 * @brief Callback for promiscuous reciever. 
 * 
 * It forwards captured frames into event pool and sorts them based on their type
 */
static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ATTACK_LOG_V(TAG, "Captured frame %d.", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;

    int32_t event_id;
    switch (type) {
        case WIFI_PKT_DATA:
            event_id = SNIFFER_EVENT_CAPTURED_DATA;
            break;
        case WIFI_PKT_MGMT:
            event_id = SNIFFER_EVENT_CAPTURED_MGMT;
            break;
        case WIFI_PKT_CTRL:
            event_id = SNIFFER_EVENT_CAPTURED_CTRL;
            break;
        default:
            return;
    }
    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id, frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), portMAX_DELAY));
}

void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    if(data) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    }
    
    if(mgmt) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    }
    
    if(ctrl) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    }
    esp_wifi_set_promiscuous_filter(&filter);
}

void wifictl_sniffer_start(uint8_t channel) {
    ATTACK_LOG_I(TAG, "Start promiscuous mode");
    // ESP32 cannot switch port, if there is some STA connected to AP
    ATTACK_LOG_D(TAG, "Kick STAs from AP");
    // esp_wifi_deauth_sta(0) might not be available in all SDK versions or takes an argument.
    // In SDK 4.x it takes a pointer or 0? 
    // Checking docs: esp_err_t esp_wifi_deauth_sta(uint16_t aid)
    // aid=0 is all.
    ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
}

void wifictl_sniffer_stop() {
    ATTACK_LOG_I(TAG, "Stop promiscuous mode");
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(CONFIG_MGMT_AP_CHANNEL, WIFI_SECOND_CHAN_NONE);
}
