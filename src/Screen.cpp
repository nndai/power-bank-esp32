#include <Arduino.h>
#include "Screen.h"
#include "Globals.h"

void Screen::setNeedRedraw(bool needRedraw) {
    _needRedraw = needRedraw;
}

bool Screen::getNeedRedraw() const {
    return _needRedraw;
}

void MainScreen::draw() {
    if (!_needRedraw) return;
    //img->fillSprite(TFT_BACKGROUND_COLOR);
    img->fillRectVGradient(0, 0, 160, 80, 0x3410, TFT_BACKGROUND_COLOR);
    // if (!ip5356->isConnected()) {
    //     img->setTextColor(TFT_RED);
    //     img->setTextSize(2);
    //     img->setFreeFont();
    //     img->drawString("IP5356 not", 21, 22);
    //     img->drawString("connected", 27, 44);
    //     img->pushSprite(0, 0);
    //     _needRedraw = false;
    //     return;
    // }

    img->setTextColor(0xFFFF);
    img->setTextSize(5);
    img->setFreeFont(&Org_01);

    uint8_t batteryPercent = ip5356->getBatteryPercentage();
    uint8_t numOne = (batteryPercent % 10 == 1) + (batteryPercent / 10 == 1);
    int8_t paddingNumber = (batteryPercent == 100 || numOne == 0) ? 0 : ((numOne == 1) ? 8 : 13);
    int8_t paddingSymbol = ((batteryPercent == 100) ? 10 : ((numOne == 0) ? 0 : ((numOne == 1) ? -12 : -27)));
    if (batteryPercent < 10) {
        img->setTextColor(TFT_RED);
        img->drawString(String("0") + (String)batteryPercent, 7 + paddingNumber, 7);
    }
    else {
        img->drawString((String)batteryPercent, 7 + paddingNumber, 7);
    }

    img->drawEllipse(68 + paddingSymbol, 23, 2, 2, 0xFFFF);
    img->drawEllipse(75 + paddingSymbol, 29, 2, 2, 0xFFFF);
    img->drawRect(74 + paddingSymbol, 28, 3, 3, 0xFFFF);
    img->drawRect(67 + paddingSymbol, 22, 3, 3, 0xFFFF);
    img->drawLine(75 + paddingSymbol, 22, 67 + paddingSymbol, 30, 0xFFFF);
    img->drawLine(76 + paddingSymbol, 22, 68 + paddingSymbol, 30, 0xFFFF);

    img->fillRoundRect(5, 46, 70, 14, 3, 0x24BE);
    img->setTextColor(0x0);
    img->setTextSize(2);

    if (ip5356->getUSB2Current() > 0.001f) {
        img->drawString("A", 14, 48);
        img->drawString("OUT", 33, 48);
    }
    else {
        img->drawString("A", 34, 48);
    }

    img->fillRoundRect(85, 46, 70, 14, 3, 0xFF47);

    IP5356::PORT_STATE typeCState = ip5356->getVBUSPortStatus();
    if (typeCState == IP5356::PORT_STATE::SOURCE) {
        img->drawString("C", 94, 48);
        img->drawString("OUT", 115, 48);
    }
    else if (typeCState == IP5356::PORT_STATE::SINK) {
        img->drawString("C", 94, 48);
        img->drawString("IN", 120, 48);
    }
    else {
        img->drawString("C", 115, 48);
    }

    img->setTextColor(0xFFFF);
    float usbAPower = ip5356->getUSB2Power() / 1000.0;
    if (usbAPower < 10) {
        img->drawString(String(usbAPower, 3) + "W", 9, 66);
    }
    else {
        img->drawString(String(usbAPower, 2) + "W", 9, 66);
    }

    float typeCPower = ip5356->getVBUSPower() / 1000.0f;
    if (typeCPower < 10) {
        img->drawString(String(typeCPower, 3) + "W", 89, 66);
    }
    else {
        img->drawString(String(typeCPower, 2) + "W", 89, 66);
    }

    if (ip5356->getSystemVoltage() > 6000) {
        img->drawBitmap(64 + paddingSymbol, 5, imageQuickCharge, 16, 16, TFT_GREEN);
    }

    IP5356::NTC_STATE_FLAG ntcState = ip5356->getNtcState();
    if (ntcState == IP5356::NTC_STATE_FLAG::HIGH_TEMPERATURE) {
        img->drawBitmap(83 + paddingSymbol, 16, imageWeatherTemperature, 16, 16, TFT_RED);
    }
    else if (ntcState == IP5356::NTC_STATE_FLAG::MEDIUM_HIGH_TEMPERATURE) {
        img->drawBitmap(83 + paddingSymbol, 16, imageWeatherTemperature, 16, 16, TFT_ORANGE);
    }

    img->pushSprite(0, 0);
}

void MainScreen::handleButton(Action action) {
}

void InfoScreen::draw() {
    if (!_needRedraw) return;
    //img->fillSprite(TFT_BACKGROUND_COLOR);
    img->fillRectVGradient(0, 0, 160, 80, 0x3410, TFT_BACKGROUND_COLOR);
    // if (!ip5356->isConnected()) {
    //     img->setTextColor(TFT_RED);
    //     img->setTextSize(2);
    //     img->setFreeFont();
    //     img->drawString("IP5356 not", 21, 22);
    //     img->drawString("connected", 27, 44);
    //     img->pushSprite(0, 0);
    //     _needRedraw = false;
    //     return;
    // }

    img->fillRoundRect(1, 47, 79, 32, 7, TFT_DARKGREEN);
    img->fillRoundRect(1, 1, 99, 40, 7, 0x5FA);
    img->fillRoundRect(105, 1, 54, 40, 7, 0xFC00);
    img->fillRoundRect(85, 47, 74, 32, 7, 0x24BE);

    img->setTextColor(0xFFFF);
    img->setTextSize(1);
    img->setFreeFont();

    img->drawString("BATTERY", 30, 3);
    float batteryVoltage = ip5356->getBatteryVoltage() / 1000.0f;
    float batteryCurrent = ip5356->getBatteryCurrent() / 1000.0f;
    float batteryPower = batteryCurrent * batteryVoltage;

    img->drawString(String(batteryVoltage, 3) + "v", 8, 13);
    img->drawString(String(batteryCurrent, 3) + "a", 8, 22);

    if (batteryPower < 10) {
        img->drawString(String(batteryPower, 3) + "w", 8, 31);
    }
    else {
        img->drawString(String(batteryPower, 2) + "w", 8, 31);
    }

    img->drawString(String(ip5356->getBatteryTemperature(), 2) + "c", 51, 13);
    img->drawString("CC:" + String(powerBankData.chargeCycle), 51, 22);
    img->drawString(String(powerBankData.batCapacity, 0) + "mah", 51, 31);

    img->drawString("TYPEC", 4, 54);

    float typeCVoltage = ip5356->getVBUSVoltage() / 1000.0f;
    float typeCCurrent = ip5356->getVBUSCurrent() / 1000.0f;
    float typeCPower = typeCVoltage * typeCCurrent;
    if (typeCVoltage < 10) {
        img->drawString(String(typeCVoltage, 3) + "v", 39, 52);
    }
    else {
        img->drawString(String(typeCVoltage, 2) + "v", 39, 52);
    }

    if (typeCPower < 10) {
        img->drawString(String(typeCPower, 3) + "w", 39, 70);
    }
    else {
        img->drawString(String(typeCPower, 2) + "w", 39, 70);
    }

    img->drawString(String(typeCCurrent, 3) + "a", 39, 61);

    String typeCProtocol = ip5356->protocolToString(ip5356->getVBUSSourceProtocol());
    img->drawString(typeCProtocol, (typeCProtocol.length() == 4) ? 8 : ((typeCProtocol.length() == 3) ? 10 : 12), 66);

    img->drawString("USBA", 89, 54);

    float usbAVoltage = ip5356->getUSB2Voltage() / 1000.0f;
    float usbACurrent = ip5356->getUSB2Current() / 1000.0f;
    float usbAPower = usbAVoltage * usbACurrent;

    if (usbAVoltage < 10) {
        img->drawString(String(usbAVoltage, 3) + "v", 117, 52);
    }
    else {
        img->drawString(String(usbAVoltage, 2) + "v", 117, 52);
    }

    if (usbAPower < 10) {
        img->drawString(String(usbAPower, 3) + "w", 117, 70);
    }
    else {
        img->drawString(String(usbAPower, 2) + "w", 117, 70);
    }
    img->drawString(String(usbACurrent, 3) + "a", 117, 61);

    String usbAProtocol = ip5356->protocolToString(ip5356->getUSB2SourceProtocol());
    img->drawString(usbAProtocol, (usbAProtocol.length() == 4) ? 89 : ((usbAProtocol.length() == 3) ? 92 : 95), 66);

    img->drawString("IP5356", 114, 3);
    float internalTemperature = ip5356->getInternalTemperature();
    if (internalTemperature < 100) {
        img->drawString(String(internalTemperature, 2) + "c", 115, 32);
    }
    else {
        img->drawString(String(internalTemperature, 1) + "c", 115, 32);
    }

    float psys = ip5356->getSystemPower() / 1000.0f;
    if (psys < 10) {
        img->drawString(String(psys, 3) + "w", 115, 13);
    }
    else {
        img->drawString(String(psys, 2) + "w", 115, 13);
    }

    img->drawString("PC:" + String(powerBankData.percentCharged), 115, 22);

    img->pushSprite(0, 0);
}

void InfoScreen::handleButton(Action action) {
}


