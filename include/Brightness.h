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
    Brightness(uint8_t pin, uint8_t pwmChannel) :
        _pin(pin), _pwmChannel(pwmChannel) {
        ledcSetup(_pwmChannel, 5000, 8);
        ledcAttachPin(_pin, _pwmChannel);
    }

    void setPercent(uint8_t percent) {
        if (percent > 100) percent = 100;
        if (percent != _percentCurrent) {
            _percentCurrent = percent;
            _mode = Mode::OFF;
            on();
        }
    }

    void on() {
        if(_mode == Mode::ON) return;
        _mode = Mode::ON;
        ledcWrite(_pwmChannel, map(_percentCurrent, 0, 100, 0, 255));
    }

    void off() {
        if (_mode == Mode::OFF) return;
        _mode = Mode::OFF;
        ledcWrite(_pwmChannel, 0);
    }

    void lowBrightness(){
        if (_mode == Mode::LOW_BRIGHTNESS) return;
        _mode = Mode::LOW_BRIGHTNESS;
        ledcWrite(_pwmChannel, 5);
    }

    Mode getMode(){
        return _mode;
    }

    uint8_t getPercent() const {
        return _percentCurrent;
    }

    ~Brightness() {
        ledcWrite(_pwmChannel, 0);
        ledcDetachPin(_pin);
    }
};

#endif // BRIGHTNESS_H
