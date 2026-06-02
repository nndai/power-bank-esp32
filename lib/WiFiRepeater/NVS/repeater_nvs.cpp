#include "NVS/repeater_nvs.h"
#include "Config/wifi_repeater_config.h"
#include <Preferences.h>

static Preferences prefs;

void rpt_nvs_load(repeater_config_t* config) {
    prefs.begin(RPT_NVS_NAMESPACE, true); // read-only

    String sta_ssid = prefs.getString("sta_ssid", "");
    String sta_pass = prefs.getString("sta_pass", "");
    String ap_ssid  = prefs.getString("ap_ssid", RPT_DEFAULT_AP_SSID);
    String ap_pass  = prefs.getString("ap_pass", RPT_DEFAULT_AP_PASS);

    strlcpy(config->sta_ssid, sta_ssid.c_str(), sizeof(config->sta_ssid));
    strlcpy(config->sta_pass, sta_pass.c_str(), sizeof(config->sta_pass));
    strlcpy(config->ap_ssid,  ap_ssid.c_str(),  sizeof(config->ap_ssid));
    strlcpy(config->ap_pass,  ap_pass.c_str(),  sizeof(config->ap_pass));

    prefs.end();
}

void rpt_nvs_save_sta(const char* ssid, const char* pass) {
    prefs.begin(RPT_NVS_NAMESPACE, false); // read-write
    prefs.putString("sta_ssid", ssid);
    prefs.putString("sta_pass", pass);
    prefs.end();
}

void rpt_nvs_save_ap(const char* ssid, const char* pass) {
    prefs.begin(RPT_NVS_NAMESPACE, false);
    prefs.putString("ap_ssid", ssid);
    prefs.putString("ap_pass", pass);
    prefs.end();
}

void rpt_nvs_erase_all() {
    prefs.begin(RPT_NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
}
