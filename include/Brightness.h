#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include <Arduino.h>
#include <Eeprom24C.h>

class Brightness {
public:
    enum class Mode {
        OFF = 0,
        ON,
        LOW_BRIGHTNESS,
    };

private:

    uint8_t _pin;
    uint8_t _pwmChannel;
    uint8_t _percentCurrent = 0;
    Mode _mode;


public:
    Brightness(uint8_t pin, uint8_t pwmChannel);

    void setPercent(uint8_t percent);

    void on();

    void off();

    void lowBrightness();

    Mode getMode();

    uint8_t getPercent() const;

    ~Brightness();
};

#endif // BRIGHTNESS_H
