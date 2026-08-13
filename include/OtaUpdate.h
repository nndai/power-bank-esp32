#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Log.h>
#include <WebSocketsServer.h>


#define KEY_UPLOAD_FIRMWARE "otaUploadFirmware"
#define CMD_UPLOAD_FIRMWARE_START "uploadFirmwareStart"
#define CMD_UPLOAD_FIRMWARE_END "uploadFirmwareEnd"


class OtaUpdate {
private:
    static TFT_eSprite* _img;
    static std::function<void(bool success)> _onComplete;

    static void showProgressOnScreen();

public:
    static void setDisplay(TFT_eSprite* img);
    static void setOnComplete(std::function<void(bool success)> onComplete);

    static void handleCommand(WebSocketsServer& websocket, const uint8_t& num, const JsonDocument& command);

    // write bytes and update display progress
    static void write(uint8_t* payload, const uint32_t& length);

    static bool isRunning();
};

#endif // OTA_UPDATE_H