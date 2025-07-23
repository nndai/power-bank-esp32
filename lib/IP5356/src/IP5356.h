#ifndef IP5356_H
#define IP5356_H

#include <Wire.h>

class IP5356 {
private:
    uint8_t _address;

    
public:
    IP5356(uint8_t address = 0x34) : _address(address) {}

    void begin() {
        Wire.begin();
    }

    uint8_t readRegister(uint8_t reg) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.endTransmission();
        Wire.requestFrom(_address, (uint8_t)1);
        return Wire.read();
    }

    void writeRegister(uint8_t reg, uint8_t value) {
        Wire.beginTransmission(_address);
        Wire.write(reg);
        Wire.write(value);
        Wire.endTransmission();
    }

};




#endif // IP5356_H