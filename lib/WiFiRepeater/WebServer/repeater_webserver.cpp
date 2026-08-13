#include "WebServer/repeater_webserver.h"
#include "Config/wifi_repeater_config.h"
#include "Repeater/wifi_repeater.h"
#include "NVS/repeater_nvs.h"
#include "WebServer/pages/page_repeater.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include "esp_log.h"

static const char* TAG = "RptWebServer";

static httpd_handle_t server = NULL;
static repeater_config_t* p_config = NULL;

// ==================== URL-decode helper ====================
static void url_decode(char* dst, const char* src, size_t dst_size) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di < dst_size - 1; si++) {
        if (src[si] == '%' && src[si+1] && src[si+2]) {
            char hex[3] = { src[si+1], src[si+2], 0 };
            dst[di++] = (char)strtol(hex, NULL, 16);
            si += 2;
        } else if (src[si] == '+') {
            dst[di++] = ' ';
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

// ==================== GET / - Status Page ====================
static esp_err_t handler_index(httpd_req_t* req) {
    char row[512];

    httpd_resp_set_type(req, "text/html");

    // Send head
    httpd_resp_send_chunk(req, PAGE_REPEATER_HEAD, HTTPD_RESP_USE_STRLEN);

    // === Status Card ===
    httpd_resp_send_chunk(req, "<div class='card'><h2>📊 Status</h2><table>", HTTPD_RESP_USE_STRLEN);

    // STA Connection
    if (repeater_is_sta_connected()) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            snprintf(row, sizeof(row),
                "<tr><td>Uplink:</td><td class='online'><strong>Connected (%d dBm)</strong></td></tr>",
                ap_info.rssi);
        } else {
            snprintf(row, sizeof(row),
                "<tr><td>Uplink:</td><td class='online'><strong>Connected</strong></td></tr>");
        }
    } else {
        snprintf(row, sizeof(row),
            "<tr><td>Uplink:</td><td class='offline'><strong>Disconnected</strong></td></tr>");
    }
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // STA SSID
    snprintf(row, sizeof(row), "<tr><td>STA SSID:</td><td>%s</td></tr>",
             strlen(p_config->sta_ssid) > 0 ? p_config->sta_ssid : "<em>Not configured</em>");
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // STA IP
    if (repeater_is_sta_connected()) {
        snprintf(row, sizeof(row), "<tr><td>STA IP:</td><td>%s</td></tr>",
                 WiFi.localIP().toString().c_str());
    } else {
        snprintf(row, sizeof(row), "<tr><td>STA IP:</td><td>N/A</td></tr>");
    }
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // AP SSID
    snprintf(row, sizeof(row), "<tr><td>AP SSID:</td><td><strong>%s</strong></td></tr>",
             p_config->ap_ssid);
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // AP IP
    snprintf(row, sizeof(row), "<tr><td>AP IP:</td><td>%s</td></tr>",
             WiFi.softAPIP().toString().c_str());
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // AP Clients
    snprintf(row, sizeof(row), "<tr><td>AP Clients:</td><td>%d</td></tr>",
             repeater_get_client_count());
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // Free Heap
    snprintf(row, sizeof(row), "<tr><td>Free Heap:</td><td>%lu bytes</td></tr>",
             (unsigned long)ESP.getFreeHeap());
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // Uptime
    uint32_t up = repeater_get_uptime_seconds();
    uint32_t h = up / 3600, m = (up % 3600) / 60, s = up % 60;
    snprintf(row, sizeof(row), "<tr><td>Uptime:</td><td>%luh %lum %lus</td></tr>",
             (unsigned long)h, (unsigned long)m, (unsigned long)s);
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    httpd_resp_send_chunk(req, "</table></div>", HTTPD_RESP_USE_STRLEN);

    // === WiFi Scanner Card ===
    httpd_resp_send_chunk(req,
        "<div class='card'>"
        "<h2>🔍 WiFi Scanner</h2>"
        "<button class='btn btn-scan' id='scanBtn' onclick='scanWifi()'>🔍 Scan WiFi</button>"
        "<div class='ap-list' id='apList'></div>"
        "</div>", HTTPD_RESP_USE_STRLEN);

    // === Configuration Card ===
    httpd_resp_send_chunk(req,
        "<div class='card'>"
        "<h2>⚙️ Configuration</h2>", HTTPD_RESP_USE_STRLEN);

    // STA config
    snprintf(row, sizeof(row),
        "<label>Upstream WiFi SSID</label>"
        "<input type='text' id='sta_ssid' value='%s' placeholder='Network name'>"
        "<label>Upstream WiFi Password</label>"
        "<input type='password' id='sta_pass' value='%s' placeholder='Password'>",
        p_config->sta_ssid, p_config->sta_pass);
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // AP config
    snprintf(row, sizeof(row),
        "<hr style='border-color:rgba(0,217,255,0.1);margin:0.8rem 0'>"
        "<label>Repeater AP SSID</label>"
        "<input type='text' id='ap_ssid' value='%s' placeholder='Repeater name'>"
        "<label>Repeater AP Password</label>"
        "<input type='password' id='ap_pass' value='%s' placeholder='Min 8 chars or empty for open'>",
        p_config->ap_ssid, p_config->ap_pass);
    httpd_resp_send_chunk(req, row, HTTPD_RESP_USE_STRLEN);

    // Save button
    httpd_resp_send_chunk(req,
        "<button class='btn btn-primary' id='saveBtn' onclick='saveConfig()'>💾 Save & Restart</button>"
        "</div>", HTTPD_RESP_USE_STRLEN);

    // Send tail (includes scripts)
    httpd_resp_send_chunk(req, PAGE_REPEATER_TAIL, HTTPD_RESP_USE_STRLEN);

    // End chunked response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// ==================== GET /scan - WiFi Scan ====================
static esp_err_t handler_scan(httpd_req_t* req) {
    ESP_LOGI(TAG, "Scanning WiFi networks...");

    int16_t n = WiFi.scanNetworks(false, true); // sync scan, show hidden

    httpd_resp_set_type(req, "application/json");

    if (n <= 0) {
        httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);
        WiFi.scanDelete();
        return ESP_OK;
    }

    // Build JSON array
    // Each entry: {"ssid":"...","rssi":-xx,"enc":x}
    // Estimate: ~80 bytes per entry
    size_t buf_size = (size_t)n * 80 + 16;
    char* buf = (char*)malloc(buf_size);
    if (!buf) {
        httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);
        WiFi.scanDelete();
        return ESP_OK;
    }

    int pos = 0;
    pos += snprintf(buf + pos, buf_size - pos, "[");

    for (int i = 0; i < n && i < RPT_SCAN_MAX_AP; i++) {
        if (i > 0) pos += snprintf(buf + pos, buf_size - pos, ",");

        // Escape SSID for JSON (basic escape for quotes and backslashes)
        String ssid = WiFi.SSID(i);
        String escaped = "";
        for (unsigned int c = 0; c < ssid.length(); c++) {
            char ch = ssid.charAt(c);
            if (ch == '"') escaped += "\\\"";
            else if (ch == '\\') escaped += "\\\\";
            else escaped += ch;
        }

        pos += snprintf(buf + pos, buf_size - pos,
            "{\"ssid\":\"%s\",\"rssi\":%d,\"enc\":%d}",
            escaped.c_str(), WiFi.RSSI(i), WiFi.encryptionType(i));
    }

    pos += snprintf(buf + pos, buf_size - pos, "]");

    httpd_resp_send(req, buf, pos);
    free(buf);
    WiFi.scanDelete();

    return ESP_OK;
}

// ==================== POST /save - Save Config ====================
static esp_err_t handler_save(httpd_req_t* req) {
    // Read POST body
    int total_len = req->content_len;
    if (total_len <= 0 || total_len > 512) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"ok\":false,\"msg\":\"Invalid request\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    char* body = (char*)malloc(total_len + 1);
    if (!body) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"ok\":false,\"msg\":\"Out of memory\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    int received = 0;
    while (received < total_len) {
        int ret = httpd_req_recv(req, body + received, total_len - received);
        if (ret <= 0) {
            free(body);
            return ESP_FAIL;
        }
        received += ret;
    }
    body[total_len] = '\0';

    // Parse form data: sta_ssid=...&sta_pass=...&ap_ssid=...&ap_pass=...
    char sta_ssid_raw[65] = "";
    char sta_pass_raw[65] = "";
    char ap_ssid_raw[65] = "";
    char ap_pass_raw[65] = "";

    // Simple query string parser
    char* token = strtok(body, "&");
    while (token) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char* key = token;
            char* val_raw = eq + 1;

            char val[65];
            url_decode(val, val_raw, sizeof(val));

            if (strcmp(key, "sta_ssid") == 0) strlcpy(sta_ssid_raw, val, sizeof(sta_ssid_raw));
            else if (strcmp(key, "sta_pass") == 0) strlcpy(sta_pass_raw, val, sizeof(sta_pass_raw));
            else if (strcmp(key, "ap_ssid") == 0) strlcpy(ap_ssid_raw, val, sizeof(ap_ssid_raw));
            else if (strcmp(key, "ap_pass") == 0) strlcpy(ap_pass_raw, val, sizeof(ap_pass_raw));
        }
        token = strtok(NULL, "&");
    }
    free(body);

    // Validate AP SSID
    if (strlen(ap_ssid_raw) == 0) {
        strlcpy(ap_ssid_raw, RPT_DEFAULT_AP_SSID, sizeof(ap_ssid_raw));
    }

    // Save to NVS
    rpt_nvs_save_sta(sta_ssid_raw, sta_pass_raw);
    rpt_nvs_save_ap(ap_ssid_raw, ap_pass_raw);

    ESP_LOGI(TAG, "Config saved. STA='%s' AP='%s'", sta_ssid_raw, ap_ssid_raw);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true,\"msg\":\"Config saved. Restarting...\"}", HTTPD_RESP_USE_STRLEN);

    // Spawn one-shot background task to restart after response is fully transmitted
    xTaskCreatePinnedToCore([](void* p) {
        vTaskDelay(pdMS_TO_TICKS(1500));
        esp_restart();
    }, "RptRebootTask", 2048, NULL, 1, NULL, 1);

    return ESP_OK;
}

// ==================== URI definitions ====================
static const httpd_uri_t uri_index = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = handler_index,
    .user_ctx  = NULL
};

static const httpd_uri_t uri_scan = {
    .uri       = "/scan",
    .method    = HTTP_GET,
    .handler   = handler_scan,
    .user_ctx  = NULL
};

static const httpd_uri_t uri_save = {
    .uri       = "/save",
    .method    = HTTP_POST,
    .handler   = handler_save,
    .user_ctx  = NULL
};

// ==================== Start / Stop ====================
void rpt_webserver_start(repeater_config_t* config) {
    p_config = config;

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.server_port = RPT_WEB_PORT;
    http_config.stack_size = 8192;

    esp_err_t err = httpd_start(&server, &http_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(err));
        return;
    }

    httpd_register_uri_handler(server, &uri_index);
    httpd_register_uri_handler(server, &uri_scan);
    httpd_register_uri_handler(server, &uri_save);

    ESP_LOGI(TAG, "Web server started on port %d", RPT_WEB_PORT);
}

void rpt_webserver_stop() {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
}
