#include "FrameAnalyzer/frame_analyzer.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#include "WifiController/sniffer.h" 
#include "FrameAnalyzer/frame_analyzer_parser.h"

static const char *TAG = "frame_analyzer";
static uint8_t target_bssid[6];
static search_type_t search_type = (search_type_t)-1;


/**
 * @brief Analyzes data frames from sniffer.
 *  
 * @param args 
 * @param event_base 
 * @param event_id 
 * @param event_data 
 */
static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ATTACK_LOG_V(TAG, "Handling DATA frame");
    //ESP_LOGI(TAG, "Handling DATA frame, core: %s", xPortGetCoreID() == 0 ? "0" : "1");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if(!is_frame_bssid_matching(frame, target_bssid)){
        ATTACK_LOG_V(TAG, "Not matching BSSIDs.");
        return;
    }

    eapol_packet_t *eapol_packet = parse_eapol_packet((data_frame_t *) frame->payload);
    if(eapol_packet == NULL){
        ATTACK_LOG_V(TAG, "Not an EAPOL packet.");
        return;
    }

    eapol_key_packet_t *eapol_key_packet = parse_eapol_key_packet(eapol_packet);
    if(eapol_key_packet == NULL){
        ATTACK_LOG_V(TAG, "Not an EAPOL-Key packet");
        return;
    }

    if(search_type == SEARCH_HANDSHAKE){
        // TODO handle timeouts properly by e.g. for cycle
        // ESP_ERROR_CHECK_WITHOUT_ABORT is not strictly available in all Arduino SDKs, but often mapped.
        // Using basic check.
        esp_err_t err = esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, frame, sizeof(wifi_promiscuous_pkt_t) + frame->rx_ctrl.sig_len, portMAX_DELAY);
        if (err != ESP_OK) {
             ATTACK_LOG_E(TAG, "Failed to post event: %s", esp_err_to_name(err));
        }
        return;
    }

    if(search_type == SEARCH_PMKID){
        pmkid_item_t *pmkid_items;
        if((pmkid_items = parse_pmkid(eapol_key_packet)) == NULL){
            return;
        }
        ESP_ERROR_CHECK(esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_items, sizeof(pmkid_item_t *), portMAX_DELAY));
        return;
    }
}

void frame_analyzer_capture_start(search_type_t search_type_arg, const uint8_t *bssid){
    ATTACK_LOG_I(TAG, "Frame analysis started...");
    search_type = search_type_arg;
    memcpy(&target_bssid, bssid, 6);
    // Depends on sniffer events
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, NULL));
}

void frame_analyzer_capture_stop(){
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &data_frame_handler));
}
