#ifndef OTAUPDATE_H
#define OTAUPDATE_H

#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Log.h>

#define KEY_UPLOAD_FIRMWARE "otaUploadFirmware"
#define CMD_UPLOAD_FIRMWARE_START "uploadFirmwareStart"
#define CMD_UPLOAD_FIRMWARE_END "uploadFirmwareEnd"

/**
 * @brief Class to handle OTA updates using WebSockets
 */
class OtaUpdate {
private:
    /**
     * @brief Private constructor to prevent instantiation
     */
    OtaUpdate() = default;

public:
    /**
     * @brief Handle incoming WebSocket messages
     * @param websocket The WebSocket server instance
     * @param num The client number
     * @param command The JSON command to handle
     */
    static void handleCommand(WebSocketsServer& websocket, const uint8_t& num, const JsonDocument& command) {
        if (!command["cmd"].is<const char*>()) {
            app_log_e("No 'cmd' in json");
            return;
        }
        String cmd = command["cmd"].as<String>();
        if (cmd == CMD_UPLOAD_FIRMWARE_START) {
            if (Update.isRunning()) {
                Update.abort();
            }
            if (!command["size"].is<uint32_t>()) {
                app_log_e("Not 'size' in json");
                return;
            }
            if (!Update.begin(command["size"].as<uint32_t>())) {
                app_log_e("Update begin failed: " + String(Update.errorString()));
                websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
                return;
            }
            app_log("Firmware upload started, size: " + String(command["size"].as<uint32_t>()));
            websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareSuccess\"}");
        }
        else if (cmd == CMD_UPLOAD_FIRMWARE_END) {
            if (Update.end()) {
                app_log_i("Update successful! -->restarting...");
                websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareSuccess\"}");
                vTaskDelay(pdMS_TO_TICKS(500));
                websocket.disconnect();
                websocket.close();
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP.restart();
            }
            else {
                websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
                app_log_e("Update end failed: " + String(Update.errorString()));
            }
        }
        else {
            app_log_e("Unknown command: " + cmd);
            websocket.sendTXT(num, "{\"cmd\":\"unknownCommand\"}");
        }
    }

    /**
     * @brief Write data to the update process
     * @param payload The data to write
     * @param length The length of the data
     */
    static void write(uint8_t* payload, const uint32_t& length) {
        if (Update.isRunning()) {
            if (Update.write(payload, length) != length) {
                Update.abort();
            }
        }
    }

    /**
     * @brief Check if an update is currently running
     * @return True if an update is running, false otherwise
     */
    static bool isRunning() {
        return Update.isRunning();
    }
};

#endif // OTAUPDATE_H