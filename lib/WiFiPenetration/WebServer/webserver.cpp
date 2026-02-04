#include "WebServer/webserver.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "AttackLog/log.h"
#include "esp_http_server.h"
#include "esp_wifi_types.h"

#include "WifiController/wifi_controller.h"
#include "Attack/attack.h"
#include "PcapSerializer/pcap_serializer.h"
#include "HccapxSerializer/hccapx_serializer.h"

#include "WebServer/pages/page_index.h"

static const char* TAG = "webserver";
ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

/**
 * @brief Handlers for index/root \c / path endpoint
 */
static esp_err_t uri_root_get_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char*)page_index, page_index_len);
}

static httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /reset endpoint
 */
static esp_err_t uri_reset_head_handler(httpd_req_t* req) {
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, NULL, 0, portMAX_DELAY));
    return httpd_resp_send(req, NULL, 0);
}

static httpd_uri_t uri_reset_head = {
    .uri = "/reset",
    .method = HTTP_HEAD,
    .handler = uri_reset_head_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /ap-list endpoint
 */
static esp_err_t uri_ap_list_get_handler(httpd_req_t* req) {
    wifictl_scan_nearby_aps();

    const wifictl_ap_records_t* ap_records;
    ap_records = wifictl_get_ap_records();

    // 33 SSID + 6 BSSID + 1 RSSI
    char resp_chunk[40];

    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    for (unsigned i = 0; i < ap_records->count; i++) {
        memcpy(resp_chunk, ap_records->records[i].ssid, 33);
        memcpy(&resp_chunk[33], ap_records->records[i].bssid, 6);
        memcpy(&resp_chunk[39], &ap_records->records[i].rssi, 1);
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, resp_chunk, 40));
    }
    return httpd_resp_send_chunk(req, resp_chunk, 0);
}

static httpd_uri_t uri_ap_list_get = {
    .uri = "/ap-list",
    .method = HTTP_GET,
    .handler = uri_ap_list_get_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /run-attack endpoint
 */
static esp_err_t uri_run_attack_post_handler(httpd_req_t* req) {
    attack_request_t attack_request;
    httpd_req_recv(req, (char*)&attack_request, sizeof(attack_request_t));
    esp_err_t res = httpd_resp_send(req, NULL, 0);
    ESP_ERROR_CHECK(esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request, sizeof(attack_request_t), portMAX_DELAY));
    return res;
}

static httpd_uri_t uri_run_attack_post = {
    .uri = "/run-attack",
    .method = HTTP_POST,
    .handler = uri_run_attack_post_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /status endpoint
 */
static esp_err_t uri_status_get_handler(httpd_req_t* req) {
    ATTACK_LOG_D(TAG, "Fetching attack status...");
    const attack_status_t* attack_status;
    attack_status = attack_get_status();

    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    // first send attack result header (state, type, content_size little-endian)
    uint8_t header[4];
    header[0] = attack_status->state;
    header[1] = attack_status->type;
    header[2] = attack_status->content_size & 0xFF;
    header[3] = (attack_status->content_size >> 8) & 0xFF;
    esp_err_t res = httpd_resp_send_chunk(req, (char*)header, 4);
    if (res != ESP_OK) {
        ATTACK_LOG_E(TAG, "Status hdr send fail: %d", res);
        return res;
    }
    // send attack result content
    if (((attack_status->state == FINISHED) || (attack_status->state == TIMEOUT)) && (attack_status->content_size > 0)) {
        res = httpd_resp_send_chunk(req, attack_status->content, attack_status->content_size);
        if (res != ESP_OK) {
            ATTACK_LOG_E(TAG, "Status body send fail: %d", res);
            return res;
        }
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

static httpd_uri_t uri_status_get = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = uri_status_get_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /capture.pcap endpoint
 */
static esp_err_t uri_capture_pcap_get_handler(httpd_req_t* req) {
    ATTACK_LOG_D(TAG, "Providing PCAP file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char*)pcap_serializer_get_buffer(), pcap_serializer_get_size());
}

static httpd_uri_t uri_capture_pcap_get = {
    .uri = "/capture.pcap",
    .method = HTTP_GET,
    .handler = uri_capture_pcap_get_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /capture.hc22000 endpoint
 */
static esp_err_t uri_capture_hc22000_get_handler(httpd_req_t* req) {
    ATTACK_LOG_D(TAG, "Providing HC22000 file...");
    
    // Max size calculation: ~713 bytes. Round up to 1024.
    const size_t buf_size = 1024;
    char *buffer = (char *)malloc(buf_size);
    if (!buffer) {
        ATTACK_LOG_E(TAG, "HC22000 buf alloc fail");
        return ESP_ERR_NO_MEM;
    }

    int len = hccapx_serializer_get_22000_string(buffer, buf_size);
    if (len <= 0) {
        free(buffer);
        ATTACK_LOG_W(TAG, "No HC22000 data");
        // Could return 404 or 204. Returning empty file implies check failed or empty.
        // Let's return 204 No Content if empty or just empty response.
        // Existing handlers seem to just send empty or fail silently?
        // Let's send 500 or just empty text.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No valid handshake data");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/plain"));
    // Content-Disposition to force download with specific filename
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"capture.hc22000\"");
    
    esp_err_t res = httpd_resp_send(req, buffer, len);
    free(buffer);
    return res;
}

static httpd_uri_t uri_capture_hc22000_get = {
    .uri = "/capture.hc22000",
    .method = HTTP_GET,
    .handler = uri_capture_hc22000_get_handler,
    .user_ctx = NULL
};

/**
 * @brief Handlers for \c /capture.hccapx endpoint
 */
static esp_err_t uri_capture_hccapx_get_handler(httpd_req_t* req) {
    ATTACK_LOG_D(TAG, "Providing HCCAPX file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char*)hccapx_serializer_get(), sizeof(hccapx_t));
}

static httpd_uri_t uri_capture_hccapx_get = {
    .uri = "/capture.hccapx",
    .method = HTTP_GET,
    .handler = uri_capture_hccapx_get_handler,
    .user_ctx = NULL
};


void webserver_run() {
    ATTACK_LOG_D(TAG, "Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_root_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_reset_head));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_ap_list_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_run_attack_post));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_status_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_pcap_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_hccapx_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_capture_hc22000_get));
}
