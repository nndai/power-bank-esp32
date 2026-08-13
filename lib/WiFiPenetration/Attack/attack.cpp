#include "Attack/attack.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "AttackLog/log.h"

#include "Attack/attack_pmkid.h"
#include "Attack/attack_handshake.h"
#include "Attack/attack_dos.h"
#include "WifiController/wifi_controller.h" 
#include "WebServer/webserver.h" 

static const char* TAG = "attack";
static attack_status_t attack_status = { .state = READY, .type = (uint8_t)-1, .content_size = 0, .content = NULL };
static esp_timer_handle_t attack_timeout_handle;

const attack_status_t *attack_get_status() {
    return &attack_status;
}

void attack_update_status(attack_state_t state) {
    attack_status.state = state;
    if(state == FINISHED) {
        ATTACK_LOG_D(TAG, "Stop attack timer");
        esp_err_t err = esp_timer_stop(attack_timeout_handle);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ATTACK_LOG_E(TAG, "Stop timer fail: %s", esp_err_to_name(err));
        }
    } 
}

void attack_append_status_content(uint8_t *buffer, unsigned size){
    if(size == 0){
        ATTACK_LOG_E(TAG, "Size 0 invalid for realloc");
        return;
    }

    if (esp_get_free_heap_size() < 50 * 1024) {
        ATTACK_LOG_E(TAG, "OOM for status content!");
        return;
    }
    
    // temporarily save new location in case of realloc failure to preserve current content
    char *reallocated_content = (char*)realloc(attack_status.content, attack_status.content_size + size);
    if(reallocated_content == NULL){
        ATTACK_LOG_E(TAG, "Realloc fail! Content bad");
        return;
    }
    // copy new data after current content
    memcpy(&reallocated_content[attack_status.content_size], buffer, size);
    attack_status.content = reallocated_content;
    attack_status.content_size += size;
}

char *attack_alloc_result_content(unsigned size) {
    attack_status.content_size = size;
    attack_status.content = (char *) malloc(size);
    return attack_status.content;
}

/**
 * @brief Callback function for attack timeout timer.
 */
static void attack_timeout(void* arg){
    ATTACK_LOG_D(TAG, "Attack timed out");
    
    attack_update_status(TIMEOUT);

    switch(attack_status.type) {
        case ATTACK_TYPE_PMKID:
            ATTACK_LOG_I(TAG, "Stop PMKID attack...");
            attack_pmkid_stop();
            break;
        case ATTACK_TYPE_HANDSHAKE:
            ATTACK_LOG_I(TAG, "Stop HANDSHAKE attack...");
            attack_handshake_stop();
            break;
        case ATTACK_TYPE_PASSIVE:
            ATTACK_LOG_I(TAG, "Stop PASSIVE attack...");
            break;
        case ATTACK_TYPE_DOS:
            ATTACK_LOG_I(TAG, "Stop DOS attack...");
            attack_dos_stop();
            break;
        default:
            ATTACK_LOG_E(TAG, "Unknown attack. No abort");
    }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_REQUEST event.
 */
static void attack_request_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ATTACK_LOG_I(TAG, "Starting attack...");
    attack_request_t *attack_request = (attack_request_t *) event_data;
    attack_config_t attack_config = { .type = attack_request->type, .method = attack_request->method, .timeout = attack_request->timeout };
    attack_config.ap_record = wifictl_get_ap_record(attack_request->ap_record_id);
    
    attack_status.state = RUNNING;
    attack_status.type = attack_config.type;

    if(attack_config.ap_record == NULL){
        ATTACK_LOG_E(TAG, "No AP record!");
        return;
    }
    // set timeout
    ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, (uint64_t)attack_config.timeout * 1000000ULL));
    // start attack based on it's type
    switch(attack_config.type) {
        case ATTACK_TYPE_PMKID:
            attack_pmkid_start(&attack_config);
            break;
        case ATTACK_TYPE_HANDSHAKE:
            attack_handshake_start(&attack_config);
            break;
        case ATTACK_TYPE_PASSIVE:
            ATTACK_LOG_W(TAG, "Passive type not impl.");
            break;
        case ATTACK_TYPE_DOS:
            attack_dos_start(&attack_config);
            break;
        default:
            ATTACK_LOG_E(TAG, "Unknown attack type!");
    }
}

/**
 * @brief Callback for WEBSERVER_EVENT_ATTACK_RESET event.
 */
static void attack_reset_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ATTACK_LOG_D(TAG, "Resetting attack status...");
    if(attack_status.content){
        free(attack_status.content);
        attack_status.content = NULL;
    }
    attack_status.content_size = 0;
    attack_status.type = -1;
    attack_status.state = READY;
}

void attack_init(){
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "attack_timeout"
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}
