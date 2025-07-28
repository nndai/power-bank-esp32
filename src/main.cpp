#include <WiFi.h>
#include <Update.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <HTTPClient.h>
#include <IP5356.h>
#include <Log.h>
#include "OtaUpdate.h"

const char* ssid = "DESKTOP-P5540";
const char* password = "aaaaaaaa";

IPAddress localIp(192, 168, 137, 194);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
WebSocketsServer websocket(82);

IP5356* ip5356 = IP5356::getInstance();

uint8_t clientSerialMonitor = 255;

void webSocketTask(void* p);
void app_loginWifiFree(void* p);
String timeConnectWifiFree();

void setup() {

    ip5356->begin();

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


    xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, NULL, 0);


    delay(2000);
    app_log("Wifi connected!");
    app_log_i("IP Address: ", "");
    app_log_i(WiFi.localIP().toString().c_str());
    app_log_i("RSSI: ", "");
    app_log_i(String(WiFi.RSSI()) + " dBm");
    app_log_i("Gateway: ", "");
    app_log_i(WiFi.gatewayIP().toString().c_str());
    app_log_i("Subnet: ", "");
    app_log_i(WiFi.subnetMask().toString().c_str());
    app_log_i("Broadcast IP: ", "");
    app_log_i(WiFi.softAPBroadcastIP().toString().c_str());
    app_log_i("Hostname: ", "");
    app_log_i(WiFi.getHostname());
    app_log_i("MAC Address: ", "");
    app_log_i(WiFi.macAddress().c_str());
    app_log_i("Channel: ", "");
    app_log_i(WiFi.channel());
    app_log_i("Mode: ", "");
    app_log_i(WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP"));
    app_log_i("Status: ", "");
    app_log_i(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");


    // while (digitalRead(5) == LOW) {
    //     delay(50);
    //     app_log("Waiting...");
    // }
    //xTaskCreatePinnedToCore(app_loginWifiFree, "LoginWifiFree", 8192, NULL, 3, NULL, 0);

}

void scanI2C() {
    app_log("Scanning I2C bus...");
    byte count = 0;
    for (uint8_t address = 1; address < 128; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            app_log("Found I2C device at address 0x" + String(address, HEX));
            count++;
            delay(10);
        }
    }
    if (count == 0) {
        app_log("No I2C devices found.");
    }
    else {
        app_log("Scan done. Found " + String(count) + " device(s).");
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


String loadDataIP5356(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission((uint8_t)addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)1);
    String str = "0x";
    if (reg < 16) {
        str += "0";
    }

    str += String(reg, HEX) + ": ";
    if (Wire.available()) {
        uint8_t value = Wire.read();
        if (value < 10) {
            str += "  ";
        }
        else if (value < 100) {
            str += " ";
        }

        str += String(value) + " | ";// + String(value, BIN);
        String bin = String(value, BIN);
        String padding = "";
        for (uint8_t i = 0; i < 8 - bin.length();i++) {
            padding = "0" + padding;
        }
        str += padding + bin;
    }
    str += "\n";
    return str;
}

void loop() {

    // // app_log(String(anaapp_logRead(35))+"", "\r");

    // // delay(100);
    //scanI2C();

    // for (int i = 0; i < 10; i++) {
    //     //scanI2C();
    //     // uint8_t ip5356_status = ip5356.readRegister(0x7b);
    //     // String statusStr = "IP5356 Status: " + String(ip5356_status);
    //     // app_log(statusStr);

    //     Wire.beginTransmission(0x75);
    //     if (Wire.endTransmission() == 0) {
    //         app_log("Found: " + String(0x75, HEX));
    //     }
    //     else {
    //         app_log("No");
    //     }

    //     digitalWrite(21, HIGH);
    //     // Wire.beginTransmission(0x75); // IP5356 default address
    //     // if (Wire.endTransmission() == 0) {
    //     //     app_log("IP5356 is connected.");
    //     // }
    //     // else {
    //     //     app_log("IP5356 is not connected.");
    //     // }
    //     vTaskDelay(500);
    // }

    // // app_log("21 value: " + String(digitalRead(21)));
    // // app_log("22 value: " + String(digitalRead(22)));

    // for(int i = 0; i < 5; i++) {
    //     app_log("Loop iteration: " + String(i+1));
    vTaskDelay(3000);
    // }

    // while (mainLog.getLogMode() != LoggerManager::MODE_WEBSOCKETS) {
    //     delay(1000);
    // }
    uint32_t time = millis();
    app_log("per: " + String(ip5356->getBatteryPercentage()));
    app_log("vbat: " + String(ip5356->getBatteryVoltage()));
    app_log("ibat: " + String(ip5356->getBatteryCurrent()));
    app_log("ntc: " + String(ip5356->getBatteryTemperature()));
    app_log("vsys: " + String(ip5356->getSystemVoltage()));
    app_log("isys: " + String(ip5356->getSystemCurrent()));
    app_log("psys: " + String(ip5356->getSystemPower()));
    app_log("vout1: " + String(ip5356->getUSB1Voltage()));
    app_log("iout1: " + String(ip5356->getUSB1Current()));
    app_log("vout2: " + String(ip5356->getUSB2Voltage()));
    app_log("iout2: " + String(ip5356->getUSB2Current()));
    app_log("vbus: " + String(ip5356->getVBUSVoltage()));
    app_log("ibus: " + String(ip5356->getVBUSCurrent()));
    app_log("vbusprotocol: " + String(static_cast<uint8_t>(ip5356->getVBUSSourceFrotocol())));
    app_log("vbusPort: " + String(static_cast<uint8_t>(ip5356->getVBUSPortStatus())));
    app_log("vout2protocol: " + String(static_cast<uint8_t>(ip5356->getUSB2SourceFrotocol())));
    app_log("vout1protocol: " + String(static_cast<uint8_t>(ip5356->getUSB1SourceFrotocol())));
    app_log("chargeStatus: " + String(static_cast<uint8_t>(ip5356->getChargeStatus())));
    // app_log("vin: " + String(ip5356->getVINVoltage()));
     //app_log("iin: " + String(ip5356->getVINCurrent()));

    app_log("time: " + String(millis() - time) + "ms");
    app_log("sizeof IP5356: " + String(sizeof(IP5356)));
    app_log("sizeof websocket: " + String(sizeof(websocket)));

    // String str = "IP5356_0X74_REG:\n";
    // for (uint8_t reg : IP5356_0X74_REG_ARR) {
    //     str+=loadDataIP5356(0x74, reg);
    // }
    // app_log(str);



    // str = "IP5356_0X75_REG:\n";
    // for (uint8_t reg : IP5356_0X75_REG_ARR) {
    //     str += loadDataIP5356(0x75, reg);
    // }
    // app_log(str);
}



// void app_log(String& msg, char end_char) {

//     if (clientSerialMonitor != 255) {
//         websocket.sendTXT(clientSerialMonitor, msg + end_char);
//     }
//     else {
//         app_log(msg + String(end_char));
//     }
// }

// void app_log(const char* msg, char end_char) {
//     if (clientSerialMonitor != 255) {
//         websocket.sendTXT(clientSerialMonitor, msg + String(end_char));
//     }
//     else {
//         app_log(msg + String(end_char));
//     }
// }


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

void handleMessage(uint8_t num, uint8_t* payload) {
    JsonDocument messageJon;
    DeserializationError error = deserializeJson(messageJon, payload);
    if (error) {
        app_log_e("Deserialization failed: " + String(error.c_str()));
        return;
    }

    if (!messageJon["key"].is<const char*>()) {
        app_log_e("No 'key' in message");
        return;
    }
    String key = messageJon["key"].as<String>();
    app_log("Received key: " + key);

    if (key == "otaUploadFirmware") {
        OtaUpdate::handleCommand(websocket, num, messageJon);
    }
    else if (key == "initSerialMonitor") {
        mainLog.setLogModeWebSockets(&websocket, num);
        app_log("Serial monitor client connected: " + String(num));
    }
    else if(key == "restart"){
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
        else{
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
        uint32_t value = messageJon["value"].as<uint32_t>();
        if(value > 255){
            app_log_e("value out of range(uint8_t)");
            return;
        }
        uint8_t temp = value;
        if (!ip5356->_write(addr, reg, &temp)) {
            app_log_e("Can not read ip5356 (maybe sleep)");
        }
    }
    else if(key == "readFileFromLittleFS"){
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
        while(readByte < file.size()){
            read = file.read(buffer, 1024);
            websocket.sendBIN(num, buffer, read);
            readByte += read;
        }
        websocket.sendTXT(num, "{\"key\":\"readFileSuccess\"}");
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
        if(OtaUpdate::isRunning()){
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

    while (true) {
        websocket.loop();
        vTaskDelay(10);
    }
}