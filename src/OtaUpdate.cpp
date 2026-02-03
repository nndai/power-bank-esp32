#include <Arduino.h>
#include "OtaUpdate.h"
#include "Globals.h"

TFT_eSprite* OtaUpdate::_img = nullptr;

void OtaUpdate::showProgressOnScreen() {
    if (!_img) return;
    static int8_t lastPercent = -1;
    int8_t percent = (Update.size() == 0) ? 0 : (int)((Update.progress() * 100ULL) / (uint64_t)Update.size());
    if (percent == lastPercent) {
        return;
    }
    lastPercent = percent;

    _img->fillSprite(TFT_BACKGROUND_COLOR);

    _img->setTextSize(1);
    _img->setTextColor(TFT_BLUE);
    _img->drawString("OTA UPLOAD FIRMWARE", 24, 8);

    _img->drawRoundRect(6, 58, 120, 9, 3, TFT_ORANGE);

    _img->setTextColor(TFT_ORANGE);
    _img->drawString(String(percent) + "%", 132, 59);

    _img->fillRoundRect(7, 59, (118 * percent) / 100, 7, 2, TFT_DARKGREEN);

    _img->setTextColor(TFT_WHITE);
    _img->drawString("Total Size:", 9, 30);
    _img->drawString("Written:", 9, 42);

    _img->setTextDatum(TR_DATUM);
    _img->drawString(String(Update.size()) + " byte", 150, 30);
    _img->drawString(String(Update.progress()) + " byte", 150, 42);
    _img->setTextDatum(TL_DATUM);

    _img->pushSprite(0, 0);
}

void OtaUpdate::setDisplay(TFT_eSprite* img) {
    _img = img;
    if (_img) {
        _img->setTextSize(1);
        _img->setFreeFont();
        showProgressOnScreen();
    }
}

void OtaUpdate::handleCommand(WebSocketsServer& websocket, const uint8_t& num, const JsonDocument& command) {
    if (!command["cmd"].is<const char*>()) {
        _app_log_e("No 'cmd' in json");
        return;
    }

    String cmd = command["cmd"].as<String>();
    if (cmd == CMD_UPLOAD_FIRMWARE_START) {
        if (Update.isRunning()) Update.abort();
        if (!command["size"].is<uint32_t>()) {
            _app_log_e("Not 'size' in json");
            return;
        }
        uint32_t totalSize = command["size"].as<uint32_t>();

        if (!Update.begin(totalSize)) {
            _app_log_e("Update begin failed: " + String(Update.errorString()));
            websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
            return;
        }
        _app_log_i("Firmware upload started, size: " + String(totalSize));
        websocket.sendTXT(num, "{\"cmd\":\"beginUploadFirmwareSuccess\"}");

        showProgressOnScreen();
    }
    else if (cmd == CMD_UPLOAD_FIRMWARE_END) {
        if (_img) {
            _img->fillSprite(TFT_BACKGROUND_COLOR);
            _img->setTextColor(TFT_BLUE);
            _img->drawString("OTA UPLOAD FIRMWARE", 24, 8);
        }
        if (Update.end()) {
            _app_log_i("Update successful! -->restarting...");
            websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareSuccess\"}");
            if (_img) {
                _img->setTextColor(TFT_GREEN);
                _img->drawString("Upload firmware success", 12, 41);
                _img->drawString("restarting...", 42, 54);
                _img->pushSprite(0, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            websocket.disconnect();
            vTaskDelay(pdMS_TO_TICKS(500));
            websocket.close();
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
        }
        else {
            websocket.sendTXT(num, "{\"cmd\":\"uploadFirmwareFailed\", \"error\":\"" + String(Update.errorString()) + "\"}");
            _app_log_e("Update end failed: " + String(Update.errorString()));
            if (_img) {
                _img->setTextColor(TFT_RED);
                _img->drawString("Upload firmware failed", 15, 46);
                _img->pushSprite(0, 0);
            }
        }
    }
    else {
        _app_log_e("Unknown command: " + cmd);
        websocket.sendTXT(num, "{\"cmd\":\"unknownCommand\"}");
    }
}

void OtaUpdate::write(uint8_t* payload, const uint32_t& length) {
    if (Update.isRunning()) {
        if (Update.write(payload, length) != length) {
            Update.abort();
        }
        else {
            showProgressOnScreen();
        }
    }
}

bool OtaUpdate::isRunning() {
    return Update.isRunning();
}
