#include "IP5356.h"


const uint8_t IP5356::CheckReg0x74Arr[256] = {
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

const uint8_t IP5356::CheckReg0x75Arr[256] = {
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

bool IP5356::_read(uint8_t addr, uint8_t reg, uint8_t* data) {
    if (!checkRegRead(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not read");
        return false;
    }
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(addr, (uint8_t)1);
    if (Wire.available()) {
        *data = Wire.read();
        return true;
    }
    return false;
}

bool IP5356::_read(uint8_t addr, uint8_t reg, uint16_t* data) {
    if (!checkRegRead(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not read");
        return false;
    }
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(addr, (uint8_t)2);
    if (Wire.available() >= 2) {
        *data = Wire.read() | Wire.read() << 8;
        return true;
    }
    return false;
}

bool IP5356::_write(uint8_t addr, uint8_t reg, uint8_t* data) {
    if (!checkRegWrite(addr, reg)) {
        app_log_e("[0x" + String(addr, HEX) + "][0x" + String(reg, HEX) + "]: can not write");
        return false;
    }

    if (_log->getIsError()) {
        app_log("FileLogger error, no write to ip5356");
        return false;
    }

    uint8_t oldData = 0;
    if(!_read(addr, reg, &oldData)){
        app_log("read IP5356 error, no write to ip5356");
        return false;
    }
    String logStr = "write [0x" + String(addr, HEX) + ":0x" + String(reg, HEX) + "]: " + String(oldData) + " -> " + String(*data);

    _log->log(Logger::NO_LEVEL, logStr, "");
    if (_log->getIsError()) {
        app_log("FileLogger error, no write to ip5356");
        return false;
    }

    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(*data);
    if (Wire.endTransmission()) {
        _log->log(Logger::NO_LEVEL, " --> error");
        return false;
    }

    _log->log(Logger::NO_LEVEL, " --> success");
    app_log(logStr);
    return true;
}

bool IP5356::checkRegRead(uint8_t addr, uint8_t reg) {
    if (addr == 0x74) {
        return CheckReg0x74Arr[reg] != 0;
    }
    if (addr == 0x75) {
        return CheckReg0x75Arr[reg] != 0;
    }
    return false;
}
bool IP5356::checkRegWrite(uint8_t addr, uint8_t reg) {
    if (addr == 0x74) {
        return CheckReg0x74Arr[reg] == 2;
    }
    if (addr == 0x75) {
        return CheckReg0x75Arr[reg] == 2;
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

uint8_t IP5356::getBatteryPercentage() {
    uint8_t value = 0xFF;
    _read(SOC_CAP_DATA, &value);
    return value;
}

float IP5356::getBatteryVoltage() {
    uint16_t value = -1;
    _read(BATVADC_DAT0, &value);
    return value * 0.26855f;
}

float IP5356::getBatteryCurrent() {
    uint16_t value = -1;
    _read(IBATIADC_DAT0, &value);
    return 1.6785 * value;
}

float IP5356::getBatteryTemperature() {
    uint16_t value = -1;
    _read(NTCVADC_DAT0, &value);
    float vNTC = 0.26855f * value / 1000.0f;
    float nowRNTC = vNTC / iNTC;
    float tempK = 3435.0f / (log(nowRNTC / rNTC) + (3435.0f / 298.15f));
    return  tempK - 273.15f;
}

float IP5356::getSystemVoltage() {
    uint16_t value = -1;
    _read(VSYSVADC_DAT0, &value);
    return  1.611328f * value;
}

float IP5356::getSystemCurrent() {
    uint16_t value = -1;
    _read(ISYS_IADC_DAT0, &value);
    return  0.671387f * value;
}

float IP5356::getSystemPower() {
    uint16_t value = -1;
    _read(VSYS_POW_DAT0, &value);
    return  4.431 * value;
}

float IP5356::getUSB1Voltage() {
    uint16_t value = -1;
    _read(VOUT1VADC_DAT0, &value);
    return  1.611328f * value;
}

float IP5356::getUSB1Current() {
    uint16_t value = -1;
    _read(IVOUT1_IADC_DAT0, &value);
    return  0.671387f * value;
}

float IP5356::getUSB2Voltage() {
    uint16_t value = -1;
    _read(VOUT2VADC_DAT0, &value);
    return  1.611328f * value;
}

float IP5356::getUSB2Current() {
    uint16_t value = -1;
    _read(IVOUT2_IADC_DAT0, &value);
    return  0.671387f * value;
}

float IP5356::getVBUSVoltage() {
    uint16_t value = -1;
    _read(VBUSVADC_DAT0, &value);
    return  1.611328f * value;
}

float IP5356::getVBUSCurrent() {
    uint16_t value = -1;
    if (getVBUSPortStatus() == PORT_STATE::SOURCE) {
        _read(IBUS_IADC_DAT0, &value);
    }
    else{
        _read(IVIN_IVBUS_IADC_DAT0, &value);
    }
    
    return  0.671387f * value;
}

#ifdef USING_VIN
float IP5356::getVINVoltage() {
    uint16_t value = -1;
    _read(VINVADC_DAT0, &value);
    return  1.611328f * value;
}

float IP5356::getVINCurrent() {
    uint16_t value = -1;
    _read(IVIN_IVBUS_IADC_DAT0, &value);
    return  0.671387f * value;
}
#endif

IP5356::NTC_STATE_FLAG IP5356::getNtcState() {
    uint8_t value = 0;
    _read(NTC_STATE, &value);
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

void IP5356::powerOff() {
    uint8_t value = 0xff;
    if (!_read(FORCE_STANDBY, &value)) return;
    value |= 0b10000000;
    _write(FORCE_STANDBY, &value);
}

IP5356::FAST_CHARGE_FROTOCOL IP5356::convertValueToProtocol(uint8_t value) {
    switch (value) {
    case 0b0110:  return FAST_CHARGE_FROTOCOL::QC_HANDSHAKE_OK;
    case 0b1001:  return FAST_CHARGE_FROTOCOL::QC2;
    case 0b1010:  return FAST_CHARGE_FROTOCOL::QC3;
    case 0b1100:  return FAST_CHARGE_FROTOCOL::FCP_SCP;
    case 0b1101:  return FAST_CHARGE_FROTOCOL::AFC;
    case 0b1110:  return FAST_CHARGE_FROTOCOL::SFCP;
    case 0b0001:  return FAST_CHARGE_FROTOCOL::DCP;
    default:     return FAST_CHARGE_FROTOCOL::NONE;
    }
}

IP5356::FAST_CHARGE_FROTOCOL IP5356::getVBUSSourceFrotocol() {
    uint8_t value = 0;
    if(!_read(STATUS_SRC1, &value)){
        return FAST_CHARGE_FROTOCOL::NONE;
    }
    value &= 0b1111;

    FAST_CHARGE_FROTOCOL proto = convertValueToProtocol(value);
    if (proto != FAST_CHARGE_FROTOCOL::NONE) {
        return proto;
    }

    value = 0;
    _read(PD_STATE1, &value);
    value &= 0b10000000;
    if (value) {
        return FAST_CHARGE_FROTOCOL::PD;
    }

    value = 0;
    _read(PD_STATE2, &value);
    value &= 0b1;
    if (value) {
        return FAST_CHARGE_FROTOCOL::PPS;
    }
    return FAST_CHARGE_FROTOCOL::NONE;
}

IP5356::FAST_CHARGE_FROTOCOL IP5356::getUSB1SourceFrotocol() {
    uint8_t value = 0;
    _read(STATUS_SRC0, &value);
    value &= 0b1111;
    return convertValueToProtocol(value);
}

IP5356::FAST_CHARGE_FROTOCOL IP5356::getUSB2SourceFrotocol() {
    uint8_t value = 0;
    _read(STATUS_SRC0, &value);
    value &= 0b11110000;
    value >>= 4;
    return convertValueToProtocol(value);
}

IP5356::PORT_STATE IP5356::getVBUSPortStatus() {
    uint8_t value = 0;
    _read(TYPEC_STATE, &value);
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
    _read(STATUS_SRC1, &value);
    value &= 0b01110000;
    if (value == 0b00100000) {
        return PORT_STATE::SINK;
    }
    return PORT_STATE::NONE;
}

IP5356::CHARGE_STATUS IP5356::getChargeStatus() {
    uint8_t value = 0;
    _read(CHG_STATE2, &value);
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

