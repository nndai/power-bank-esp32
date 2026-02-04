/**
 * @file hccapx_serializer.h
 * @brief Provides interface to generate HCCAPX formatted binary from raw frame bytes 
 */
#ifndef HCCAPX_SERIALIZER_H
#define HCCAPX_SERIALIZER_H

#include <stdint.h>
#include <Arduino.h>
#include "FrameAnalyzer/frame_analyzer_types.h"

/**
 * @brief HCCAPX structure according to reference
 * 
 * @see Ref: https://hashcat.net/wiki/doku.php?id=hccapx
 */
typedef struct __attribute__((__packed__)){
    uint32_t signature;
    uint32_t version;
    uint8_t message_pair;
    uint8_t essid_len;
    uint8_t essid[32];
    uint8_t keyver;
    uint8_t keymic[16];
    uint8_t mac_ap[6];
    uint8_t nonce_ap[32];
    uint8_t mac_sta[6];
    uint8_t nonce_sta[32];
    uint16_t eapol_len;
    uint8_t eapol[256];
} hccapx_t;

/**
 * @brief Creates new HCCAPX buffer for given SSID.
 * 
 * This will clear any previous HCCAPX. If you want to save it, first call hccapx_serializer_get() and copy buffer somewhere else.
 * @param ssid SSID of AP from which the handshake frames will be comming.
 * @param size length of SSID string (including \0)
 */
void hccapx_serializer_init(const uint8_t *ssid, unsigned size);

/**
 * @brief Returns pointer to buffer with HCCAPX formatted binary data 
 * 
 * @return hccapx_t* 
 */
hccapx_t *hccapx_serializer_get();

/**
 * @brief Checks if current HCCAPX contains valid handshake (message pair != 255)
 * 
 * @return true if handshake is valid and ready for cracking
 * @return false if not enough messages captured
 */
bool hccapx_serializer_is_valid();

/**
 * @brief Adds new handshake frames into current HCCAPX.
 * 
 * This function will process given frames and extract data that are relevant.
 * If frame contains handshake from another STA than the one that was already added before,
 * frame will be skipped and error message will be printed.
 * 
 * @param frame data frame with EAPoL-Key packet
 */
void hccapx_serializer_add_frame(data_frame_t *frame);


/**
 * @brief Fills the buffer with HC22000 formatted string
 * 
 * Format: WPA*02*MIC*MAC_AP*MAC_STA*ESSID*NONCE_AP*EAPOL*MESSAGE_PAIR
 * 
 * @param buffer buffer to store the string
 * @param max_len maximum length of the buffer
 * @return int number of characters written, or -1 if error
 */
int hccapx_serializer_get_22000_string(char *buffer, size_t max_len);

#endif
