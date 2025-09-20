#include <Arduino.h>
#include "ButtonManager.h"
#include <Log.h>

ButtonManager* ButtonManager::_instance = nullptr;
uint64_t ButtonManager::_lastActionTime = millis();

void ButtonManager::handleClick() {
    _lastActionTime = millis();
    app_log("handleClick");
    if (_instance && _instance->_actionCallback) {
        _instance->_actionCallback(Action::CLICK);
    }
}

void ButtonManager::handleDoubleClick() {
    _lastActionTime = millis();
    app_log("handleDoubleClick");
    if (_instance && _instance->_actionCallback) {
        _instance->_actionCallback(Action::DOUBLE_CLICK);
    }
}

void ButtonManager::handleLongPressStart() {
    _lastActionTime = millis();
    app_log("handleLongPressStart");
    if (_instance && _instance->_actionCallback) {
        _instance->_actionCallback(Action::LONG_PRESS_START);
    }
}

void ButtonManager::handleLongPressStop() {
    _lastActionTime = millis();
    app_log("handleLongPressStop");
    if (_instance && _instance->_actionCallback) {
        _instance->_actionCallback(Action::LONG_PRESS_STOP);
    }
}

ButtonManager::ButtonManager(OneButton* button) : _button(button) {
    _instance = this;
}

void ButtonManager::setActionCallback(std::function<void(Action)> actionCallback) {
    _actionCallback = actionCallback;

    _button->attachClick(handleClick);
    _button->attachDoubleClick(handleDoubleClick);
    _button->attachLongPressStart(handleLongPressStart);
    // _button->attachLongPressStop(handleLongPressStop);
}

void ButtonManager::tick() {
    _button->tick();
}

uint64_t ButtonManager::getIdleTime() {
    return millis() - _lastActionTime;
}


