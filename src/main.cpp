#include <WiFi.h>
#include <Update.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include <HTTPClient.h>
#include <IP5356.h>
#include <Log.h>

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



    xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, NULL, 0);


    delay(1000);
    app_log("\nDa ket noi!");
    app_log("IP Address: ", "");
    app_log(WiFi.localIP().toString().c_str());
    app_log("RSSI: ", "");
    app_log(String(WiFi.RSSI()) + " dBm");
    app_log("Gateway: ", "");
    app_log(WiFi.gatewayIP().toString().c_str());
    app_log("Subnet: ", "");
    app_log(WiFi.subnetMask().toString().c_str());
    app_log("Broadcast IP: ", "\r");
    app_log(WiFi.softAPBroadcastIP().toString().c_str());
    app_log("Hostname: ", "");
    app_log(WiFi.getHostname());
    app_log("MAC Address: ", "");
    app_log(WiFi.macAddress().c_str());
    app_log("Channel: ", "");
    app_log(WiFi.channel());
    app_log("Mode: ", "");
    app_log(WiFi.getMode() == WIFI_MODE_STA ? "STA" : (
        WiFi.getMode() == WIFI_MODE_AP ? "AP" : "STA+AP"));
    app_log("Status: ", "");
    app_log(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");


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

void loop() {

    // // app_log(String(anaapp_logRead(35))+"", "\r");

    // // delay(100);
    // //scanI2C();

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
         vTaskDelay(1000);
    // }
    app_log(ip5356->getPercentage());

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
        app_log("Deserialization failed: " + String(error.c_str()));
        return;
    }

    if (!messageJon["key"].is<const char*>()) {
        app_log("No 'key' in message");
        return;
    }
    String key = messageJon["key"].as<String>();
    app_log("Received key: " + key);

    if (key == "uploadFirmwareStart") {
        if (Update.isRunning()) {
            Update.abort();
        }
        if (!messageJon["size"].is<uint32_t>()) {
            return;
        }
        if (!Update.begin(messageJon["size"].as<uint32_t>())) {
            app_log("Update begin failed: " + String(Update.errorString()));
            return;
        }
        app_log("Firmware upload started, size: " + String(messageJon["size"].as<uint32_t>()));
    }
    else if (key == "uploadFirmwareEnd") {
        if (Update.end()) {
            app_log("Update successful!\nrestarting...");
            websocket.sendTXT(num, "{\"key\":\"uploadFirmwareSuccess\"}");
            websocket.close();
            vTaskDelay(1000);
            ESP.restart();
        }
        else {
            websocket.sendTXT(num, "{\"key\":\"updateFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
            app_log("Update end failed: " + String(Update.errorString()));
        }
    }
    else if (key == "initSerialMonitor") {
        mainLog.setLogModeWebSockets(&websocket, num);
        app_log("Serial monitor client connected: " + String(num));
    }
    else {
        app_log("Unknown key: " + key);
    }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
    case WStype_TEXT:
        app_log("[" + String(num) + "] Received text message: " + String((char*)payload));
        handleMessage(num, payload);
        break;
    case WStype_BIN:
        if (Update.isRunning()) {
            if (Update.write(payload, length) != length) {
                Update.abort();
            }
            else {
                size_t total_byte_received = Update.progress() + length;
                String end_char = "\r";
                if (Update.isFinished()) {
                    end_char = "\n";
                }
                app_log("Received: " + String(total_byte_received) + "/" + String(Update.size())
                    + " bytes | " + String((total_byte_received * 100) / Update.size()) + "%", end_char);
            }
        }
        break;
    case WStype_CONNECTED:
        app_log("[" + String(num) + "] Connected!");
        break;
    case WStype_DISCONNECTED:
        app_log("[" + String(num) + "] Disconnected!");
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