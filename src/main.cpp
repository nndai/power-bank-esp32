#include <WiFi.h>
#include <Update.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <HTTPClient.h>
//#include <Wire.h>
#include <IP5356.h>
#include "ip5356.h"

const char* ssid = "DESKTOP-P5540";
const char* password = "aaaaaaaa";

IPAddress localIp(192, 168, 137, 194);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
WebSocketsServer websocket(82);

//IP5356 ip5356(0x75); // Default I2C address for IP5356

uint8_t clientSerialMonitor = 255;

void webSocketTask(void* p);
void loginWifiFree(void* p);
String timeConnectWifiFree();
void log(String& msg, char end_char = '\n');
void log(const char* msg, char end_char = '\n');


void setup() {
    Serial.begin(115200);

   // Wire.begin(); // SDA, SCL

    if (!WiFi.config(localIp, gateway, subnet)) {
        log("STA Failed to configure");
    }

    WiFi.begin(ssid, password);
    Serial.print("Dang ket noi toi WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }


    ///
    xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, NULL, 0);
    ///


    delay(1000);
    log("\nDa ket noi!");
    Serial.print("IP Address: ");
    log(WiFi.localIP().toString().c_str());
    Serial.print("RSSI: ");
    log(String(WiFi.RSSI()) + " dBm");
    Serial.print("Gateway: ");
    log(WiFi.gatewayIP().toString().c_str());
    Serial.print("Subnet: ");
    log(WiFi.subnetMask().toString().c_str());
    Serial.print("Broadcast IP: ");
    log(WiFi.softAPBroadcastIP().toString().c_str());
    Serial.print("Hostname: ");
    log(WiFi.getHostname());
    Serial.print("MAC Address: ");
    log(WiFi.macAddress().c_str());
    Serial.print("Channel: ");
    log(WiFi.channel());
    Serial.print("Mode: ");
    log(WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP"));
    Serial.print("Status: ");
    log(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    
    // while (digitalRead(5) == LOW) {
    //     delay(50);
    //     log("Waiting...");
    // }
    //xTaskCreatePinnedToCore(loginWifiFree, "LoginWifiFree", 8192, NULL, 3, NULL, 0);


    gpio_reset_pin(SDA_PIN);
    gpio_reset_pin(SCL_PIN);
    gpio_set_direction(SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCL_PIN, GPIO_MODE_OUTPUT);
    P_I2C_SDA_SET;
    P_I2C_SCL_SET;
    delay(1000);
}

// void scanI2C() {
//     log("Scanning I2C bus...");
//     byte count = 0;
//     for (uint8_t address = 1; address < 128; address++) {
//         Wire.beginTransmission(address);
//         if (Wire.endTransmission() == 0) {
//             log("Found I2C device at address 0x" + String(address, HEX));
//             count++;
//             delay(10);
//         }
//     }
//     if (count == 0) {
//         log("No I2C devices found.");
//     }
//     else {
//         log("Scan done. Found " + String(count) + " device(s).");
//     }
// }

// void loop() {

//     // // log(String(analogRead(35))+"", '\r');

//     // // delay(100);
//     // //scanI2C();

//     // for (int i = 0; i < 10; i++) {
//     //     //scanI2C();
//     //     // uint8_t ip5356_status = ip5356.readRegister(0x7b);
//     //     // String statusStr = "IP5356 Status: " + String(ip5356_status);
//     //     // log(statusStr);

//     //     Wire.beginTransmission(0x75);
//     //     if (Wire.endTransmission() == 0) {
//     //         log("Found: " + String(0x75, HEX));
//     //     }
//     //     else {
//     //         log("No");
//     //     }
        
//     //     digitalWrite(21, HIGH);
//     //     // Wire.beginTransmission(0x75); // IP5356 default address
//     //     // if (Wire.endTransmission() == 0) {
//     //     //     log("IP5356 is connected.");
//     //     // }
//     //     // else {
//     //     //     log("IP5356 is not connected.");
//     //     // }
//     //     vTaskDelay(500);
//     // }

//     // // log("21 value: " + String(digitalRead(21)));
//     // // log("22 value: " + String(digitalRead(22)));

//     // for(int i = 0; i < 5; i++) {
//     //     log("Loop iteration: " + String(i+1));
//     //     vTaskDelay(1000);
//     // }
// }


void loop() {
    uint8_t low = ip53xs_readByte(0x3c, 0x16);
    uint8_t high = ip53xs_readByte(0x3c, 0x15);
    uint16_t raw = ((high & 0x0F) << 8) | low;
    float voltage = raw * 1.2 / 1000.0;
    log("Battery voltage: " + String(voltage, 3) + " V");

    for(int i = 0; i < 2; i++) {
        log("Loop iteration: " + String(i + 1));
        vTaskDelay(1000);
    }
}




void log(String& msg, char end_char) {

    if (clientSerialMonitor != 255) {
        websocket.sendTXT(clientSerialMonitor, msg + end_char);
    }
    else {
        Serial.print(msg + String(end_char));
    }
}

void log(const char* msg, char end_char) {
    if (clientSerialMonitor != 255) {
        websocket.sendTXT(clientSerialMonitor, msg + String(end_char));
    }
    else {
        Serial.print(msg + String(end_char));
    }
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

bool login() {
    HTTPClient login;
    login.begin("http://" + getIpGateway() + "/login");
    String body = "username=awing15-15&password=Awing15-15@2023";
    int res = login.POST(body);
    log("Login: " + String(res));
    login.end();
    return res > 0 && res < 400;
}

bool logout() {
    HTTPClient logout;
    logout.begin("http://" + getIpGateway() + "/logout?");
    int res = logout.GET();
    log("Logout: " + String(res));
    logout.end();
    return res > 0 && res < 400;
}

void loginWifiFree(void* p) {
    while (true) {
        if (logout()) {
            vTaskDelay(500);
            if (login()) {
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
        log("Deserialization failed: " + String(error.c_str()));
        return;
    }

    if (!messageJon["key"].is<const char*>()) {
        log("No 'key' in message");
        return;
    }
    String key = messageJon["key"].as<String>();
    log("Received key: " + key);

    if (key == "uploadFirmwareStart") {
        if (Update.isRunning()) {
            Update.abort();
        }
        if (!messageJon["size"].is<uint32_t>()) {
            return;
        }
        if (!Update.begin(messageJon["size"].as<uint32_t>())) {
            log("Update begin failed: " + String(Update.errorString()));
            return;
        }
        log("Firmware upload started, size: " + String(messageJon["size"].as<uint32_t>()));
    }
    else if (key == "uploadFirmwareEnd") {
        if (Update.end()) {
            log("Update successful!\nrestarting...");
            websocket.sendTXT(num, "{\"key\":\"updateFirmwareSuccess\"}");
            vTaskDelay(1000);
            ESP.restart();
        }
        else {
            log("Update end failed: " + String(Update.errorString()));
        }
    }
    else if (key == "initSerialMonitor") {
        clientSerialMonitor = num;
        log("Serial monitor client connected: " + String(num));
    }
    else {
        log("Unknown key: " + key);
    }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_TEXT:
        log("[" + String(num) + "] Received text message: " + String((char*)payload));
        handleMessage(num, payload);
        break;
    case WStype_BIN:
        if (Update.isRunning()) {
            if (Update.write(payload, length) != length) {
                Update.abort();
            }
            else {
                size_t total_byte_received = Update.progress() + length;
                char end_char = '\r';
                if (Update.isFinished()) {
                    end_char = '\n';
                }
                log(" Received: " + String(total_byte_received) + "/" + String(Update.size())
                    + " bytes | " + String((total_byte_received * 100) / Update.size()) + "%", end_char);
            }
        }
        break;
    case WStype_CONNECTED:
        log("[" + String(num) + "] Connected!");
        break;
    case WStype_DISCONNECTED:
        log("[" + String(num) + "] Disconnected!");
        if (num == clientSerialMonitor) {
            clientSerialMonitor = 255; // Reset serial monitor client
            log("Serial monitor client disconnected");
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