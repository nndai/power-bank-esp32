#ifndef OTAUPDATE_DISPLAY_H
#define OTAUPDATE_DISPLAY_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Log.h>
#include <WebSocketsServer.h>

#define KEY_UPLOAD_FIRMWARE "otaUploadFirmware"
#define CMD_UPLOAD_FIRMWARE_START "uploadFirmwareStart"
#define CMD_UPLOAD_FIRMWARE_END "uploadFirmwareEnd"



class OtaUpdateDisplay {
private:
    OtaUpdateDisplay() = default;

    static TFT_eSprite* _img;
    static uint32_t _totalSize;
    static uint32_t _written;
    static int8_t _lastPercent;

    static void showProgressOnScreen() {
        if (!_img) return;
        int8_t percent = (_totalSize == 0) ? 0 : (int)((_written * 100ULL) / (uint64_t)_totalSize);
        if (percent == _lastPercent) return;
        _lastPercent = percent;

        _img->fillSprite(TFT_BACKGROUND_COLOR);

        _img->setTextColor(0x55E);
        _img->drawString("OTA UPLOAD FIRMWARE", 24, 5);

        _img->drawRoundRect(6, 58, 120, 9, 3, 0xFE00);

        _img->setTextColor(0xFE00);
        _img->drawString(String(percent) + "%", 132, 59);

        _img->fillRoundRect(7, 59, (118 * percent) / 100, 7, 2, 0x4B1);

        _img->setTextColor(0xFFFF);
        _img->drawString("Total Size:", 9, 30);
        _img->drawString("Written:", 9, 42);
        _img->setTextDatum(TR_DATUM);
        _img->drawString(String(_totalSize) + " byte", 150, 30);
        _img->drawString(String(_written) + " byte", 150, 42);
        _img->setTextDatum(TL_DATUM);

        _img->pushSprite(0, 0);
    }

public:
    static void setDisplay(TFT_eSprite* img) {
        _img = img;
        _totalSize = 0;
        _written = 0;
        _lastPercent = -1;
        if (_img) {
            _img->setTextSize(1);
            _img->setFreeFont();
            showProgressOnScreen();
        }
    }

    static void handleCommand(WebSocketsServer& websocket, const uint8_t& num, const JsonDocument& command) {
        if (_img == nullptr) {
            app_log_e("Display not set for OtaUpdateDisplay");
            websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareFailed\", \"error\":\"Display not set\"}");
            return;
        }
        if (!command["cmd"].is<const char*>()) {
            app_log_e("No 'cmd' in json");
            return;
        }
        String cmd = command["cmd"].as<String>();
        if (cmd == CMD_UPLOAD_FIRMWARE_START) {
            if (Update.isRunning()) Update.abort();
            if (!command["size"].is<uint32_t>()) {
                app_log_e("Not 'size' in json");
                return;
            }
            _totalSize = command["size"].as<uint32_t>();
            _written = 0;
            _lastPercent = -1;

            if (!Update.begin(_totalSize)) {
                app_log_e("Update begin failed: " + String(Update.errorString()));
                websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
                return;
            }
            app_log("Firmware upload started, size: " + String(_totalSize));
            websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareSuccess\"}");
            showProgressOnScreen();
        }
        else if (cmd == CMD_UPLOAD_FIRMWARE_END) {
            if (Update.end()) {
                app_log_i("Update successful! -->restarting...");
                websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareSuccess\"}");
                _written = _totalSize;
                showProgressOnScreen();
                vTaskDelay(pdMS_TO_TICKS(500));
                websocket.disconnect();
                websocket.close();
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP.restart();
            }
            else {
                websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
                app_log_e("Update end failed: " + String(Update.errorString()));
                if (_img) {
                    _img->fillSprite(TFT_BLACK);
                    _img->setTextColor(TFT_RED, TFT_BLACK);
                    _img->setTextSize(2);
                    _img->drawString("OTA Failed", 8, 10);
                    _img->drawString(String(Update.errorString()), 8, 40);
                    _img->pushSprite(0, 0);
                }
            }
        }
        else {
            app_log_e("Unknown command: " + cmd);
            websocket.sendTXT(num, "{\"cmd\":\"unknownCommand\"}");
        }
    }

    // write bytes and update display progress
    static void write(uint8_t* payload, const uint32_t& length) {
        if (Update.isRunning()) {
            size_t writtenNow = Update.write(payload, length);
            if (writtenNow != length) {
                Update.abort();
            }
            else {
                _written += writtenNow;
                showProgressOnScreen();
            }
        }
    }

    static bool isRunning() {
        return Update.isRunning();
    }
};

// static members init
TFT_eSprite* OtaUpdateDisplay::_img = nullptr;
uint32_t OtaUpdateDisplay::_totalSize = 0;
uint32_t OtaUpdateDisplay::_written = 0;
int8_t OtaUpdateDisplay::_lastPercent = -1;

#endif // OTAUPDATE_DISPLAY_H