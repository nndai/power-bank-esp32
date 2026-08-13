#include "NVS/repeater_nvs.h"
#include "Config/wifi_repeater_config.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include "esp_log.h"

static const char* TAG = "RptNVS";
static Preferences prefs;

/**
 * @brief Ensure NVS flash is initialized. If corrupted, erase and re-init.
 */
static void rpt_nvs_ensure_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrupted (err=0x%x), erasing and re-init...", err);
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS flash init failed: %s", esp_err_to_name(err));
    }
}

void rpt_nvs_load(repeater_config_t* config) {
    // Ensure NVS flash is initialized
    rpt_nvs_ensure_init();

    // Set defaults first in case prefs.begin() fails
    strlcpy(config->sta_ssid, "", sizeof(config->sta_ssid));
    strlcpy(config->sta_pass, "", sizeof(config->sta_pass));
    strlcpy(config->ap_ssid,  RPT_DEFAULT_AP_SSID, sizeof(config->ap_ssid));
    strlcpy(config->ap_pass,  RPT_DEFAULT_AP_PASS, sizeof(config->ap_pass));

    if (!prefs.begin(RPT_NVS_NAMESPACE, true)) { // read-only mode
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s', using defaults", RPT_NVS_NAMESPACE);
        return;
    }

    String sta_ssid = prefs.getString("sta_ssid", "");
    String sta_pass = prefs.getString("sta_pass", "");
    String ap_ssid  = prefs.getString("ap_ssid", RPT_DEFAULT_AP_SSID);
    String ap_pass  = prefs.getString("ap_pass", RPT_DEFAULT_AP_PASS);

    strlcpy(config->sta_ssid, sta_ssid.c_str(), sizeof(config->sta_ssid));
    strlcpy(config->sta_pass, sta_pass.c_str(), sizeof(config->sta_pass));
    strlcpy(config->ap_ssid,  ap_ssid.c_str(),  sizeof(config->ap_ssid));
    strlcpy(config->ap_pass,  ap_pass.c_str(),  sizeof(config->ap_pass));

    prefs.end();
    ESP_LOGI(TAG, "Config loaded OK: STA='%s', AP='%s'", config->sta_ssid, config->ap_ssid);
}

void rpt_nvs_save_sta(const char* ssid, const char* pass) {
    if (!prefs.begin(RPT_NVS_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open NVS for writing STA config");
        return;
    }
    prefs.putString("sta_ssid", ssid);
    prefs.putString("sta_pass", pass);
    prefs.end();
}

void rpt_nvs_save_ap(const char* ssid, const char* pass) {
    if (!prefs.begin(RPT_NVS_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open NVS for writing AP config");
        return;
    }
    prefs.putString("ap_ssid", ssid);
    prefs.putString("ap_pass", pass);
    prefs.end();
}

void rpt_nvs_erase_all() {
    if (!prefs.begin(RPT_NVS_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open NVS for erase");
        return;
    }
    prefs.clear();
    prefs.end();
    ESP_LOGI(TAG, "NVS config erased");
}
