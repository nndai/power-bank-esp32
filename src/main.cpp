#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <cstring>
#include "OtaUpdate.h"
#include "luffy.h"
#include "logoAttack.h"
#include "Manager.h"
#include "esp_ota_ops.h"
#include "Globals.h"
#include "esp_task_wdt.h"
#include "soc/rtc_wdt.h"
#include <deque>

#include "Attack/attack.h"
#include "WifiController/wifi_controller.h"
#include "WebServer/webserver.h"
#include "AttackLog/log.h"



const char* ssid = "DESKTOP-P5540";
const char* password = "aaaaaaaa";

std::vector<String> sleepOptions = { "Never", "20s", "30s", "1min", "2min", "5min", "10min" };
std::vector<uint8_t> sleepValueOptions = { 0, 2, 3, 6, 12, 30, 60 }; // *10 -> second; 0 -> never

PowerBankData powerBankData;

bool isConnectedWebsocket = false;
bool isUploadFirmware = false;

IPAddress localIp(192, 168, 137, 194);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
WebSocketsServer websocket(82);

OneButton button(BUTTON_PIN, false, false);
ButtonManager buttonManager(&button);

ScreenManager* screenManager;

TFT_eSPI* tft;
TFT_eSprite* img;
Brightness* brightness;

IP5356* ip5356;
Eeprom24C* eeprom;

// --- LittleFS upload state ---
static fs::File g_uploadFile;
static bool g_uploadInProgress = false;
static size_t g_uploadExpectedSize = 0;
static size_t g_uploadWritten = 0;
static bool g_uploadFailed = false;


TaskHandle_t taskHandleDrawScreen = NULL;
TaskHandle_t taskHandleSleep = NULL;
TaskHandle_t taskHandleWifi = NULL;
TaskHandle_t taskHandleCapacityCalculator = NULL;
TaskHandle_t taskHandleChargeCycleCalculator = NULL;
TaskHandle_t taskHandleWebsocket = NULL;
TaskHandle_t taskHandleGoToSleep = NULL;


void webSocketTask(void* p);
void app_loginWifiFree(void* p);
String timeConnectWifiFree();

void _app_log_e(String mess, String end) {
    if (powerBankData.onWifi) {
        app_log_e(mess, end);
    }
}

void _app_log_i(String mess, String end) {
    if (powerBankData.onWifi) {
        app_log_i(mess, end);
    }
}

void taskDrawScreen(void* p) {
    //vTaskDelay(pdMS_TO_TICKS(200));
    uint8_t count = 0;
    while (true) {
        buttonManager.tick();
        if (screenManager->getNeedRedraw() || count == 0) {
            screenManager->setNeedRedraw(true);
            screenManager->draw();
            screenManager->setNeedRedraw(false);
            count = 0;
        }

        count++;
        if (count >= 50) count = 0;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void savePowerBankData(PowerBankData data) {
    eeprom->write(EEPROM_DATA_ADDR, &data, sizeof(data));
}

void goToSleep(void* p) {
    if (taskHandleDrawScreen) {
        vTaskDelete(taskHandleDrawScreen);
        taskHandleDrawScreen = NULL;
    }
    if (taskHandleSleep) {
        vTaskDelete(taskHandleSleep);
        taskHandleSleep = NULL;
    }

    savePowerBankData(powerBankData);

    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            if (isConnectedWebsocket) {
                esp_ota_mark_app_valid_cancel_rollback();
                _app_log_i("OTA marked as valid.");
            }
            else {
                _app_log_e("OTA pending verify. Not mark as valid because not connected websocket.");
            }
        }
        else {
            _app_log_i("OTA state: " + String(ota_state));
        }
    }


    screenManager->playVideo("/videos/turnOff.vd", false);
    brightness->on();
    vTaskDelay(pdMS_TO_TICKS(3000));

    if (taskHandleCapacityCalculator) {
        vTaskDelete(taskHandleCapacityCalculator);
        taskHandleCapacityCalculator = NULL;
    }
    if (taskHandleChargeCycleCalculator) {
        vTaskDelete(taskHandleChargeCycleCalculator);
        taskHandleChargeCycleCalculator = NULL;
    }
    brightness->off();
    ip5356->powerOff();

    _app_log_i("DeepSleep started");
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_deep_sleep_start();
}

void sleep() {
    if (taskHandleGoToSleep) return;
    xTaskCreatePinnedToCore(goToSleep, "sleep", 4096, NULL, 6, &taskHandleGoToSleep, 0);
}

void taskSleep(void* p) {
    uint32_t timeIsysLowStart = millis();
    bool isIsysLow = false;
    while (true) {
        if (ip5356->checkIsysLow()) {
            if (!isIsysLow) {
                isIsysLow = true;
                timeIsysLowStart = millis();
                brightness->on();
            }
        }
        else {
            if (isIsysLow) {
                isIsysLow = false;
                brightness->on();
            }

        }

        if (powerBankData.indexTimeAutoSleep != 0 && screenManager->getMode() != ScreenManager::ModeScreen::PLAY_VIDEO) {
            uint32_t idleTime = buttonManager.getIdleTime();
            if (idleTime > sleepValueOptions[powerBankData.indexTimeAutoSleep] * 10000) {
                if (isIsysLow) {
                    if (millis() - timeIsysLowStart > sleepValueOptions[powerBankData.indexTimeAutoSleep] * 10000) {
                        if (!OtaUpdate::isRunning()) {
                            sleep();
                        }
                    }
                }
                else if (ip5356->getChargeStatus() == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    if (ip5356->getChargeStatus() == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
                        if (ip5356->getBatteryPercentage() == 100) {
                            esp_sleep_enable_ext1_wakeup(BITMASK_DEEPSLEEP, ESP_EXT1_WAKEUP_ALL_LOW);
                            ip5356->isysLowAutoOff(true);
                            ip5356->set8sAutoOff();
                            sleep();
                        }
                    }
                }
                if (powerBankData.onLowBright) {
                    brightness->lowBrightness();
                }
            }
            else if (powerBankData.onLowBright && idleTime > sleepValueOptions[powerBankData.indexTimeAutoSleep] * 10000 - 10000) {
                brightness->lowBrightness();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskWifi(void* p) {
    if (!WiFi.config(localIp, gateway, subnet)) {
        _app_log_e("STA Failed to configure");
    }

    WiFi.begin(ssid, password);
    _app_log_i("Dang ket noi toi WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //_app_log_i(".", "");
    }
    _app_log_i("\n");
    delay(100);
    _app_log_i("Wifi connected!");
    _app_log_i("IP Address: " + WiFi.localIP().toString());
    _app_log_i("RSSI: " + String(WiFi.RSSI()) + " dBm");
    _app_log_i("Gateway: " + WiFi.gatewayIP().toString());
    _app_log_i("Subnet: " + WiFi.subnetMask().toString());
    _app_log_i("Broadcast IP: " + WiFi.softAPBroadcastIP().toString());
    _app_log_i((String)"Hostname: " + WiFi.getHostname());
    _app_log_i("MAC Address: " + WiFi.macAddress());
    _app_log_i("Channel: " + (String)WiFi.channel());
    _app_log_i((String)"Mode: " + (WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP")));
    _app_log_i((String)"Status: " + (WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));

    xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, &taskHandleWebsocket, 0);
    taskHandleWifi = NULL;
    vTaskDelete(NULL);
}

void taskCapacityCalculator(void* p) {
    uint32_t oldTime = millis() - 500;
    float value;
    if (ip5356->getBatteryVoltage() > 2000 && ip5356->getBatteryPercentage() == 0 && ip5356->getBatteryVoltage() < 3200) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (ip5356->getBatteryPercentage() == 0) {
            powerBankData.batCapacity = 0.0f;
        }
    }

    while (true) {
        uint32_t interval = millis() - oldTime;
        oldTime = millis();

        value = (ip5356->getBatteryCurrent() * interval) / 3600000.0f;

        IP5356::CHARGE_STATUS state = ip5356->getChargeStatus();
        if (state == IP5356::CHARGE_STATUS::NOT_CHARGING) {
            powerBankData.batCapacity -= value;
        }
        else if (state == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
            //
        }
        else {
            powerBankData.batCapacity += value;
        }

        powerBankData.batCapacity -= interval * 100.0f / 3600000.0f; // average current consumption of esp32 and ip5356

        if (powerBankData.batCapacity > 10000) powerBankData.batCapacity = 10000;
        else if (powerBankData.batCapacity < 0) powerBankData.batCapacity = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskChargeCycleCalculator(void* p) {
    uint8_t curentPercent = ip5356->getBatteryPercentage();
    while (ip5356->isWork() && ip5356->isConnected() && curentPercent > 100) {
        curentPercent = ip5356->getBatteryPercentage();
    }
    uint8_t oldPercent = curentPercent;

    // File fileLog = LittleFS.open("/logs/charge.log", "a", true);
    // String logStr = "time,current,capacity,voltage,percent,status\n" + String(millis() / 1000) + "," + String(ip5356->getBatteryCurrent(), 0)
    //     + "," + String(powerBankData.batCapacity, 0) + "," + String(ip5356->getBatteryVoltage(), 0) + "," + String(curentPercent) + "," + String((uint8_t)ip5356->getChargeStatus()) + "\n";

    // if (fileLog) {
    //     fileLog.print(logStr);
    //     fileLog.close();
    // }

    while (true) {
        curentPercent = ip5356->getBatteryPercentage();
        if (curentPercent > oldPercent) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            curentPercent = ip5356->getBatteryPercentage();
            if (curentPercent > oldPercent) {

                // fileLog = LittleFS.open("/logs/charge.log", "a", true);
                // String logStr = String(millis() / 1000) + "," + String(ip5356->getBatteryCurrent(), 0) + "," + String(powerBankData.batCapacity, 0) + ","
                //     + String(ip5356->getBatteryVoltage(), 0) + "," + String(curentPercent) + "," + String((uint8_t)ip5356->getChargeStatus()) + "\n";

                // if (fileLog) {
                //     fileLog.print(logStr);
                //     fileLog.close();
                // }

                oldPercent++;
                powerBankData.percentCharged++;
                if (powerBankData.percentCharged >= 100) {
                    powerBankData.percentCharged = 0;
                    powerBankData.chargeCycle++;
                }
            }
        }
        else if (curentPercent < oldPercent) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            curentPercent = ip5356->getBatteryPercentage();
            if (curentPercent < oldPercent) {
                oldPercent--;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void disable_all_watchdogs()
{
    // Task WDT
    esp_task_wdt_deinit();
    esp_task_wdt_delete(NULL);

    // RTC WDT
    rtc_wdt_protect_off();
    rtc_wdt_disable();

// #ifdef ARDUINO
//     disableCore0WDT();
//     disableCore1WDT();
// #endif
}

void goWifiAttack() {
    powerBankData.modeAttack = true;
    savePowerBankData(powerBankData);
    esp_restart();
}

// --- Log Viewer Globals ---
struct LogEntry {
    String msg;
    uint16_t color;
};
static std::deque<LogEntry> g_log_buffer;
static int g_scroll_offset = 0; // 0 = at bottom (newest)
volatile int g_scroll_step_req = 0;
volatile bool g_scroll_bottom_req = false;

// Constants
const int MAX_LOG_LINES = 80;
const int LINE_HEIGHT = 10;
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 80;
const int VISIBLE_LINES = SCREEN_HEIGHT / LINE_HEIGHT;

void taskDisplayAttackLogs(void* p) {
    // Wait for queue to be initialized
    while (!g_log_queue) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    set_log_enabled(true);
    set_log_esp_enabled(false);

    img->setTextSize(1);
    
    // Initial clear
    //img->fillSprite(TFT_BLACK);
    //img->pushSprite(0, 0);

    bool need_redraw = false;

    while (true) {
        // 1. Process Input Requests
        if (g_scroll_bottom_req) {
            if (g_scroll_offset != 0) {
                g_scroll_offset = 0;
                need_redraw = true;
            }
            g_scroll_bottom_req = false;
        }

        if (g_scroll_step_req > 0) {
            g_scroll_offset += g_scroll_step_req;
            g_scroll_step_req = 0;
            
            // Clamp offset
            // Max offset: buffer.size() - VISIBLE_LINES
            // If buffer size < VISIBLE_LINES, max offset is 0.
            int max_offset = 0;
            if (g_log_buffer.size() > VISIBLE_LINES) {
                max_offset = g_log_buffer.size() - VISIBLE_LINES;
            }
            
            if (g_scroll_offset > max_offset) {
                g_scroll_offset = max_offset;
            }
            need_redraw = true;
        }


        // 2. Process Logs
        log_t log;
        // Use timeout to allow input processing even if no logs
        if (xQueueReceive(g_log_queue, &log, pdMS_TO_TICKS(50)) == pdTRUE) {
            String msg = String(log.msg);
            uint16_t color = TFT_WHITE;
            
            switch (log.level) {
                case ESP_LOG_ERROR: 
                    color = TFT_RED;
                    break;
                case ESP_LOG_WARN:  
                    color = TFT_YELLOW;
                    break;
                case ESP_LOG_INFO:  
                    color = TFT_GREEN;
                    break;
                case ESP_LOG_DEBUG: 
                    color = TFT_CYAN;
                    break;
                case ESP_LOG_VERBOSE: 
                    color = TFT_LIGHTGREY;
                    break;
                default: 
                    color = TFT_WHITE; 
                    break;
            }

            // Just push back
            g_log_buffer.push_back({msg, color});

            // Handle Circular Buffer
            if (g_log_buffer.size() > MAX_LOG_LINES) {
                g_log_buffer.pop_front();
                // If we popped front, and we are scrolled up, do not shift offset if we want "Lock to bottom" behavior?
                // Just use same logic as before.
            } else {
                 // Buffer growing
                 if (g_scroll_offset > 0) {
                     g_scroll_offset++;
                 }
            }
            need_redraw = true;
        }

        // 3. Draw
        if (need_redraw) {
            img->fillSprite(TFT_BLACK);
            
            if (!g_log_buffer.empty()) {
                int total_lines = g_log_buffer.size();
                int start_line_idx = total_lines - VISIBLE_LINES - g_scroll_offset;
                
                if (start_line_idx < 0) start_line_idx = 0;
                
                int y = 0;
                for (int i = 0; i < VISIBLE_LINES; i++) {
                     int idx = start_line_idx + i;
                     if (idx < total_lines) {
                         const LogEntry& entry = g_log_buffer[idx];
                         img->setTextColor(entry.color, TFT_BLACK); 
                         img->drawString(entry.msg, 0, y);
                         y += LINE_HEIGHT;
                     }
                }
                
                // 4. Scrollbar
                if (total_lines > VISIBLE_LINES) {
                    int bar_h = (VISIBLE_LINES * SCREEN_HEIGHT) / total_lines;
                    if (bar_h < 4) bar_h = 4;
                    // Position
                    // Top (Index 0) -> Bar Y = 0?
                    // Bottom (Index Total) -> Bar Y = Height.
                    // View Start Index: start_line_idx.
                    int bar_y = (start_line_idx * SCREEN_HEIGHT) / total_lines;
                    
                    img->fillRect(SCREEN_WIDTH - 2, bar_y, 2, bar_h, TFT_WHITE);
                }
            }
            
            img->pushSprite(0, 0);
            need_redraw = false;
        }
    }
}

void attackInit() {
    log_init();
    disable_all_watchdogs();
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    xTaskCreatePinnedToCore(taskDisplayAttackLogs, "DisplayAttackLogs", 4096, NULL, 1, NULL, 1);
    vTaskDelay(pdMS_TO_TICKS(2000));

    wifictl_mgmt_ap_start();
    attack_init();
    webserver_run();
}

void taskButtonTick(void* p) {
    while (true) {
        buttonManager.tick();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


const char*  resetReasonToStr(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_UNKNOWN:    return "Không xác định";
    case ESP_RST_POWERON:    return "Reset do cấp nguồn (Power On)";
    case ESP_RST_EXT:        return "Reset do chân Reset/EN (ngoài) (Không áp dụng cho ESP32)";
    case ESP_RST_SW:         return "Reset do phần mềm (esp_restart)";
    case ESP_RST_PANIC:      return "Reset do panic/exception";
    case ESP_RST_INT_WDT:    return "Reset do interrupt watchdog";
    case ESP_RST_TASK_WDT:   return "Reset do task watchdog";
    case ESP_RST_WDT:        return "Reset do watchdog khác";
    case ESP_RST_DEEPSLEEP:  return "Reset sau khi thoát deep sleep";
    case ESP_RST_BROWNOUT:   return "Reset do Brownout (nguồn sụt áp)";
    case ESP_RST_SDIO:       return "Reset do SDIO";
    default:                 return "Reset lý do khác";
    }
}


void setup() {
    //#############################################################################
    pinMode(TFT_EN_PIN, OUTPUT);
    digitalWrite(TFT_EN_PIN, HIGH);

    tft = new TFT_eSPI;
    tft->init();
    tft->setRotation(1);
    tft->setSwapBytes(true);
    
    //tft->fillScreen(TFT_DARKCYAN);

    Wire.begin(SDA_I2C_PIN, SCL_I2C_PIN, 200000);
    eeprom = new Eeprom24C(&Wire, 2, 0x50);

    eeprom->read(EEPROM_DATA_ADDR, &powerBankData, sizeof(powerBankData));
    if(powerBankData.modeAttack){
        tft->pushImage(0, 0, 160, 80, logoAttack);
    }
    else{
        tft->pushImage(0, 0, 160, 80, luffy);
    }


    brightness = new Brightness(TFT_BL_PIN, 0);
    brightness->setPercent(powerBankData.brightness);
    brightness->on();

    img = new TFT_eSprite(tft);
    img->createSprite(160, 80);
    img->setSwapBytes(true);

    esp_reset_reason_t reason = esp_reset_reason();
    _app_log_i("Reason reset: " + String(resetReasonToStr(reason)));
    if (reason == ESP_RST_POWERON || reason == ESP_RST_PANIC || reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT ||
        reason == ESP_RST_WDT || reason == ESP_RST_WDT || powerBankData.modeUpdateFirmware) {
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);
        if (digitalRead(BUTTON_PIN) == HIGH || powerBankData.modeUpdateFirmware) {
            powerBankData.modeUpdateFirmware = false;
            savePowerBankData(powerBankData);

            _app_log_i("Mode upload firmware start!");
            isUploadFirmware = true;

            OtaUpdate::setDisplay(img);
            xTaskCreatePinnedToCore(taskWifi, "WifiTask", 4096, NULL, 1, &taskHandleWifi, 0);
            button.attachLongPressStart(esp_restart);
            while (true) {
                button.tick();
                delay(10);
            }
        }
    }
    //############################################################################################

    if (powerBankData.modeAttack) {
        powerBankData.modeAttack = false;
        savePowerBankData(powerBankData);
        attackInit();
        buttonManager.setActionCallback(([](Action action) {
            if (action == Action::LONG_PRESS_START) {
                esp_restart();
            } else if (action == Action::CLICK) {
                // Scroll up (history)
                g_scroll_step_req++;
            } else if (action == Action::DOUBLE_CLICK) {
                // Scroll to bottom
                g_scroll_bottom_req = true;
            }
            })
        );
        xTaskCreatePinnedToCore(taskButtonTick, "ButtonTick", 2048, NULL, 1, NULL, 1);
        return;
    }




    ip5356 = new IP5356(&Wire, NTC_DIE_PIN, KEY_PIN, INT_PIN);


    screenManager = new ScreenManager;

    buttonManager.setActionCallback(([](Action action) {
        if (brightness->getMode() == Brightness::Mode::LOW_BRIGHTNESS) {
            brightness->on();
            return;
        }
        screenManager->handleButton(action);
        screenManager->setNeedRedraw(true);

        })
    );

    LittleFS.begin(true);
    //LittleFS.format();

    if (powerBankData.indexTimeAutoSleep >= sleepValueOptions.size()) {
        powerBankData.indexTimeAutoSleep = 2;
    }

    if (powerBankData.onWifi) {
        xTaskCreatePinnedToCore(taskWifi, "WifiTask", 4096, NULL, 1, &taskHandleWifi, 0);
    }


    delay(1000);
    ip5356->init();
    ip5356->reduceChargeCurrentWhenHot(powerBankData.reduceChargeCurrentWhenHot);
    ip5356->setBatteryCapacity(9000); //mAh

    xTaskCreatePinnedToCore(taskDrawScreen, "drawScreen", 4096, NULL, 3, &taskHandleDrawScreen, 1);

    esp_sleep_enable_ext1_wakeup(BITMASK_DEEPSLEEP, ESP_EXT1_WAKEUP_ANY_HIGH);
    xTaskCreatePinnedToCore(taskSleep, "SleepTask", 2048, NULL, 2, &taskHandleSleep, 1);

    xTaskCreatePinnedToCore(taskCapacityCalculator, "calCapacity", 2048, NULL, 5, &taskHandleCapacityCalculator, 1);
    delay(1000);
    xTaskCreatePinnedToCore(taskChargeCycleCalculator, "ChargeCycleCalculator", 4096, NULL, 4, &taskHandleChargeCycleCalculator, 1);



}

void loop() {



    vTaskDelete(NULL);




    // if (!ip5356->isConnected()) {
    //     _app_log_e("IP5356 not connected (maybe sleep)");
    //     delay(5000);
    //     return;
    // }

    // uint32_t time = millis();

    // float per = ip5356->getBatteryPercentage();
    // float vbat = ip5356->getBatteryVoltage();
    // float ibat = ip5356->getBatteryCurrent();
    // float ntc = ip5356->getBatteryTemperature();
    // float vsys = ip5356->getSystemVoltage();
    // float isys = ip5356->getSystemCurrent();
    // float psys = ip5356->getSystemPower();
    // float vout1 = ip5356->getUSB1Voltage();
    // float iout1 = ip5356->getUSB1Current();
    // float vout2 = ip5356->getUSB2Voltage();
    // float iout2 = ip5356->getUSB2Current();
    // float vbus = ip5356->getVBUSVoltage();
    // float ibus = ip5356->getVBUSCurrent();
    // uint8_t vbusprotocol = static_cast<uint8_t>(ip5356->getVBUSSourceFrotocol());
    // uint8_t vbusPort = static_cast<uint8_t>(ip5356->getVBUSPortStatus());
    // uint8_t vout2protocol = static_cast<uint8_t>(ip5356->getUSB2SourceFrotocol());
    // uint8_t vout1protocol = static_cast<uint8_t>(ip5356->getUSB1SourceFrotocol());
    // uint8_t chargeStatus = static_cast<uint8_t>(ip5356->getChargeStatus());
    // // float vin = ip5356->getVINVoltage();
    // // float iin = ip5356->getVINCurrent();
    // time = millis() - time;
    // _app_log_i("per: " + String(per));
    // _app_log_i("vbat: " + String(vbat));
    // _app_log_i("ibat: " + String(ibat));
    // _app_log_i("ntc: " + String(ntc));
    // _app_log_i("vsys: " + String(vsys));
    // _app_log_i("isys: " + String(isys));
    // _app_log_i("psys: " + String(psys));
    // _app_log_i("vout1: " + String(vout1));
    // _app_log_i("iout1: " + String(iout1));
    // _app_log_i("vout2: " + String(vout2));
    // _app_log_i("iout2: " + String(iout2));
    // _app_log_i("vbus: " + String(vbus));
    // _app_log_i("ibus: " + String(ibus));
    // _app_log_i("vbusprotocol: " + String(vbusprotocol));
    // _app_log_i("vbusPort: " + String(vbusPort));
    // _app_log_i("vout2protocol: " + String(vout2protocol));
    // _app_log_i("vout1protocol: " + String(vout1protocol));
    // _app_log_i("chargeStatus: " + String(chargeStatus));
    // // _app_log_i("vin: " + String(vin));
    // // _app_log_i("iin: " + String(iin));

    // _app_log_i("time: " + String(time) + "ms");
    // //_app_log_i("cpuCore: " + String(xPortGetCoreID())); //1

    // // String str = "IP5356_0X74_REG:\n";
    // // for (uint8_t reg : IP5356_0X74_REG_ARR) {
    // //     str+=loadDataIP5356(0x74, reg);
    // // }
    // // _app_log_i(str);



    // // str = "IP5356_0X75_REG:\n";
    // // for (uint8_t reg : IP5356_0X75_REG_ARR) {
    // //     str += loadDataIP5356(0x75, reg);
    // // }
    // // _app_log_i(str);
}

extern "C" bool verifyRollbackLater() {
    return true;
}


String getIpGateway() {
    IPAddress _gateway = WiFi.gatewayIP();
    String gatewayStr = String(_gateway[0]) + "." +
        String(_gateway[1]) + "." +
        String(_gateway[2]) + "." +
        String(_gateway[3]);

    return gatewayStr;
}

String timeConnectWifiFree() {
    String res_str = "?";
    HTTPClient status;
    status.begin("http://" + getIpGateway() + "/status");
    int res = status.GET();
    if (res > 0) {
        res_str = status.getString();
        int idx_str = res_str.indexOf("left");
        if (idx_str != -1) {
            res_str = res_str.substring(idx_str + 13, res_str.indexOf("/", idx_str + 13));
        }
    }

    return res_str;
}

bool app_login() {
    HTTPClient app_login;
    app_login.begin("http://" + getIpGateway() + "/app_login");
    String body = "username=awing15-15&password=Awing15-15@2023";
    int res = app_login.POST(body);
    _app_log_i("Login: " + String(res));
    app_login.end();
    return res > 0 && res < 400;
}

bool app_logout() {
    HTTPClient app_logout;
    app_logout.begin("http://" + getIpGateway() + "/app_logout?");
    int res = app_logout.GET();
    _app_log_i("Logout: " + String(res));
    app_logout.end();
    return res > 0 && res < 400;
}

void app_loginWifiFree(void* p) {
    while (true) {
        if (app_logout()) {
            vTaskDelay(500);
            if (app_login()) {
                vTaskDelay(600000);
            }
        }
        vTaskDelay(5000);
    }
}

String addPaddingBin8(uint8_t value) {
    String str = String(value) + " | ";
    String bin = String(value, BIN);
    String padding = "";
    for (uint8_t i = 0; i < 8 - bin.length();i++) {
        padding = "0" + padding;
    }
    str += padding + bin;
    return str;
}



String formatTimestamp(int64_t timestamp, int tzOffsetHours = 7) {
    time_t local_ts = (time_t)(timestamp + tzOffsetHours * 3600);
    struct tm t;
    gmtime_r(&local_ts, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return String(buf);
}


void printInfomation() {
    String info = "Information\n";
#ifdef BUILD_TIME
    info += "BUILD_TIME: " + formatTimestamp(BUILD_TIME) + "\n";
#endif
    esp_reset_reason_t reason = esp_reset_reason();
    info += "Reason reset: " + String(resetReasonToStr(reason)) + "\n";
    info += "Size LittleFS: " + String(LittleFS.totalBytes()) + "\n";
    info += "Used LittelFS: " + String(LittleFS.usedBytes()) + "\n";
    info += "IP Address: " + WiFi.localIP().toString() + "\n";
    info += "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    info += "Gateway: " + WiFi.gatewayIP().toString() + "\n";
    info += ("Subnet: " + WiFi.subnetMask().toString()) + "\n";
    info += ("Broadcast IP: " + WiFi.softAPBroadcastIP().toString()) + "\n";
    info += ((String)"Hostname: " + WiFi.getHostname()) + "\n";
    info += ("MAC Address: " + WiFi.macAddress()) + "\n";
    info += ("Channel: " + (String)WiFi.channel()) + "\n";
    info += ((String)"Mode: " + (WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP"))) + "\n";
    info += ((String)"Status: " + (WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected")) + "\n";
    _app_log_i(info);
}

void listDirRecursive(const char* dirname) {
    File root = LittleFS.open(dirname);
    if (!root) {
        _app_log_e("Failed to open dir: " + String(dirname));
        return;
    }
    if (!root.isDirectory()) {
        _app_log_e(String(dirname) + "is not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            _app_log_i("DIR : " + String(file.name()));
            listDirRecursive(file.name());
        }
        else {
            _app_log_i("FILE: " + String(file.name()) + "SIZE: " + String(file.size()));
        }
        file = root.openNextFile();
    }
}

void handleMessage(uint8_t num, uint8_t* payload) {
    JsonDocument messageJon;
    DeserializationError error = deserializeJson(messageJon, payload);
    if (error) {
        _app_log_e("Deserialization failed: " + String(error.c_str()));
        return;
    }

    if (!messageJon["key"].is<const char*>()) {
        _app_log_e("Not have 'key' in message");
        return;
    }
    String key = messageJon["key"].as<String>();
    _app_log_i("Received key: " + key);

    if (key == KEY_UPLOAD_FIRMWARE) {
        if (isUploadFirmware == false) {
            _app_log_e("Not in upload firmware mode");
            websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareFailed\", \"error\":\"Not in upload firmware mode\"}");
            return;
        }
        OtaUpdate::handleCommand(websocket, num, messageJon);
    }
    else if (key == "initSerialMonitor") {
        mainLog.setLogModeWebSockets(&websocket, num);
        _app_log_i("Serial monitor client connected: " + String(num));

        isConnectedWebsocket = true;
        printInfomation();
    }
    else if (key == "restart") {
        _app_log_i("Restarting...");
        vTaskDelay(1000);
        websocket.disconnect();
        websocket.close();
        ESP.restart();
    }
    else if (key == "readIP5356") {
        if (!messageJon["addr"].is<uint32_t>()) {
            _app_log_e("Not have \"addr\" in json");
            return;
        }
        if (!messageJon["reg"].is<uint32_t>()) {
            _app_log_e("Not have \"reg\" in json");
            return;
        }
        uint32_t addr = messageJon["addr"].as<uint32_t>();
        if (addr != 0x74 && addr != 0x75) {
            _app_log_e("addr incorrect");
            return;
        }
        uint32_t reg = messageJon["reg"].as<uint32_t>();
        if (reg > 0xFF) {
            _app_log_e("reg out of range(uint8_t)");
            return;
        }
        uint8_t value = 0;
        if (ip5356->_read(addr, reg, &value)) {
            _app_log_i("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: " + addPaddingBin8(value));
        }
        else {
            _app_log_e("Can not read ip5356 (maybe sleep)");
        }
    }
    else if (key == "writeIP5356") {
        if (!messageJon["addr"].is<uint32_t>()) {
            _app_log_e("Not have \"addr\" in json");
            return;
        }
        if (!messageJon["reg"].is<uint32_t>()) {
            _app_log_e("Not have \"reg\" in json");
            return;
        }
        if (!messageJon["value"].is<uint32_t>()) {
            _app_log_e("Not have \"value\" in json");
            return;
        }
        uint32_t addr = messageJon["addr"].as<uint32_t>();
        if (addr != 0x74 && addr != 0x75) {
            _app_log_e("addr incorrect");
            return;
        }
        uint32_t reg = messageJon["reg"].as<uint32_t>();
        if (reg > 0xFF) {
            _app_log_e("reg out of range(uint8_t)");
            return;
        }
        uint32_t value = messageJon["value"].as<uint32_t>();
        if (value > 255) {
            _app_log_e("value out of range(uint8_t)");
            return;
        }
        uint8_t temp = value;
        if (!ip5356->_write(addr, reg, &temp)) {
            _app_log_e("Can not read ip5356 (maybe sleep)");
        }
    }
    else if (key == "readFileFromLittleFS") {
        if (!messageJon["name"].is<const char*>()) {
            _app_log_e("Not have \"name\" in json");
            return;
        }
        String name = messageJon["name"].as<const char*>();
        File file = LittleFS.open(name, "r");
        if (!file) {
            _app_log_e("Failed to open file for writing");
            websocket.sendTXT(num, "{\"key\":\"openFileFailed\"}");
            return;
        }
        websocket.sendTXT(num, "{\"key\":\"openFileSuccess\"}");
        uint32_t readByte = 0, read;
        uint8_t buffer[1024];
        while (readByte < file.size()) {
            read = file.read(buffer, 1024);
            websocket.sendBIN(num, buffer, read);
            readByte += read;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        websocket.sendTXT(num, "{\"key\":\"readFileSuccess\"}");
    }
    else if (key == "writeFileToLittleFS") {
        if (!messageJon["name"].is<const char*>()) {
            _app_log_e("Not have \"name\" in json");
            return;
        }
        if (g_uploadInProgress) {
            _app_log_e("Upload already in progress");
            websocket.sendTXT(num, "{\"key\":\"openFileFailed\"}");
            return;
        }
        String name = messageJon["name"].as<const char*>();
        g_uploadExpectedSize = messageJon["size"].is<uint32_t>() ? messageJon["size"].as<uint32_t>() : 0;
        g_uploadFile = LittleFS.open(name, "w", true);
        if (!g_uploadFile) {
            _app_log_e("Failed to open file for writing: " + name);
            websocket.sendTXT(num, "{\"key\":\"openFileFailed\"}");
            return;
        }
        g_uploadInProgress = true;
        g_uploadFailed = false;
        g_uploadWritten = 0;
        websocket.sendTXT(num, "{\"key\":\"openFileSuccess\"}");
    }
    else if (key == "writeFileToLittleFSEnd") {
        if (!g_uploadInProgress) {
            _app_log_e("No upload in progress");
            websocket.sendTXT(num, "{\"key\":\"writeFileFailed\"}");
            return;
        }
        if (g_uploadFile) g_uploadFile.close();
        bool ok = !g_uploadFailed && (g_uploadExpectedSize == 0 || g_uploadWritten == g_uploadExpectedSize);
        g_uploadInProgress = false;
        if (ok) {
            websocket.sendTXT(num, "{\"key\":\"writeFileSuccess\"}");
        }
        else {
            websocket.sendTXT(num, "{\"key\":\"writeFileFailed\"}");
        }
    }
    else if (key == "loadAllRegDataIP5356") {
        String str = "IP5356\n";
        str += ip5356->loadAllRegData();
        _app_log_i(str);
    }
    else if (key == "setBatCapacity") {
        if (!messageJon["value"].is<uint32_t>()) {
            _app_log_e("Not have \"value\" in json");
            return;
        }
        float value = messageJon["value"].as<float>();
        powerBankData.batCapacity = value;
        _app_log_i("batCapacity: " + String(powerBankData.batCapacity));
    }
    else if (key == "dir") {
        listDirRecursive("/");
    }
    else if (key == "help") {
        String help =
            "Available commands:\n"
            "1. uploadFirmware: Start firmware upload process.\n"
            "2. initSerialMonitor: Initialize serial monitor over WebSocket.\n"
            "3. restart: Restart the device.\n"
            "4. readIP5356: Read a register from ip5356-> Requires 'addr' and 'reg'.\n"
            "5. writeIP5356: Write a value to a register in ip5356-> Requires 'addr', 'reg', and 'value'.\n"
            "6. readFileFromLittleFS: Read a file from LittleFS. Requires 'name'.\n"
            "8. loadAllRegDataIP5356: Load and display all register data from ip5356->\n"
            "7. help: Display this help message.\n";
        _app_log_i(help);
    }
    else {
        _app_log_e("Unknown key: " + key);
    }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_TEXT:
        _app_log_i("[" + String(num) + "] Received text message: " + String((char*)payload));
        handleMessage(num, payload);
        break;
    case WStype_BIN:
        if (OtaUpdate::isRunning()) {
            OtaUpdate::write(payload, length);
        }
        else if (g_uploadInProgress && g_uploadFile) {
            size_t written = g_uploadFile.write(payload, length);
            if (written != length) {
                g_uploadFailed = true;
            }
            else {
                g_uploadWritten += written;
            }
        }
        break;
    case WStype_CONNECTED:
        _app_log_i("[" + String(num) + "] Connected!");
        break;
    case WStype_DISCONNECTED:
        _app_log_i("[" + String(num) + "] Disconnected!");
        if (mainLog.getLogMode() != LoggerManager::MODE_WEBSOCKETS) {
            break;
        }
        if (((WebSocketLogger*)mainLog.getLogger())->getIdClientWebSockets() == num) {
            mainLog.setLogModeSerial();
            _app_log_i("Serial monitor client disconnected");
        }
        break;

    }
}

void webSocketTask(void* p) {
    websocket.begin();
    websocket.onEvent(webSocketEvent);
    _app_log_i("WebSocket task is running");
    while (true) {
        websocket.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}