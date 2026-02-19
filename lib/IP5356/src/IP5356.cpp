#include "IP5356.h"


const uint8_t IP5356::checkReg0x74Arr[256] = {
    2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 2, 2, 0, 0, 2,
    2, 0, 0, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2,
    2, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 0, 2,
    2, 2, 0, 2, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, };

const uint8_t IP5356::checkReg0x75Arr[256] = {
    0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, };

IP5356::IP5356(TwoWire* wire, int8_t ntcDiePin, int8_t keyPin, int8_t intPin) {
    log_i("IP5356 init");
    if (wire == nullptr) {
        _i2c = &Wire;
    }
    else {
        _i2c = wire;
    }

    _ntcDiePin = ntcDiePin;
    _keyPin = keyPin;
    _intPin = intPin;

    if (_ntcDiePin != -1) {
        pinMode(_ntcDiePin, INPUT);
    }
    if (_keyPin != -1) {
        pinMode(_keyPin, OUTPUT);
        digitalWrite(_keyPin, HIGH);
    }
    if (_intPin != -1) {
        pinMode(_intPin, INPUT);
        if (digitalRead(_intPin) == LOW) {
            shortPressKey();
        }
    }
}

void IP5356::init() {
    uint8_t value = 0;

    // iNTC = 80μA
    if (!_read(NTC_CTL0, &value)) return;
    value &= ~(0b11 << 4);
    value |= (0b10 << 4);
    _write(NTC_CTL0, &value);

    while (true) {
        isysLowAutoOff(false);
        value = 0;
        if (!_read(SYS_CTL1, &value)) return;
        if (value & 0b100) {
            delay(10);
            continue;
        }
        else {
            break;
        }
    }

    // reduce a haft charge current when high temperature battery
    // if (!_read(NTC_CTL1, &value)) return;
    // value |= 0b01000000;
    // _write(NTC_CTL1, &value);
}

bool IP5356::_read(uint8_t addr, uint8_t reg, uint8_t* data) {
    if (_intPin != -1) {
        if (!isWork()) return false;
    }
    else if (!isConnected()) return false; // accept if isWork == true then isConnected = true

    if (!checkRegRead(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not read");
        return false;
    }
    _i2c->beginTransmission(addr);
    _i2c->write(reg);
    _i2c->endTransmission();
    _i2c->requestFrom(addr, (uint8_t)1);
    if (_i2c->available()) {
        *data = _i2c->read();
        return true;
    }
    return false;
}

bool IP5356::_read(uint8_t addr, uint8_t reg, uint16_t* data) {
    if (_intPin != -1) {
        if (!isWork()) return false;
    }
    else if (!isConnected()) return false; // accept if isWork == true then isConnected = true

    if (!checkRegRead(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not read");
        return false;
    }
    _i2c->beginTransmission(addr);
    _i2c->write(reg);
    _i2c->endTransmission();
    _i2c->requestFrom(addr, (uint8_t)2);
    if (_i2c->available() >= 2) {
        *data = _i2c->read() | _i2c->read() << 8;
        return true;
    }
    return false;
}

bool IP5356::_write(uint8_t addr, uint8_t reg, uint8_t* data) {
    if (_intPin != -1) {
        if (!isWork()) {
            app_log_e("write [0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "] failed");
            return false;
        }
    }
    else if (!isConnected()) {  // accept if isWork == true then isConnected = true
        app_log_e("write [0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "] failed");
        return false;
    } 

    if (!checkRegWrite(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not write");
        return false;
    }

    uint8_t oldData = 0;
    if (!_read(addr, reg, &oldData)) {
        app_log("read IP5356 error, no write to ip5356");
        return false;
    }
    String logStr = "write [0x" + String(addr, HEX) + ":0x" + String(reg, HEX) + "]: " + String(oldData) + " -> " + String(*data);

    _i2c->beginTransmission(addr);
    _i2c->write(reg);
    _i2c->write(*data);
    if (_i2c->endTransmission()) {
        app_log(logStr + "---> fail");
        return false;
    }
    app_log(logStr + "---> success");
    return true;
}

bool IP5356::checkRegRead(uint8_t addr, uint8_t reg) {
    if (addr == 0x74) {
        return checkReg0x74Arr[reg] != 0;
    }
    if (addr == 0x75) {
        return checkReg0x75Arr[reg] != 0;
    }
    return false;
}
bool IP5356::checkRegWrite(uint8_t addr, uint8_t reg) {
    if (addr == 0x74) {
        return checkReg0x74Arr[reg] == 2;
    }
    if (addr == 0x75) {
        return checkReg0x75Arr[reg] == 2;
    }
    return false;
}

bool IP5356::_read(IP5356_0X74_REG reg, uint8_t* data) {
    return _read(0x74, static_cast<uint8_t>(reg), data);
}

bool IP5356::_read(IP5356_0X74_REG reg, uint16_t* data) {
    return _read(0x74, static_cast<uint8_t>(reg), data);
}

bool IP5356::_read(IP5356_0X75_REG reg, uint8_t* data) {
    return _read(0x75, static_cast<uint8_t>(reg), data);
}

bool IP5356::_read(IP5356_0X75_REG reg, uint16_t* data) {
    return _read(0x75, static_cast<uint8_t>(reg), data);
}

bool IP5356::_write(IP5356_0X74_REG reg, uint8_t* data) {
    return _write(0x74, static_cast<uint8_t>(reg), data);
}

bool IP5356::_write(IP5356_0X75_REG reg, uint8_t* data) {
    return _write(0x75, static_cast<uint8_t>(reg), data);
}

String IP5356::loadRegData(uint8_t addr, uint8_t reg) {
    uint8_t value;
    if (addr == 0x74) {
        if (checkReg0x74Arr[reg] != 0) {
            _read((IP5356_0X74_REG)reg, &value);
        }
        else {
            return "";
        }
    }
    else if (addr == 0x75) {
        if (checkReg0x75Arr[reg] != 0) {
            _read((IP5356_0X75_REG)reg, &value);
        }
        else {
            return "";
        }
    }
    else {
        return "";
    }

    String str = "0x";
    if (reg < 16) {
        str += "0";
    }

    str += String(reg, HEX) + ": ";

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
    return str;
}

String IP5356::loadAllRegData() {
    String str = "IP5356 Register Data:\nIP5356 Address 0x74:\n";
    for (int i = 0; i < 256;i++) {
        String regValue = loadRegData(0x74, i);
        if (regValue == "") continue;
        str = str + regValue + "\n";
    }
    str += "\nIP5356 Address 0x75:\n";
    for (int i = 0; i < 256;i++) {
        String regValue = loadRegData(0x75, i);
        if (regValue == "") continue;
        str = str + regValue + "\n";
    }
    return str;
}

bool IP5356::isConnected() {
    _i2c->beginTransmission(address1);
    uint8_t error = _i2c->endTransmission();
    return (error == 0);
}

bool IP5356::isWork() {
    if (_intPin == -1) {
        return false;
    }
    return digitalRead(_intPin);
}

void IP5356::powerOff() {
    uint8_t value = 0xff;
    if (!_read(FORCE_STANDBY, &value)) return;
    value |= 0b10000000;
    _write(FORCE_STANDBY, &value);
}

void IP5356::isysLowAutoOff(bool on) {
    uint8_t value = 0;
    if (!_read(SYS_CTL1, &value)) return;
    if (value & 0b100) {
        if (!on) {
            value &= 0b11111011;
            _write(SYS_CTL1, &value);
        }
    }
    else {
        if (on) {
            value |= 0b100;
            _write(SYS_CTL1, &value);
        }
    }
}

void IP5356::set8sAutoOff() {
    uint8_t value = 0xff;
    if (!_read(SYS_CTL1, &value)) return;
    value &= ~(0b11 << 4);
    _write(SYS_CTL1, &value);
}


bool IP5356::checkIsysLow() {
    uint8_t value = -1;
    if (!_read(ILOW_STATE, &value)) return true;
    value &= 0b01000000;
    if (value) return true;
    return false;
}

void IP5356::reduceChargeCurrentWhenHot(bool on) {
    uint8_t value;
    if (!_read(IP5356::NTC_CTL1, &value)) return;
    if (value & 0b01000000) {
        if (!on) {
            value &= 0b10111111;
            _write(IP5356::NTC_CTL1, &value);
        }
    }
    else {
        if (on) {
            value |= 0b01000000;
            _write(IP5356::NTC_CTL1, &value);
        }
    }
}

uint8_t IP5356::getBatteryPercentage() {
    uint8_t value = -1;
    if (!_read(SOC_CAP_DATA, &value)) return 0;
    return value;
}

float IP5356::getBatteryVoltage() {
    uint16_t value = -1;
    if (!_read(BATVADC_DAT0, &value)) return 0.0f;
    return value * 0.26855f;
}

float IP5356::getBatteryCurrent() {
    uint16_t value = -1;
    if (!_read(IBATIADC_DAT0, &value)) return 0.0f;
    return 1.6785 * value;
}

float IP5356::getBatteryPower() {
    return getBatteryVoltage() * getBatteryCurrent() / 1000.0f;
}

float IP5356::getBatteryTemperature() {
    uint16_t value = -1;
    if (!_read(NTCVADC_DAT0, &value)) return 10.0f;
    float vNTC = 0.26855f * value / 1000.0f;
    float nowRNTC = vNTC / _iNTC;
    float tempK = 3590.0f / (log(nowRNTC / _rNTC) + (3590.0f / 298.15f));
    return  tempK - 273.15f;
}

float IP5356::getSystemVoltage() {
    uint16_t value = -1;
    if (!_read(VSYSVADC_DAT0, &value)) return 0.0f;
    return  1.611328f * value;
}

float IP5356::getSystemCurrent() {
    uint16_t value = -1;
    if (!_read(ISYS_IADC_DAT0, &value)) return 0.0f;
    return  0.671387f * value;
}

float IP5356::getSystemPower() {
    uint16_t value = -1;
    if (!_read(VSYS_POW_DAT0, &value)) return 0.0f;
    return  4.431 * value;
}

float IP5356::getUSB1Voltage() {
    uint16_t value = -1;
    if (!_read(VOUT1VADC_DAT0, &value)) return 0.0f;
    return  1.611328f * value;
}

float IP5356::getUSB1Current() {
    uint16_t value = -1;
    if (!_read(IVOUT1_IADC_DAT0, &value)) return 0.0f;
    return  0.671387f * value;
}

float IP5356::getUSB1Power() {
    return getUSB1Voltage() * getUSB1Current() / 1000.0f;
}

float IP5356::getUSB2Voltage() {
    uint16_t value = -1;
    if (!_read(VOUT2VADC_DAT0, &value)) return 0.0f;
    return  1.611328f * value;
}

float IP5356::getUSB2Current() {
    uint16_t value = -1;
    if (!_read(IVOUT2_IADC_DAT0, &value)) return 0.0f;
    return  0.671387f * value;
}

float IP5356::getUSB2Power() {
    return getUSB2Voltage() * getUSB2Current() / 1000.0f;
}

float IP5356::getVBUSVoltage() {
    uint16_t value = -1;
    if (!_read(VBUSVADC_DAT0, &value)) return 0.0f;
    return  1.611328f * value;
}

float IP5356::getVBUSCurrent() {
    uint16_t value = -1;
    if (getVBUSPortStatus() == PORT_STATE::SOURCE) {
        if (!_read(IBUS_IADC_DAT0, &value)) return 0.0f;
    }
    else {
        if (!_read(IVIN_IVBUS_IADC_DAT0, &value)) return 0.0f;
    }

    return  0.671387f * value;
}

float IP5356::getVBUSPower() {
    return getVBUSVoltage() * getVBUSCurrent() / 1000.0f;
}

float IP5356::getVINVoltage() {
    uint16_t value = -1;
    if (!_read(VINVADC_DAT0, &value)) return 0.0f;
    return  1.611328f * value;
}

float IP5356::getVINCurrent() {
    uint16_t value = -1;
    if (!_read(IVIN_IVBUS_IADC_DAT0, &value)) return 0.0f;
    return  0.671387f * value;
}

float IP5356::getVINPower() {
    return getVINVoltage() * getVINCurrent() / 1000.0f;
}

float IP5356::getInternalTemperature() {
    if (_ntcDiePin == -1) {
        return -100.0f; // NTC pin not defined
    }
    int adcValue = analogRead(_ntcDiePin);
    float vNTC = (adcValue / 4095.0f) * 3.3f;
    if (vNTC == 0.0f || vNTC == 3.3f) {
        return -100.0f;
    }

    float rFixed = 3700.0f;
    float nowRNTC = (vNTC * rFixed) / (3.3f - vNTC);
    float tempK = 3590.0f / (log(nowRNTC / _rNTC) + (3590.0f / 298.15f));
    return  tempK - 273.15f;
}

bool IP5356::setBatteryCapacity(uint16_t mAh) {
    // Calculate N from formula: FCAP = 385 * N mAH
    // N = round(mAh / 385)
    uint8_t n = (uint8_t)((mAh + 192) / 385); // Adding 192 (385/2) for rounding
    
    // Read current FCAP_REG value and only write if different
    uint8_t currentN = 0;
    if (!_read(FCAP_REG, &currentN)) {
        app_log_e("setBatteryCapacity: Failed to read FCAP_REG");
        return false;
    }
    
    if (currentN != n) {
        if (!_write(FCAP_REG, &n)) {
            app_log_e("setBatteryCapacity: Failed to write FCAP_REG");
            return false;
        }
        app_log("setBatteryCapacity: FCAP_REG changed from " + String(currentN) + " to " + String(n));
    }
    
    // Read EN_FCAP and clear bit 7 only if it's set
    uint8_t enFcapValue = 0;
    if (!_read(EN_FCAP, &enFcapValue)) {
        app_log_e("setBatteryCapacity: Failed to read EN_FCAP");
        return false;
    }
    
    if (enFcapValue & (1 << 7)) { // Only write if bit 7 is set
        enFcapValue &= ~(1 << 7); // Clear bit 7
        if (!_write(EN_FCAP, &enFcapValue)) {
            app_log_e("setBatteryCapacity: Failed to write EN_FCAP");
            return false;
        }
        app_log("setBatteryCapacity: EN_FCAP bit 7 cleared");
    }
    
    app_log("setBatteryCapacity: Capacity set to " + String(n * 385) + " mAh (N=" + String(n) + ")");
    return true;
}

void IP5356::deletePressTask() {
    if (_shortPressTaskHandle != NULL) {
        vTaskDelete(_shortPressTaskHandle);
        _shortPressTaskHandle = NULL;
    }
    if (_longPressTaskHandle != NULL) {
        vTaskDelete(_longPressTaskHandle);
        _longPressTaskHandle = NULL;
    }
    if (_doublePressTaskHandle != NULL) {
        vTaskDelete(_doublePressTaskHandle);
        _doublePressTaskHandle = NULL;
    }
    if (_resetWholeSystemTaskHandle != NULL) {
        vTaskDelete(_resetWholeSystemTaskHandle);
        _resetWholeSystemTaskHandle = NULL;
    }
    digitalWrite(_keyPin, HIGH);
}

void IP5356::shortPressKey() {
    if (_keyPin == -1) {
        return;
    }
    deletePressTask();
    xTaskCreatePinnedToCore([](void* param) {
        IP5356* instance = static_cast<IP5356*>(param);
        digitalWrite(instance->_keyPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(instance->_keyPin, HIGH);
        instance->_shortPressTaskHandle = NULL;
        vTaskDelete(NULL);
        }, "IP5356_shortPressKey", 2048, this, 1, &_shortPressTaskHandle, 1);
}

void IP5356::longPressKey() {
    if (_keyPin == -1) {
        return;
    }
    deletePressTask();
    xTaskCreatePinnedToCore([](void* param) {
        IP5356* instance = static_cast<IP5356*>(param);
        digitalWrite(instance->_keyPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(2500));
        digitalWrite(instance->_keyPin, HIGH);
        instance->_longPressTaskHandle = NULL;
        vTaskDelete(NULL);
        }, "IP5356_longPressKey", 2048, this, 1, &_longPressTaskHandle, 1);
}
void IP5356::doublePressKey() {
    if (_keyPin == -1) {
        return;
    }
    deletePressTask();
    xTaskCreatePinnedToCore([](void* param) {
        IP5356* instance = static_cast<IP5356*>(param);
        digitalWrite(instance->_keyPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(instance->_keyPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(instance->_keyPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(instance->_keyPin, HIGH);
        instance->_doublePressTaskHandle = NULL;
        vTaskDelete(NULL);
        }, "IP5356_doublePressKey", 2048, this, 1, &_doublePressTaskHandle, 1);
}
void IP5356::resetWholeSystem() {
    if (_keyPin == -1) {
        return;
    }
    deletePressTask();
    xTaskCreatePinnedToCore([](void* param) {
        IP5356* instance = static_cast<IP5356*>(param);
        digitalWrite(instance->_keyPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(13000));
        digitalWrite(instance->_keyPin, HIGH);
        instance->_resetWholeSystemTaskHandle = NULL;
        vTaskDelete(NULL);
        }, "IP5356_resetWholeSystem", 2048, this, 1, &_resetWholeSystemTaskHandle, 1);
}



IP5356::NTC_STATE_FLAG IP5356::getNtcState() {
    uint8_t value = 0;
    if (!_read(NTC_STATE, &value)) return NTC_STATE_FLAG::NORMAL_TEMPERATURE;
    if (value & 0b10000000) {
        return NTC_STATE_FLAG::HIGH_TEMPERATURE;
    }
    if (value & 0b01000000) {
        return NTC_STATE_FLAG::MEDIUM_HIGH_TEMPERATURE;
    }
    if (value & 0b00001000) {
        return NTC_STATE_FLAG::LOW_TEMPERATURE;
    }
    if (value & 0b00100000) {
        return NTC_STATE_FLAG::MEDIUM_LOW_TEMPERATURE;
    }
    return NTC_STATE_FLAG::NORMAL_TEMPERATURE;
}



String IP5356::protocolToString(FAST_CHARGE_PROTOCOL protocol) {
    switch (protocol) {
    case FAST_CHARGE_PROTOCOL::NONE: return "NONE";
    case FAST_CHARGE_PROTOCOL::DCP:return "DCP";
    case FAST_CHARGE_PROTOCOL::QC_HANDSHAKE_OK: return "QCOK";
    case FAST_CHARGE_PROTOCOL::QC2: return "QC2";
    case FAST_CHARGE_PROTOCOL::QC3: return "QC3";
    case FAST_CHARGE_PROTOCOL::FCP_SCP: return "FSCP";
    case FAST_CHARGE_PROTOCOL::AFC: return "AFC";
    case FAST_CHARGE_PROTOCOL::SFCP: return "SFCP";
    case FAST_CHARGE_PROTOCOL::PD: return "PD";
    case FAST_CHARGE_PROTOCOL::PPS: return "PPS";
    default: return "NONE";
    }
}

IP5356::FAST_CHARGE_PROTOCOL IP5356::convertValueToProtocol(uint8_t value) {
    switch (value) {
    case 0b0110:  return FAST_CHARGE_PROTOCOL::QC_HANDSHAKE_OK;
    case 0b1001:  return FAST_CHARGE_PROTOCOL::QC2;
    case 0b1010:  return FAST_CHARGE_PROTOCOL::QC3;
    case 0b1100:  return FAST_CHARGE_PROTOCOL::FCP_SCP;
    case 0b1101:  return FAST_CHARGE_PROTOCOL::AFC;
    case 0b1110:  return FAST_CHARGE_PROTOCOL::SFCP;
    case 0b0001:  return FAST_CHARGE_PROTOCOL::DCP;
    default:     return FAST_CHARGE_PROTOCOL::NONE;
    }
}

IP5356::FAST_CHARGE_PROTOCOL IP5356::getVBUSSourceProtocol() {
    uint8_t value = 0;
    if (!_read(STATUS_SRC1, &value)) {
        return FAST_CHARGE_PROTOCOL::NONE;
    }
    value &= 0b1111;

    FAST_CHARGE_PROTOCOL proto = convertValueToProtocol(value);
    if (proto != FAST_CHARGE_PROTOCOL::NONE) {
        return proto;
    }

    value = 0;
    _read(PD_STATE1, &value);
    value &= 0b10000000;
    if (value) {
        return FAST_CHARGE_PROTOCOL::PD;
    }

    value = 0;
    _read(PD_STATE2, &value);
    value &= 0b1;
    if (value) {
        return FAST_CHARGE_PROTOCOL::PPS;
    }
    return FAST_CHARGE_PROTOCOL::NONE;
}

IP5356::FAST_CHARGE_PROTOCOL IP5356::getUSB1SourceProtocol() {
    uint8_t value = 0;
    if(!_read(STATUS_SRC0, &value)) return FAST_CHARGE_PROTOCOL::NONE;
    value &= 0b1111;
    return convertValueToProtocol(value);
}

IP5356::FAST_CHARGE_PROTOCOL IP5356::getUSB2SourceProtocol() {
    uint8_t value = 0;
    if (!_read(STATUS_SRC0, &value)) return FAST_CHARGE_PROTOCOL::NONE;
    value &= 0b11110000;
    value >>= 4;
    return convertValueToProtocol(value);
}

IP5356::PORT_STATE IP5356::getVBUSPortStatus() {
    uint8_t value = 0;
    if (!_read(TYPEC_STATE, &value)) return PORT_STATE::NONE;
    uint8_t b7 = value & 0b10000000;
    if (b7) {
        return PORT_STATE::SOURCE;
    }

    uint8_t b6 = value & 0b01000000;
    if (b6) {
        return PORT_STATE::SINK;
    }
    return PORT_STATE::NONE;
}

IP5356::PORT_STATE IP5356::getUSB2PortStatus() {
    uint8_t value = 0;
    if (!_read(STATUS_SRC1, &value))  return PORT_STATE::NONE;
    value &= 0b01110000;
    if (value == 0b00100000) {
        return PORT_STATE::SOURCE;
    }
    return PORT_STATE::NONE;
}

IP5356::CHARGE_STATUS IP5356::getChargeStatus() {
    uint8_t value = 0;
    if (!_read(CHG_STATE2, &value))  return CHARGE_STATUS::NOT_CHARGING;
    if (!(value & 0b1000)) {
        return CHARGE_STATUS::NOT_CHARGING;
    }
    value >>= 4;
    value &= 0b111;
    switch (value) {
    case 0b001: return CHARGE_STATUS::TRICKLE_CHARGE;
    case 0b010: return CHARGE_STATUS::CONSTANT_CURRENT;
    case 0b011: return CHARGE_STATUS::CONSTANT_VOLTAGE;
    case 0b101: return CHARGE_STATUS::FULLY_CHARGED;
    case 0b110: return CHARGE_STATUS::CHARGE_TIMEOUT;
    }
    return CHARGE_STATUS::NOT_CHARGING;
}

