#include "hccapx_serializer.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"
#include "arpa/inet.h"

#include "FrameAnalyzer/frame_analyzer.h"
#include "FrameAnalyzer/frame_analyzer_types.h"
#include "FrameAnalyzer/frame_analyzer_parser.h"

// Constants
#define HCCAPX_SIGNATURE 0x58504348
#define HCCAPX_VERSION 4
#define HCCAPX_KEYVER_WPA 1
#define HCCAPX_KEYVER_WPA2 2
#define HCCAPX_MAX_EAPOL_SIZE 256

static const char *TAG = "hccapx_serializer";

static hccapx_t hccapx = { 
    .signature = HCCAPX_SIGNATURE, 
    .version = 4, 
    .message_pair = 255,
    .essid_len = 0,
    .essid = {0},
    .keyver = HCCAPX_KEYVER_WPA2,
    .keymic = {0},
    .mac_ap = {0},
    .nonce_ap = {0},
    .mac_sta = {0},
    .nonce_sta = {0},
    .eapol_len = 0,
    .eapol = {0}
};

static unsigned message_ap = 0;
static unsigned message_sta = 0;
static unsigned eapol_source = 0;

static bool is_array_zero(uint8_t *array, unsigned size){
    for(unsigned i = 0; i < size; i++){
        if(array[i] != 0){
            return false;
        }
    }
    return true;
}

void hccapx_serializer_init(const uint8_t *ssid, unsigned size){
    hccapx.essid_len = size;
    memcpy(hccapx.essid, ssid, size);
    hccapx.message_pair = 255;
    
    // Reset state variables
    message_ap = 0;
    message_sta = 0;
    eapol_source = 0;
    memset(hccapx.mac_ap, 0, 6);
    memset(hccapx.mac_sta, 0, 6);
    memset(hccapx.nonce_ap, 0, 32);
    memset(hccapx.nonce_sta, 0, 32);
    memset(hccapx.keymic, 0, 16);
    memset(hccapx.eapol, 0, 256);
}

hccapx_t *hccapx_serializer_get(){
    if(hccapx.message_pair == 255){
        return NULL;
    }
    
    return &hccapx;
}

bool hccapx_serializer_is_valid(){
    return hccapx.message_pair != 255;
}

static unsigned save_eapol(eapol_packet_t *eapol_packet, eapol_key_packet_t *eapol_key_packet){
    unsigned eapol_len = 0;
    // ntohs is available in arpa/inet.h or lwip
    eapol_len = sizeof(eapol_packet_header_t) + ntohs(eapol_packet->header.packet_body_length);
    if(eapol_len > HCCAPX_MAX_EAPOL_SIZE){
        ATTACK_LOG_W(TAG, "EAPoL is too long (%u/%u)", eapol_len, HCCAPX_MAX_EAPOL_SIZE);
        return 1;
    }
    hccapx.eapol_len = eapol_len;
    memcpy(hccapx.eapol, eapol_packet, hccapx.eapol_len);
    memcpy(hccapx.keymic, eapol_key_packet->key_mic, 16);
    
    // Clear key MIC from EAPoL packet
    memset(&hccapx.eapol[81], 0x0, 16);
    return 0;
}

static void ap_message_m1(eapol_key_packet_t *eapol_key_packet){
    ATTACK_LOG_D(TAG, "From AP M1");
    message_ap = 1;
    memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
}

static void ap_message_m3(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ATTACK_LOG_D(TAG, "From AP M3");
    message_ap = 3;
    if(message_ap == 0){
        memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
    }
    if(eapol_source == 2){
        hccapx.message_pair = 2;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 3;
    if(message_sta == 2){
        hccapx.message_pair = 3;
    }
}

static void ap_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if((!is_array_zero(hccapx.mac_sta, 6)) && (memcmp(frame->mac_header.addr1, hccapx.mac_sta, 6) != 0)){
        ATTACK_LOG_E(TAG, "Different STA");
        return;
    }
    if(message_ap == 0){
        memcpy(hccapx.mac_ap, frame->mac_header.addr2, 6);
    }
    if(is_array_zero(eapol_key_packet->key_mic, 16)){
        ap_message_m1(eapol_key_packet);
    } 
    else {
        ap_message_m3(eapol_packet, eapol_key_packet);
    }
}

static void sta_message_m2(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ATTACK_LOG_D(TAG, "From STA M2");
    message_sta = 2;
    memcpy(hccapx.nonce_sta, eapol_key_packet->key_nonce, 32);
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 2;
    if(message_ap == 1){
        hccapx.message_pair = 0;
        return;
    }
}

static void sta_message_m4(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    ATTACK_LOG_D(TAG, "From STA M4");
    if((message_sta == 2) && (eapol_source != 0)){
        ATTACK_LOG_D(TAG, "Already have M2, not worth");
        return;
    }
    if(message_ap == 0){
        ATTACK_LOG_E(TAG, "Not enough M1/M2/M3/M4");
        return;
    }
    if(eapol_source == 3){
        hccapx.message_pair = 4;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 4;
    if(message_ap == 1){
        hccapx.message_pair = 1;
    }
    if(message_ap == 3){
        hccapx.message_pair = 5;
    }
}

static void sta_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if(is_array_zero(hccapx.mac_sta, 6)){
        memcpy(hccapx.mac_sta, frame->mac_header.addr2, 6);
    }
    else if(memcmp(frame->mac_header.addr2, hccapx.mac_sta, 6) != 0){
        ATTACK_LOG_E(TAG, "Different STA");
        return;
    }
    if(!is_array_zero(eapol_key_packet->key_nonce, 16)){
        sta_message_m2(eapol_packet, eapol_key_packet);
    } 
    else {
        sta_message_m4(eapol_packet, eapol_key_packet);
    }
}

void hccapx_serializer_add_frame(data_frame_t *frame){
    eapol_packet_t *eapol_packet = parse_eapol_packet(frame);
    // Safety check just in case
    if (eapol_packet == NULL) return;
    
    eapol_key_packet_t *eapol_key_packet = parse_eapol_key_packet(eapol_packet);
    if (eapol_key_packet == NULL) return;

    if(memcmp(frame->mac_header.addr2, frame->mac_header.addr3, 6) == 0){
        ap_message(frame, eapol_packet, eapol_key_packet);
    } 
    else if(memcmp(frame->mac_header.addr1, frame->mac_header.addr3, 6) == 0){
        sta_message(frame, eapol_packet, eapol_key_packet);
    } 
    else {
        ATTACK_LOG_E(TAG, "Bad frame format");
    }
}

static void byte_array_to_hex(const uint8_t *in, size_t len, char *out) {
    for (size_t i = 0; i < len; ++i) {
        sprintf(out + (i * 2), "%02x", in[i]);
    }
    out[len * 2] = '\0';
}

int hccapx_serializer_get_22000_string(char *buffer, size_t max_len) {
    if (hccapx.message_pair == 255) {
        return -1;
    }

    /*
     * PMKID: WPA*01*PMKID*MAC_AP*MAC_STA*ESSID
     * EAPOL: WPA*02*MIC*MAC_AP*MAC_STA*ESSID*NONCE_AP*EAPOL*MESSAGE_PAIR
     */

    char mic_hex[33];
    char mac_ap_hex[13];
    char mac_sta_hex[13];
    char essid_hex[65]; // 32 * 2 + 1
    char nonce_ap_hex[65];
    char eapol_hex[513]; // 256 * 2 + 1

    byte_array_to_hex(hccapx.keymic, 16, mic_hex);
    byte_array_to_hex(hccapx.mac_ap, 6, mac_ap_hex);
    byte_array_to_hex(hccapx.mac_sta, 6, mac_sta_hex);
    byte_array_to_hex(hccapx.essid, hccapx.essid_len, essid_hex); // Use hccapx.essid_len
    byte_array_to_hex(hccapx.nonce_ap, 32, nonce_ap_hex);
    byte_array_to_hex(hccapx.eapol, hccapx.eapol_len, eapol_hex); // Use hccapx.eapol_len

    int written = snprintf(buffer, max_len, "WPA*02*%s*%s*%s*%s*%s*%s*%02x",
        mic_hex,
        mac_ap_hex,
        mac_sta_hex,
        essid_hex,
        nonce_ap_hex,
        eapol_hex,
        hccapx.message_pair
    );
    
    if (written < 0 || (size_t)written >= max_len) {
         return -1;
    }

    return written;
}
