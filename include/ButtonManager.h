#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <OneButton.h>

/**
 * @brief Enum representing different button actions.
 */
enum class Action {
    NONE,
    CLICK,
    DOUBLE_CLICK,
    LONG_PRESS_START,
    LONG_PRESS_STOP,
    LEFT,
    RIGHT,
};

/**
 * @brief Class to manage button actions using the OneButton library.
 * @note This class only supports a single button instance.
 */
class ButtonManager {
private:
    OneButton* _button;
    static ButtonManager* _instance;
    std::function<void(Action)> _actionCallback = nullptr;
    static uint64_t _lastActionTime;

    static void handleClick() {
        _lastActionTime = millis();
        app_log("handleClick");
        if (_instance && _instance->_actionCallback) {
            _instance->_actionCallback(Action::CLICK);
        }
    }

    static void handleDoubleClick() {
        _lastActionTime = millis();
        app_log("handleDoubleClick");
        if (_instance && _instance->_actionCallback) {
            _instance->_actionCallback(Action::DOUBLE_CLICK);
        }
    }

    static void handleLongPressStart() {
        _lastActionTime = millis();
        app_log("handleLongPressStart");
        if (_instance && _instance->_actionCallback) {
            _instance->_actionCallback(Action::LONG_PRESS_START);
        }
    }

    static void handleLongPressStop() {
        _lastActionTime = millis();
        app_log("handleLongPressStop");
        if (_instance && _instance->_actionCallback) {
            _instance->_actionCallback(Action::LONG_PRESS_STOP);
        }
    }

public:
    ButtonManager(OneButton* button) : _button(button) {
        _instance = this;
    }

    /**
     * @brief Set the callback function to handle button actions.
     * @param actionCallback The callback function to be called on button actions.
     */
    void setActionCallback(std::function<void(Action)> actionCallback) {
        _actionCallback = actionCallback;

        _button->attachClick(handleClick);
        _button->attachDoubleClick(handleDoubleClick);
        _button->attachLongPressStart(handleLongPressStart);
        //_button->attachLongPressStop(handleLongPressStop);
    }

    /**
     * @brief Call this method periodically to update the button state.
     */
    void tick() {
        _button->tick();
    }

    uint64_t getIdleTime() {
        return millis() - _lastActionTime;
    }
};

ButtonManager* ButtonManager::_instance = nullptr;
uint64_t ButtonManager::_lastActionTime = millis();

#endif // BUTTON_MANAGER_H
