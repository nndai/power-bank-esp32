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

// Progress callback for visual debugging
static repeater_progress_cb_t s_progress_cb = NULL;

static void report_step(int step, const char* msg) {
    //ESP_LOGI(TAG, "[Step %d] %s", step, msg);
    if (s_progress_cb) {
        s_progress_cb(step, msg);
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // small delay so screen updates before potential crash
}

void repeater_set_progress_callback(repeater_progress_cb_t cb) {
    s_progress_cb = cb;
}

// Forward declarations
static void rpt_wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info);
static void rpt_start_ap();
static void rpt_connect_sta();
static void rpt_enable_napt();
static void rpt_setup_dns_for_ap();

void repeater_init() {
    report_step(1, "NVS load");
    boot_time_ms = millis();

    // Load saved config from NVS
    rpt_nvs_load(&rpt_config);

    ESP_LOGI(TAG, "STA SSID: '%s'", rpt_config.sta_ssid);
    ESP_LOGI(TAG, "AP  SSID: '%s'", rpt_config.ap_ssid);

    report_step(2, "WiFi events");
    // Register WiFi events
    WiFi.onEvent(rpt_wifi_event_handler);

    report_step(3, "WiFi AP_STA mode");
    // Set WiFi mode to AP+STA
    WiFi.mode(WIFI_AP_STA);

    report_step(4, "WiFi PS off");
    esp_wifi_set_ps(WIFI_PS_NONE);

    report_step(5, "AutoReconnect");
    WiFi.setAutoReconnect(true);

    report_step(6, "Start AP");
    // Start AP first
    rpt_start_ap();

    report_step(7, "AP BW HT40");
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40);

    // Connect STA if configured
    if (strlen(rpt_config.sta_ssid) > 0) {
        report_step(8, "Connect STA");
        rpt_connect_sta();

        report_step(9, "STA BW HT40");
        esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40);
    } else {
        report_step(8, "No STA config");
        ESP_LOGI(TAG, "No upstream WiFi configured. Use web UI at 192.168.4.1 to configure.");
    }

    report_step(10, "Web server");
    // Start the web server and DNS captive portal server
    rpt_webserver_start(&rpt_config);
    dnsServer.start(53, "*", WiFi.softAPIP());

    rpt_running = true;
    report_step(11, "DONE!");
    ESP_LOGI(TAG, "=== WiFi Repeater Ready ===");
    ESP_LOGI(TAG, "Web UI: http://192.168.4.1");
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

uint32_t repeater_get_ap_ip() {
    return (uint32_t)WiFi.softAPIP();
}

uint32_t repeater_get_uptime_seconds() {
    return (millis() - boot_time_ms) / 1000;
}

void repeater_tick() {
    if (rpt_running) {
        dnsServer.processNextRequest();
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
