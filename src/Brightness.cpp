#include <Arduino.h>
#include "Brightness.h"

Brightness::Brightness(uint8_t pin, uint8_t pwmChannel) :
    _pin(pin), _pwmChannel(pwmChannel) {
    ledcSetup(_pwmChannel, 5000, 8);
    ledcAttachPin(_pin, _pwmChannel);
}

void Brightness::setPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    if (percent != _percentCurrent) {
        _percentCurrent = percent;
        _mode = Mode::OFF;
        on();
    }
}

void Brightness::on() {
    if(_mode == Mode::ON) return;
    _mode = Mode::ON;
    ledcWrite(_pwmChannel, map(_percentCurrent, 0, 100, 0, 255));
}

void Brightness::off() {
    if (_mode == Mode::OFF) return;
    _mode = Mode::OFF;
    ledcWrite(_pwmChannel, 0);
}

void Brightness::lowBrightness(){
    if (_mode == Mode::LOW_BRIGHTNESS) return;
    _mode = Mode::LOW_BRIGHTNESS;
    ledcWrite(_pwmChannel, 5);
}

Brightness::Mode Brightness::getMode(){
    return _mode;
}

uint8_t Brightness::getPercent() const {
    return _percentCurrent;
}

Brightness::~Brightness() {
    ledcWrite(_pwmChannel, 0);
    ledcDetachPin(_pin);
}


