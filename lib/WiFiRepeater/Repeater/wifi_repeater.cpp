#include "Repeater/wifi_repeater.h"
#include "Config/wifi_repeater_config.h"
#include "NVS/repeater_nvs.h"
#include "WebServer/repeater_webserver.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/lwip_napt.h"
#include "lwip/ip4_addr.h"
#include "dhcpserver/dhcpserver.h"
#include "dhcpserver/dhcpserver_options.h"

static const char* TAG = "WiFiRepeater";

// Runtime state
static repeater_config_t rpt_config;
static volatile bool sta_connected = false;
static volatile uint16_t client_count = 0;
static uint32_t boot_time_ms = 0;
static volatile bool rpt_running = false;
static DNSServer dnsServer;

// Forward declarations
static void rpt_wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info);
static void rpt_start_ap();
static void rpt_connect_sta();
static void rpt_enable_napt();
static void rpt_setup_dns_for_ap();

void repeater_init() {
    boot_time_ms = millis();

    // Load saved config from NVS
    rpt_nvs_load(&rpt_config);

    ESP_LOGI(TAG, "STA SSID: '%s'", rpt_config.sta_ssid);
    ESP_LOGI(TAG, "AP  SSID: '%s'", rpt_config.ap_ssid);

    // Register WiFi events
    WiFi.onEvent(rpt_wifi_event_handler);

    // Set WiFi mode to AP+STA
    WiFi.mode(WIFI_AP_STA);

    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setAutoReconnect(true);

    // Start AP first
    rpt_start_ap();
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40);

    // Connect STA if configured
    if (strlen(rpt_config.sta_ssid) > 0) {
        rpt_connect_sta();
        esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40);
    } else {
        ESP_LOGI(TAG, "No upstream WiFi configured. Use web UI at http://%s to configure.", WiFi.softAPIP().toString().c_str());
    }

    // Start the web server and DNS captive portal server
    rpt_webserver_start(&rpt_config);
    dnsServer.start(53, "*", WiFi.softAPIP());

    // Initialize traffic stats netif hooks
    rpt_stats_init();

    rpt_running = true;
    ESP_LOGI(TAG, "=== WiFi Repeater Ready ===");
    ESP_LOGI(TAG, "Web UI: http://%s", WiFi.softAPIP().toString().c_str());
}

void repeater_apply_config(const char* sta_ssid, const char* sta_pass, const char* ap_ssid, const char* ap_pass) {
    if (!sta_ssid) sta_ssid = "";
    if (!sta_pass) sta_pass = "";
    if (!ap_ssid || strlen(ap_ssid) == 0) ap_ssid = RPT_DEFAULT_AP_SSID;
    if (!ap_pass) ap_pass = "";

    // 1. Save to NVS
    rpt_nvs_save_sta(sta_ssid, sta_pass);
    rpt_nvs_save_ap(ap_ssid, ap_pass);

    // 2. Update RAM config struct
    strlcpy(rpt_config.sta_ssid, sta_ssid, sizeof(rpt_config.sta_ssid));
    strlcpy(rpt_config.sta_pass, sta_pass, sizeof(rpt_config.sta_pass));
    strlcpy(rpt_config.ap_ssid,  ap_ssid,  sizeof(rpt_config.ap_ssid));
    strlcpy(rpt_config.ap_pass,  ap_pass,  sizeof(rpt_config.ap_pass));

    ESP_LOGI(TAG, "Applying config on-the-fly: STA='%s', AP='%s'", rpt_config.sta_ssid, rpt_config.ap_ssid);

    // 3. Re-configure AP on-the-fly
    rpt_start_ap();
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40);

    // 4. Re-configure STA on-the-fly
    sta_connected = false;
    WiFi.disconnect();

    if (strlen(rpt_config.sta_ssid) > 0) {
        rpt_connect_sta();
        esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40);
    }
}

static void rpt_start_ap() {
    IPAddress ap_ip;
    ap_ip.fromString(RPT_DEFAULT_AP_IP);
    IPAddress ap_gateway = ap_ip;
    IPAddress ap_subnet(255, 255, 255, 0);

    // Configure AP with softAP
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);

    if (strlen(rpt_config.ap_pass) >= 8) {
        WiFi.softAP(rpt_config.ap_ssid, rpt_config.ap_pass, RPT_DEFAULT_AP_CHANNEL, 0, RPT_AP_MAX_CONNECTIONS);
    } else {
        // Open AP (no password or password too short)
        WiFi.softAP(rpt_config.ap_ssid, NULL, RPT_DEFAULT_AP_CHANNEL, 0, RPT_AP_MAX_CONNECTIONS);
    }

    ESP_LOGI(TAG, "AP started: SSID='%s' IP=%s", rpt_config.ap_ssid, WiFi.softAPIP().toString().c_str());
}

static void rpt_connect_sta() {
    ESP_LOGI(TAG, "Connecting STA to: '%s'", rpt_config.sta_ssid);
    WiFi.begin(rpt_config.sta_ssid, rpt_config.sta_pass);
}

static void rpt_enable_napt() {
    // Enable NAPT on the AP interface
    // The AP IP address is needed for ip_napt_enable
    ip4_addr_t ap_ip;
    IP4_ADDR(&ap_ip, 192, 168, 4, 1);

    // Parse actual AP IP
    IPAddress softApIp = WiFi.softAPIP();
    IP4_ADDR(&ap_ip, softApIp[0], softApIp[1], softApIp[2], softApIp[3]);

    ip_napt_enable(ap_ip.addr, 1);
    ESP_LOGI(TAG, "NAPT enabled - packets will be forwarded from AP to STA");
}

static void rpt_setup_dns_for_ap() {
    // Copy DNS server from STA to AP's DHCP server
    // so AP clients can resolve domain names
    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (ap_netif && sta_netif) {
        esp_netif_dns_info_t dns_info;
        if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
            esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_info);
            ESP_LOGI(TAG, "DNS forwarded to AP: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        }

        // Enable DNS offer in DHCP
        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        esp_netif_dhcps_stop(ap_netif);
        esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET,
                               ESP_NETIF_DOMAIN_NAME_SERVER,
                               &dhcps_dns_value, sizeof(dhcps_dns_value));
        esp_netif_dhcps_start(ap_netif);
    }
}

static void rpt_wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "STA connected! IP: %s", WiFi.localIP().toString().c_str());
            sta_connected = true;

            // Setup DNS forwarding and enable NAPT
            rpt_setup_dns_for_ap();
            rpt_enable_napt();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGW(TAG, "STA disconnected from upstream WiFi");
            sta_connected = false;
            // Auto-reconnect is handled by WiFi.setAutoReconnect(true)
            // Do NOT call delay() or WiFi.begin() here — this runs in the WiFi task
            // context with a small stack, and blocking will trigger Task Watchdog panic.
            break;

        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            client_count++;
            ESP_LOGI(TAG, "AP client connected. Total: %d", client_count);
            break;
        }

        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
            if (client_count > 0) client_count--;
            ESP_LOGI(TAG, "AP client disconnected. Total: %d", client_count);
            break;
        }

        default:
            break;
    }
}

bool repeater_is_sta_connected() {
    return sta_connected;
}

uint16_t repeater_get_client_count() {
    return client_count;
}

uint32_t repeater_get_sta_ip() {
    if (!sta_connected) return 0;
    return (uint32_t)WiFi.localIP();
}

const char* repeater_get_sta_ssid() {
    return rpt_config.sta_ssid;
}

uint32_t repeater_get_ap_ip() {
    return (uint32_t)WiFi.softAPIP();
}

const char* repeater_get_ap_ssid() {
    return rpt_config.ap_ssid;
}

const char* repeater_get_ap_pass() {
    return rpt_config.ap_pass;
}

uint32_t repeater_get_uptime_seconds() {
    return (millis() - boot_time_ms) / 1000;
}

void repeater_tick() {
    if (rpt_running) {
        // Only run Captive Portal DNS hijacking when upstream STA is DISCONNECTED.
        // When STA is connected, let AP clients resolve real Internet domain names!
        if (!sta_connected) {
            dnsServer.processNextRequest();
        }
        rpt_stats_tick();
    }
}

void repeater_stop() {
    rpt_running = false;
    dnsServer.stop();
    rpt_webserver_stop();
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    sta_connected = false;
    client_count = 0;
    ESP_LOGI(TAG, "WiFi Repeater stopped");
}
