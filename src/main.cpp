#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include "OtaUpdate.h"
#include "luffy.h"
#include "Manager.h"
#include "esp_ota_ops.h"
#include "Globals.h"



const char* ssid = "DESKTOP-P5540";
const char* password = "aaaaaaaa";

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



void webSocketTask(void* p);
void app_loginWifiFree(void* p);
String timeConnectWifiFree();

void taskDrawScreen(void* p) {
    vTaskDelay(pdMS_TO_TICKS(500));
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

void sleep() {
    brightness->off();
    ip5356->powerOff();
    EepromData eepromData = { brightness->getPercent(), chargeCycle, percentCharged, batCapacity };
    eeprom->write(EEPROM_DATA_ADDR, &eepromData, sizeof(eepromData));
    delay(500);
    app_log("DeepSleep started");
    esp_deep_sleep_start();
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

        uint32_t idleTime = buttonManager.getIdleTime();
        if (idleTime > 30000) {
            if (isIsysLow) {
                if (millis() - timeIsysLowStart > 30000) {
                    if (!OtaUpdate::isRunning()) {
                        sleep();
                    }
                }
            }
            else if (ip5356->getChargeStatus() == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (ip5356->getChargeStatus() == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
                    esp_sleep_enable_ext1_wakeup(BITMASK_DEEPSLEEP, ESP_EXT1_WAKEUP_ALL_LOW);
                    ip5356->isysLowAutoOff(true);
                    ip5356->set8sAutoOff();
                    sleep();
                }
            }
            brightness->lowBrightness();
        }
        else if (idleTime > 20000) {
            brightness->lowBrightness();
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskWifi(void* p) {
    if (!WiFi.config(localIp, gateway, subnet)) {
        app_log("STA Failed to configure");
    }

    WiFi.begin(ssid, password);
    app_log("Dang ket noi toi WiFi", "");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        app_log(".", "");
    }
    app_log("");
    delay(500);
    app_log("Wifi connected!");
    app_log_i("IP Address: " + WiFi.localIP().toString());
    app_log_i("RSSI: " + String(WiFi.RSSI()) + " dBm");
    app_log_i("Gateway: " + WiFi.gatewayIP().toString());
    app_log_i("Subnet: " + WiFi.subnetMask().toString());
    app_log_i("Broadcast IP: " + WiFi.softAPBroadcastIP().toString());
    app_log_i((String)"Hostname: " + WiFi.getHostname());
    app_log_i("MAC Address: " + WiFi.macAddress());
    app_log_i("Channel: " + (String)WiFi.channel());
    app_log_i((String)"Mode: " + (WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP")));
    app_log_i((String)"Status: " + (WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
    xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, NULL, 1);
    vTaskDelete(NULL);
}

const char* resetReasonToStr(esp_reset_reason_t reason) {
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


void taskCapacityCalculator(void* p) {
    uint32_t oldTime = millis();
    float value;

    while (true) {
        uint32_t interval = millis() - oldTime;
        oldTime = millis();

        value = (ip5356->getBatteryCurrent() * interval) / 3600000.0f;

        IP5356::CHARGE_STATUS state = ip5356->getChargeStatus();
        if (state == IP5356::CHARGE_STATUS::NOT_CHARGING) {
            batCapacity -= value;
        }
        else if (state == IP5356::CHARGE_STATUS::FULLY_CHARGED) {
            //
        }
        else {
            batCapacity += value;
        }

        batCapacity -= interval * 100.0f / 3600000.0f; // average current consumption of esp32 and ip5356

        // if (batCapacity > 10000) batCapacity = 10000;
        // else if (batCapacity < 0) batCapacity = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskChargeCycleCalculator(void* p) {
    uint8_t curentPercent = ip5356->getBatteryPercentage();
    while (curentPercent > 100) {
        curentPercent = ip5356->getBatteryPercentage();
    }
    uint8_t oldPercent = curentPercent;
    while (true) {
        curentPercent = ip5356->getBatteryPercentage();
        if (curentPercent > oldPercent) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            curentPercent = ip5356->getBatteryPercentage();
            if (curentPercent > oldPercent) {
                oldPercent++;
                percentCharged++;
                if (percentCharged == 100) {
                    percentCharged = 0;
                    chargeCycle++;
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


void setup() {
    //#############################################################################
    pinMode(TFT_EN_PIN, OUTPUT);
    digitalWrite(TFT_EN_PIN, HIGH);

    tft = new TFT_eSPI;
    tft->init();
    tft->setRotation(1);
    tft->setSwapBytes(true);
    tft->pushImage(0, 0, 160, 80, luffy);
    //tft->fillScreen(TFT_DARKCYAN);

    Wire.begin(SDA_I2C_PIN, SCL_I2C_PIN, 200000);
    eeprom = new Eeprom24C(&Wire, 2, 0x50);

    EepromData eepromData;
    eeprom->read(EEPROM_DATA_ADDR, &eepromData, sizeof(eepromData));

    brightness = new Brightness(TFT_BL_PIN, 0);
    brightness->setPercent(eepromData.brightness);
    brightness->on();

    img = new TFT_eSprite(tft);
    img->createSprite(160, 80);
    img->setSwapBytes(true);

    esp_reset_reason_t reason = esp_reset_reason();
    app_log("Reason reset: " + String(resetReasonToStr(reason)));
    if (reason == ESP_RST_POWERON || 1) {
        pinMode(BUTTON_PIN, INPUT_PULLDOWN);
        if (digitalRead(BUTTON_PIN) == HIGH) {
            app_log("Mode upload firmware start!");
            OtaUpdate::setDisplay(img);
            xTaskCreatePinnedToCore(taskWifi, "WifiTask", 4096, NULL, 1, NULL, 1);
            button.attachLongPressStart(esp_restart);
            while (true) {
                button.tick();
                delay(10);
            }
        }
    }
    //############################################################################################


    ip5356 = new IP5356(&Wire, NTC_DIE_PIN, KEY_PIN, INT_PIN);


    screenManager = new ScreenManager;

    buttonManager.setActionCallback(([&](Action action) {
        if (brightness->getMode() == Brightness::Mode::LOW_BRIGHTNESS) {
            brightness->on();
            return;
        }
        screenManager->handleButton(action);
        screenManager->setNeedRedraw(true);

        }
        ));

    LittleFS.begin();

    batCapacity = eepromData.batCapacity;
    percentCharged = eepromData.percentCharged;
    chargeCycle = eepromData.chargeCycle;

    xTaskCreatePinnedToCore(taskDrawScreen, "drawScreen", 4096, nullptr, 2, nullptr, 0);

    esp_sleep_enable_ext1_wakeup(BITMASK_DEEPSLEEP, ESP_EXT1_WAKEUP_ANY_HIGH);
    xTaskCreatePinnedToCore(taskSleep, "SleepTask", 2048, NULL, 1, NULL, 1);

    xTaskCreatePinnedToCore(taskWifi, "WifiTask", 4096, NULL, 1, NULL, 1);

    delay(2000);
    ip5356->init();

    xTaskCreatePinnedToCore(taskChargeCycleCalculator, "ChargeCycleCalculator", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskCapacityCalculator, "calCapacity", 2048, NULL, 5, NULL, 0);
}

void loop() {



    vTaskDelete(NULL);




    // if (!ip5356->isConnected()) {
    //     app_log_e("IP5356 not connected (maybe sleep)");
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
    // app_log("per: " + String(per));
    // app_log("vbat: " + String(vbat));
    // app_log("ibat: " + String(ibat));
    // app_log("ntc: " + String(ntc));
    // app_log("vsys: " + String(vsys));
    // app_log("isys: " + String(isys));
    // app_log("psys: " + String(psys));
    // app_log("vout1: " + String(vout1));
    // app_log("iout1: " + String(iout1));
    // app_log("vout2: " + String(vout2));
    // app_log("iout2: " + String(iout2));
    // app_log("vbus: " + String(vbus));
    // app_log("ibus: " + String(ibus));
    // app_log("vbusprotocol: " + String(vbusprotocol));
    // app_log("vbusPort: " + String(vbusPort));
    // app_log("vout2protocol: " + String(vout2protocol));
    // app_log("vout1protocol: " + String(vout1protocol));
    // app_log("chargeStatus: " + String(chargeStatus));
    // // app_log("vin: " + String(vin));
    // // app_log("iin: " + String(iin));

    // app_log("time: " + String(time) + "ms");
    // //app_log("cpuCore: " + String(xPortGetCoreID())); //1

    // // String str = "IP5356_0X74_REG:\n";
    // // for (uint8_t reg : IP5356_0X74_REG_ARR) {
    // //     str+=loadDataIP5356(0x74, reg);
    // // }
    // // app_log(str);



    // // str = "IP5356_0X75_REG:\n";
    // // for (uint8_t reg : IP5356_0X75_REG_ARR) {
    // //     str += loadDataIP5356(0x75, reg);
    // // }
    // // app_log(str);
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
    app_log("Login: " + String(res));
    app_login.end();
    return res > 0 && res < 400;
}

bool app_logout() {
    HTTPClient app_logout;
    app_logout.begin("http://" + getIpGateway() + "/app_logout?");
    int res = app_logout.GET();
    app_log("Logout: " + String(res));
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
    info += "BUILD_TIME: " + formatTimestamp(BUILD_TIME) + "\n";
    esp_reset_reason_t reason = esp_reset_reason();
    info += "Reason reset: " + String(resetReasonToStr(reason)) + "\n";
    info += "Size LittleFS: " + String(LittleFS.totalBytes()) + "\n";
    info += "Used LittelFS: " + String(LittleFS.usedBytes()) + "\n";

    app_log_i(info);
}

void handleMessage(uint8_t num, uint8_t* payload) {
    JsonDocument messageJon;
    DeserializationError error = deserializeJson(messageJon, payload);
    if (error) {
        app_log_e("Deserialization failed: " + String(error.c_str()));
        return;
    }

    if (!messageJon["key"].is<const char*>()) {
        app_log_e("Not have 'key' in message");
        return;
    }
    String key = messageJon["key"].as<String>();
    app_log("Received key: " + key);

    if (key == KEY_UPLOAD_FIRMWARE) {
        OtaUpdate::handleCommand(websocket, num, messageJon);
    }
    else if (key == "initSerialMonitor") {
        mainLog.setLogModeWebSockets(&websocket, num);
        app_log("Serial monitor client connected: " + String(num));

        const esp_partition_t* running = esp_ota_get_running_partition();
        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                esp_ota_mark_app_valid_cancel_rollback();
                app_log("OTA marked as valid.");
            }
        }
        printInfomation();
    }
    else if (key == "restart") {
        app_log_i("Restarting...");
        vTaskDelay(1000);
        websocket.disconnect();
        websocket.close();
        ESP.restart();
    }
    else if (key == "readIP5356") {
        if (!messageJon["addr"].is<uint32_t>()) {
            app_log_e("Not have \"addr\" in json");
            return;
        }
        if (!messageJon["reg"].is<uint32_t>()) {
            app_log_e("Not have \"reg\" in json");
            return;
        }
        uint32_t addr = messageJon["addr"].as<uint32_t>();
        if (addr != 0x74 && addr != 0x75) {
            app_log_e("addr incorrect");
            return;
        }
        uint32_t reg = messageJon["reg"].as<uint32_t>();
        if (reg > 0xFF) {
            app_log_e("reg out of range(uint8_t)");
            return;
        }
        uint8_t value = 0;
        if (ip5356->_read(addr, reg, &value)) {
            app_log_i("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: " + addPaddingBin8(value));
        }
        else {
            app_log_e("Can not read ip5356 (maybe sleep)");
        }
    }
    else if (key == "writeIP5356") {
        if (!messageJon["addr"].is<uint32_t>()) {
            app_log_e("Not have \"addr\" in json");
            return;
        }
        if (!messageJon["reg"].is<uint32_t>()) {
            app_log_e("Not have \"reg\" in json");
            return;
        }
        if (!messageJon["value"].is<uint32_t>()) {
            app_log_e("Not have \"value\" in json");
            return;
        }
        uint32_t addr = messageJon["addr"].as<uint32_t>();
        if (addr != 0x74 && addr != 0x75) {
            app_log_e("addr incorrect");
            return;
        }
        uint32_t reg = messageJon["reg"].as<uint32_t>();
        if (reg > 0xFF) {
            app_log_e("reg out of range(uint8_t)");
            return;
        }
        uint32_t value = messageJon["value"].as<uint32_t>();
        if (value > 255) {
            app_log_e("value out of range(uint8_t)");
            return;
        }
        uint8_t temp = value;
        if (!ip5356->_write(addr, reg, &temp)) {
            app_log_e("Can not read ip5356 (maybe sleep)");
        }
    }
    else if (key == "readFileFromLittleFS") {
        if (!messageJon["name"].is<const char*>()) {
            app_log_e("Not have \"name\" in json");
            return;
        }
        String name = messageJon["name"].as<const char*>();
        File file = LittleFS.open(name, "r");
        if (!file) {
            app_log("Failed to open file for writing");
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
        }
        websocket.sendTXT(num, "{\"key\":\"readFileSuccess\"}");
    }
    else if (key == "loadAllRegDataIP5356") {
        String str = "IP5356\n";
        str += ip5356->loadAllRegData();
        app_log(str);
    }
    else if (key == "setBatCapacity") {
        if (!messageJon["value"].is<uint32_t>()) {
            app_log_e("Not have \"value\" in json");
            return;
        }
        float value = messageJon["value"].as<float>();
        batCapacity = value;
        app_log("batCapacity: " + String(batCapacity));
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
        app_log(help);
    }
    else {
        app_log_e("Unknown key: " + key);
    }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_TEXT:
        app_log_i("[" + String(num) + "] Received text message: " + String((char*)payload));
        handleMessage(num, payload);
        break;
    case WStype_BIN:
        if (OtaUpdate::isRunning()) {
            OtaUpdate::write(payload, length);
        }
        break;
    case WStype_CONNECTED:
        app_log_i("[" + String(num) + "] Connected!");
        break;
    case WStype_DISCONNECTED:
        app_log_i("[" + String(num) + "] Disconnected!");
        if (mainLog.getLogMode() != LoggerManager::MODE_WEBSOCKETS) {
            break;
        }
        if (((WebSocketLogger*)mainLog.getLogger())->getIdClientWebSockets() == num) {
            mainLog.setLogModeSerial();
            app_log("Serial monitor client disconnected");
        }
        break;

    }
}

void webSocketTask(void* p) {
    websocket.begin();
    websocket.onEvent(webSocketEvent);
    app_log("WebSocket task is running");
    while (true) {
        websocket.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}