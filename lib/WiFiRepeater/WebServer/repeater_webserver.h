#ifndef REPEATER_WEBSERVER_H
#define REPEATER_WEBSERVER_H

#include <Arduino.h>
#include "NVS/repeater_nvs.h"

/**
 * @brief Start the repeater web server
 * @param config Pointer to the runtime config (for reading current settings)
 */
void rpt_webserver_start(repeater_config_t* config);

/**
 * @brief Stop the repeater web server
 */
void rpt_webserver_stop();

#endif // REPEATER_WEBSERVER_H
