#ifndef WIFI_REPEATER_CONFIG_H
#define WIFI_REPEATER_CONFIG_H

// Default AP settings for the repeater's own WiFi network
#ifndef RPT_DEFAULT_AP_SSID
#define RPT_DEFAULT_AP_SSID       "ESP32_Repeater"
#endif

#ifndef RPT_DEFAULT_AP_PASS
#define RPT_DEFAULT_AP_PASS       ""
#endif

#ifndef RPT_DEFAULT_AP_IP
#define RPT_DEFAULT_AP_IP         "192.168.4.1"
#endif

#ifndef RPT_DEFAULT_AP_CHANNEL
#define RPT_DEFAULT_AP_CHANNEL    1
#endif

#ifndef RPT_AP_MAX_CONNECTIONS
#define RPT_AP_MAX_CONNECTIONS    8
#endif

// NVS namespace for repeater config (max 15 chars)
#define RPT_NVS_NAMESPACE         "wifi_rpt"

// NAPT max entries
#ifndef IP_NAPT_MAX
#define IP_NAPT_MAX               64
#endif

// Web server port
#ifndef RPT_WEB_PORT
#define RPT_WEB_PORT              80
#endif

// Max APs to scan
#define RPT_SCAN_MAX_AP           30

// Reconnect delay (ms) when STA disconnects
#define RPT_RECONNECT_DELAY_MS    5000

#endif // WIFI_REPEATER_CONFIG_H
