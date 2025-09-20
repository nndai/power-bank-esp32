#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <OneButton.h>
#include <functional>

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

    static void handleClick();

    static void handleDoubleClick();

    static void handleLongPressStart();

    static void handleLongPressStop();

public:
    ButtonManager(OneButton* button);

    /**
     * @brief Set the callback function to handle button actions.
     * @param actionCallback The callback function to be called on button actions.
     */
    void setActionCallback(std::function<void(Action)> actionCallback);

    /**
     * @brief Call this method periodically to update the button state.
     */
    void tick();

    uint64_t getIdleTime();
};

#endif // BUTTON_MANAGER_H
