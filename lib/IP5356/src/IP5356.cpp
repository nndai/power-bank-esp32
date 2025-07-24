#include "IP5356.h"


IP5356::IP5356() {
    if (!_log) {
        _log = new FileLogger("/ip5356_write.log");
    }
}

IP5356* IP5356::_ip5356 = nullptr;

IP5356* IP5356::getInstance() {
    if (!_ip5356) {
        _ip5356 = new IP5356;
    }
    return _ip5356;
}

void IP5356::begin() {
    Wire.begin();
}

void IP5356::_read(uint8_t addr, uint8_t reg, uint8_t* data) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, (uint8_t)1);
    if (Wire.available()) {
        *data = Wire.read();
    }
}

void IP5356::_read(uint8_t addr, uint8_t reg, uint16_t* data) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, (uint8_t)2);
    if (Wire.available()) {
        data[0] = Wire.read();
        data[1] = Wire.read();
    }
}

void IP5356::_write(uint8_t addr, uint8_t reg, uint8_t* data) {
    if(_log->getIsError()){
        app_log("FileLogger error, no write to ip5356");
        return;
    }

    uint8_t oldData = 0;
    _read(addr, reg, &oldData);
    String logStr = "write [0x" + String(addr, HEX) + ":0x" + String(reg, HEX)+"]: " + String(oldData) + " -> " + String(*data);

    _log->log(Logger::NO_LEVEL, logStr);
    if (_log->getIsError()) {
        app_log("FileLogger error, no write to ip5356");
        return;
    }

    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(*data);
    Wire.endTransmission();
    app_log(logStr);
}

void IP5356::_read(IP5356_0X74_REG reg, uint8_t* data) {
    _read(0x74, static_cast<uint8_t>(reg), data);
}

void IP5356::_read(IP5356_0X74_REG reg, uint16_t* data) {
    _read(0x74, static_cast<uint8_t>(reg), data);
}

void IP5356::_read(IP5356_0X75_REG reg, uint8_t* data) {
    _read(0x75, static_cast<uint8_t>(reg), data);
}

void IP5356::_read(IP5356_0X75_REG reg, uint16_t* data) {
    _read(0x75, static_cast<uint8_t>(reg), data);
}

void IP5356::_write(IP5356_0X74_REG reg, uint8_t* data) {
    _write(0x74, static_cast<uint8_t>(reg), data);
}

void IP5356::_write(IP5356_0X75_REG reg, uint8_t* data) {
    _write(0x75, static_cast<uint8_t>(reg), data);
}

uint8_t IP5356::getPercentage() {
    uint8_t value = 255;
    _read(SOC_CAP_DATA, &value);
    return value;
}

