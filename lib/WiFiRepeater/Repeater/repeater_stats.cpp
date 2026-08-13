#include "Repeater/repeater_stats.h"
#include "esp_log.h"

static const char* TAG = "RptStats";

#ifdef __cplusplus
extern "C" {
#endif
struct netif *esp_netif_get_netif_impl(esp_netif_t *esp_netif);
#ifdef __cplusplus
}
#endif

static volatile uint64_t s_rx_bytes_total = 0;
static volatile uint64_t s_tx_bytes_total = 0;

static uint64_t s_prev_rx_bytes = 0;
static uint64_t s_prev_tx_bytes = 0;
static uint32_t s_rx_speed_bps = 0;
static uint32_t s_tx_speed_bps = 0;
static uint32_t s_last_calc_ms = 0;

static netif_input_fn s_orig_input = NULL;
static netif_linkoutput_fn s_orig_linkoutput = NULL;

static err_t rpt_netif_input_hook(struct pbuf *p, struct netif *inp) {
    if (p != NULL) {
        s_rx_bytes_total += p->tot_len;
    }
    if (s_orig_input != NULL) {
        return s_orig_input(p, inp);
    }
    return ERR_VAL;
}

static err_t rpt_netif_linkoutput_hook(struct netif *netif, struct pbuf *p) {
    if (p != NULL) {
        s_tx_bytes_total += p->tot_len;
    }
    if (s_orig_linkoutput != NULL) {
        return s_orig_linkoutput(netif, p);
    }
    return ERR_IF;
}

void rpt_stats_init() {
    s_rx_bytes_total = 0;
    s_tx_bytes_total = 0;
    s_prev_rx_bytes = 0;
    s_prev_tx_bytes = 0;
    s_rx_speed_bps = 0;
    s_tx_speed_bps = 0;
    s_last_calc_ms = millis();

    esp_netif_t *sta_handle = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_handle != NULL) {
        struct netif *sta_nif = esp_netif_get_netif_impl(sta_handle);
        if (sta_nif != NULL && s_orig_input == NULL) {
            s_orig_input = sta_nif->input;
            sta_nif->input = rpt_netif_input_hook;
            s_orig_linkoutput = sta_nif->linkoutput;
            sta_nif->linkoutput = rpt_netif_linkoutput_hook;
            ESP_LOGI(TAG, "Traffic netif hooks installed on WIFI_STA_DEF");
        }
    }
}

void rpt_stats_tick() {
    uint32_t now = millis();
    uint32_t dt = now - s_last_calc_ms;
    if (dt >= 1000) {
        uint64_t current_rx = s_rx_bytes_total;
        uint64_t current_tx = s_tx_bytes_total;

        uint64_t rx_delta = (current_rx >= s_prev_rx_bytes) ? (current_rx - s_prev_rx_bytes) : 0;
        uint64_t tx_delta = (current_tx >= s_prev_tx_bytes) ? (current_tx - s_prev_tx_bytes) : 0;

        s_rx_speed_bps = (uint32_t)((rx_delta * 1000) / dt);
        s_tx_speed_bps = (uint32_t)((tx_delta * 1000) / dt);

        s_prev_rx_bytes = current_rx;
        s_prev_tx_bytes = current_tx;
        s_last_calc_ms = now;
    }
}

void rpt_stats_get_traffic(rpt_traffic_stats_t* stats) {
    if (!stats) return;
    stats->rx_bytes_total = s_rx_bytes_total;
    stats->tx_bytes_total = s_tx_bytes_total;
    stats->rx_speed_bps = s_rx_speed_bps;
    stats->tx_speed_bps = s_tx_speed_bps;
}

int rpt_stats_get_clients(rpt_client_t* clients, int max_clients) {
    if (!clients || max_clients <= 0) return 0;

    wifi_sta_list_t sta_list;
    if (esp_wifi_ap_get_sta_list(&sta_list) != ESP_OK) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < sta_list.num && count < max_clients; i++) {
        wifi_sta_info_t *sta = &sta_list.sta[i];
        memcpy(clients[count].mac, sta->mac, 6);
        snprintf(clients[count].mac_str, sizeof(clients[count].mac_str),
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 sta->mac[0], sta->mac[1], sta->mac[2],
                 sta->mac[3], sta->mac[4], sta->mac[5]);
        clients[count].rssi = sta->rssi;

        ip4_addr_t ip_addr;
        if (dhcp_search_ip_on_mac(sta->mac, &ip_addr)) {
            clients[count].has_ip = true;
            snprintf(clients[count].ip_str, sizeof(clients[count].ip_str),
                     IPSTR, IP2STR(&ip_addr));
        } else {
            clients[count].has_ip = false;
            snprintf(clients[count].ip_str, sizeof(clients[count].ip_str), "Assigning...");
        }

        count++;
    }
    return count;
}

void rpt_format_bytes(uint64_t bytes, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    if (bytes >= 1073741824ULL) {
        snprintf(buf, buf_size, "%.1f GB", (double)bytes / 1073741824.0);
    } else if (bytes >= 1048576ULL) {
        snprintf(buf, buf_size, "%.1f MB", (double)bytes / 1048576.0);
    } else if (bytes >= 1024ULL) {
        snprintf(buf, buf_size, "%.1f KB", (double)bytes / 1024.0);
    } else {
        snprintf(buf, buf_size, "%llu B", (unsigned long long)bytes);
    }
}

void rpt_format_speed(uint32_t bytes_per_sec, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    char bytes_buf[32];
    rpt_format_bytes((uint64_t)bytes_per_sec, bytes_buf, sizeof(bytes_buf));
    snprintf(buf, buf_size, "%s/s", bytes_buf);
}
