#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <vector>
#include <Log.h>
#include <functional>
#include "Globals.h"
#include "Screen.h"

class Menu;

// Base MenuItem class
class MenuItem {
protected:
    String _title;
    bool _enabled;

public:
    MenuItem(String title, bool enabled = true) : _title(title), _enabled(enabled) {}

    virtual uint8_t draw(bool selected, uint8_t posY = 0) {
        uint16_t bgColor = selected ? TFT_CYAN : TFT_BLACK;
        uint16_t textColor = selected ? TFT_BLACK : (_enabled ? TFT_WHITE : TFT_DARKGREY);

        if (selected) {
            img->fillRect(0, posY - 3, 160, 13, bgColor);
        }

        img->setTextColor(textColor);
        img->drawString(_title, 10, posY);

        return posY + 13;
    }

    virtual void onSelect() = 0;
    virtual void onLeft() {}
    virtual void onRight() {}
    virtual void onLongPress() {}

    String getLabel() const { return _title; }
    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    virtual Menu* asSubmenu() { return nullptr; }
    virtual bool asBackMenuItem() { return false; }
    virtual bool hasValue() const { return false; }
    virtual String getValue() const { return ""; }

    virtual ~MenuItem() = default;
};

// Action MenuItem
class ActionMenuItem : public MenuItem {
private:
    std::function<void()> _action;

public:
    ActionMenuItem(String title, std::function<void()> action, bool enabled = true)
        : MenuItem(title, enabled), _action(action) {
    }

    void onSelect() override {
        if (_enabled && _action) _action();
    }
};

// Back MenuItem (special action to go back to parent menu)
class BackMenuItem : public MenuItem {
private:
    Menu* _targetMenu;

public:
    BackMenuItem(String title = "Back") : MenuItem(title), _targetMenu(nullptr) {}

    void onSelect() override {
        // Selection will be handled by MenuManager
    }

    void setTargetMenu(Menu* menu) {
        _targetMenu = menu;
    }

    Menu* getTargetMenu() const {
        return _targetMenu;
    }

    bool asBackMenuItem() override {
        return true;
    }
};

// Submenu MenuItem
class SubmenuMenuItem : public MenuItem {
private:
    Menu* _submenu;

public:
    SubmenuMenuItem(String title, Menu* submenu, bool enabled = true)
        : MenuItem(title, enabled), _submenu(submenu) {
    }

    void onSelect() override {
        // Selection is handled by MenuManager
    }

    Menu* asSubmenu() override {
        return _enabled ? _submenu : nullptr;
    }
};

// Value MenuItem (displays a value, can be editable)
class ValueMenuItem : public MenuItem {
protected:
    String _value;
    bool _editable;

public:
    ValueMenuItem(String title, String value, bool editable = false, bool enabled = true)
        : MenuItem(title, enabled), _value(value), _editable(editable) {
    }

    uint8_t draw(bool selected, uint8_t posY = 0) override {
        uint16_t bgColor = selected ? TFT_CYAN : TFT_BLACK;
        uint16_t textColor = selected ? TFT_BLACK : (_enabled ? TFT_WHITE : TFT_DARKGREY);

        if (selected) {
            img->fillRect(0, posY - 3, 160, 13, bgColor);
        }

        img->setTextColor(textColor);
        img->drawString(_title, 10, posY);
        img->drawString(_value, 160 - _value.length() * 6 - 10, posY);

        return posY + 13;
    }

    void onSelect() override {
        // Override in derived classes
    }

    bool hasValue() const override { return true; }
    String getValue() const override { return _value; }
    void setValue(String value) { _value = value; }
    bool isEditable() const { return _editable; }
};

// Selectable MenuItem for single choice from options
class SelectableMenuItem : public ValueMenuItem {
private:
    std::vector<String> _options;
    int _selectedIndex;
    int _eepromAddress;
    std::function<void(int)> _onValueChanged;
    std::function<int(void)> _loadValue;

public:
    SelectableMenuItem(String title, std::vector<String> options, int defaultIndex = 0,
        int eepromAddress = -1, std::function<void(int)> onChanged = nullptr)
        : ValueMenuItem(title, options.empty() ? "" : options[defaultIndex], true),
        _options(options), _selectedIndex(defaultIndex), _eepromAddress(eepromAddress),
        _onValueChanged(onChanged) {

        loadFromEEPROM();
        updateDisplayValue();
    }

    SelectableMenuItem(String title, std::vector<String> options, int defaultIndex = 0,
        std::function<int(void)> loadValue = nullptr, std::function<void(int)> onChanged = nullptr)
        : ValueMenuItem(title, options.empty() ? "" : options[defaultIndex], true),
        _options(options), _selectedIndex(defaultIndex), _loadValue(loadValue),
        _onValueChanged(onChanged) {
        _eepromAddress = -1;
        loadFromEEPROM();
        updateDisplayValue();
    }

    uint8_t draw(bool selected, uint8_t posY = 0) override {
        uint16_t bgColor = selected ? TFT_CYAN : TFT_BLACK;
        uint16_t textColor = selected ? TFT_BLACK : (_enabled ? TFT_WHITE : TFT_DARKGREY);

        if (selected) {
            img->fillRect(0, posY - 3, 160, 13, bgColor);
        }

        img->setTextColor(textColor);
        img->drawString(_title, 10, posY);

        img->drawString(_value, 160 - _value.length() * 6 - 10, posY);

        return posY + 13;
    }

    void onSelect() override {
        // Cycle through options
        _selectedIndex = (_selectedIndex + 1) % _options.size();
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_selectedIndex);
        }
    }

    void onLeft() override {
        _selectedIndex = (_selectedIndex - 1 + _options.size()) % _options.size();
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_selectedIndex);
        }
    }

    void onRight() override {
        _selectedIndex = (_selectedIndex + 1) % _options.size();
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_selectedIndex);
        }
    }

    int getSelectedIndex() const { return _selectedIndex; }
    String getSelectedOption() const {
        return (_selectedIndex >= 0 && _selectedIndex < _options.size()) ? _options[_selectedIndex] : "";
    }

private:
    void updateDisplayValue() {
        if (_selectedIndex >= 0 && _selectedIndex < _options.size()) {
            _value = _options[_selectedIndex];
        }
    }

    void saveToEEPROM() {
        if (_eepromAddress >= 0) {
            eeprom->write(_eepromAddress, _selectedIndex);
        }
    }

    void loadFromEEPROM() {
        if (_eepromAddress >= 0) {
            int savedIndex = eeprom->read(_eepromAddress);
            if (savedIndex >= 0 && savedIndex < _options.size()) {
                _selectedIndex = savedIndex;
            }
        }
        else if (_loadValue) {
            int savedIndex = _loadValue();
            if (savedIndex >= 0 && savedIndex < _options.size()) {
                _selectedIndex = savedIndex;
            }
        }
    }
};

// Toggle MenuItem (on/off switch)
class ToggleMenuItem : public ValueMenuItem {
private:
    bool _state;
    int _eepromAddress;
    std::function<void(bool)> _onValueChanged;
    std::function<bool(void)> _loadValue;

public:
    ToggleMenuItem(String title, bool defaultState = false, int eepromAddress = -1,
        std::function<void(bool)> onChanged = nullptr)
        : ValueMenuItem(title, defaultState ? "ON" : "OFF", true),
        _state(defaultState), _eepromAddress(eepromAddress), _onValueChanged(onChanged) {

        loadFromEEPROM();
        updateDisplayValue();
    }

    ToggleMenuItem(String title, bool defaultState = false,
        std::function<bool(void)> loadValue = nullptr, std::function<void(int)> onChanged = nullptr)
        : ValueMenuItem(title, defaultState ? "ON" : "OFF", true),
        _state(defaultState), _eepromAddress(-1), _onValueChanged(onChanged), _loadValue(loadValue) {

        loadFromEEPROM();
        updateDisplayValue();
    }

    void onSelect() override {
        _state = !_state;
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_state);
        }
    }

    uint8_t draw(bool selected, uint8_t posY = 0) override {
        loadFromEEPROM();
        updateDisplayValue();
        return ValueMenuItem::draw(selected, posY);
    }

    void onLeft() override { onSelect(); }
    void onRight() override { onSelect(); }

    bool getState() const { return _state; }
    void setState(bool state) {
        _state = state;
        updateDisplayValue();
        saveToEEPROM();
    }

private:
    void updateDisplayValue() {
        _value = _state ? "ON" : "OFF";
    }

    void saveToEEPROM() {
        if (_eepromAddress >= 0) {
            eeprom->write(_eepromAddress, _state ? 1 : 0);
        }
    }

    void loadFromEEPROM() {
        if (_eepromAddress >= 0) {
            _state = eeprom->read(_eepromAddress) != 0;
        }
        else if (_loadValue){
            _state = _loadValue();
        }
    }
};

// Numeric MenuItem (for numeric values with increment/decrement)
class NumericMenuItem : public ValueMenuItem {
private:
    int _value_int;
    int _minValue, _maxValue, _step;
    int _eepromAddress;
    std::function<void(int)> _onValueChanged;

public:
    NumericMenuItem(String title, int defaultValue, int minVal, int maxVal, int step = 1,
        int eepromAddress = -1, std::function<void(int)> onChanged = nullptr)
        : ValueMenuItem(title, String(defaultValue), true),
        _value_int(defaultValue), _minValue(minVal), _maxValue(maxVal), _step(step),
        _eepromAddress(eepromAddress), _onValueChanged(onChanged) {

        loadFromEEPROM();
        updateDisplayValue();
    }

    void onSelect() override {
        // Could toggle editing mode or cycle through predefined values
    }

    void onLeft() override {
        _value_int = max(_minValue, _value_int - _step);
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_value_int);
        }
    }

    void onRight() override {
        _value_int = min(_maxValue, _value_int + _step);
        updateDisplayValue();
        saveToEEPROM();

        if (_onValueChanged) {
            _onValueChanged(_value_int);
        }
    }

    int getNumericValue() const { return _value_int; }

private:
    void updateDisplayValue() {
        _value = String(_value_int);
    }

    void saveToEEPROM() {
        if (_eepromAddress >= 0) {
            eeprom->write(_eepromAddress, &_value_int, sizeof(_value_int));
        }
    }

    void loadFromEEPROM() {
        if (_eepromAddress >= 0) {
            eeprom->read(_eepromAddress, &_value_int, sizeof(_value_int));
            _value_int = constrain(_value_int, _minValue, _maxValue);
        }
    }
};

class VideoPlayMenuItem : public MenuItem {
private:

public:



};

// Menu class
class Menu {
private:
    String _title;
    std::vector<MenuItem*> _menuItems;
    uint8_t _selectedIndex;
    int _scrollOffset;
    Menu* _parentMenu;
    const uint8_t _visibleMenuItems = 5;

public:
    Menu(String title, Menu* parent = nullptr) :
        _title(title), _selectedIndex(0), _scrollOffset(0), _parentMenu(parent) {
    }

    ~Menu() {
        for (auto item : _menuItems) {
            delete item;
        }
    }

    void addMenuItem(MenuItem* item) {
        _menuItems.push_back(item);
    }

    MenuItem* getCurrentMenuItem() const {
        if (_selectedIndex < _menuItems.size()) {
            return _menuItems[_selectedIndex];
        }
        return nullptr;
    }

    void navigateUp() {
        if (_selectedIndex == 0) {
            _selectedIndex = _menuItems.size() - 1;
        }
        else {
            _selectedIndex--;
        }
        updateScrollOffset();
    }

    void navigateDown() {
        if (_selectedIndex == _menuItems.size() - 1) {
            _selectedIndex = 0;
        }
        else {
            _selectedIndex++;
        }
        updateScrollOffset();
    }

    void updateScrollOffset() {
        if (_selectedIndex < _scrollOffset) {
            _scrollOffset = _selectedIndex;
        }
        else if (_selectedIndex >= _scrollOffset + _visibleMenuItems) {
            _scrollOffset = _selectedIndex - _visibleMenuItems + 1;
        }
    }

    Menu* selectCurrentMenuItem() {
        MenuItem* item = getCurrentMenuItem();
        if (item && item->isEnabled()) {
            if (item->asBackMenuItem()) {
                return ((BackMenuItem*)item)->getTargetMenu();
            }

            item->onSelect();
            Menu* submenu = item->asSubmenu();
            if (submenu) {
                submenu->setParent(this);
                return submenu;
            }
        }
        return nullptr;
    }

    void handleLeft() {
        MenuItem* item = getCurrentMenuItem();
        if (item && item->isEnabled()) {
            item->onLeft();
        }
    }

    void handleRight() {
        MenuItem* item = getCurrentMenuItem();
        if (item && item->isEnabled()) {
            item->onRight();
        }
    }

    void draw() {
        img->fillSprite(TFT_BLACK);
        img->setFreeFont();
        img->setTextColor(TFT_WHITE);
        img->setTextSize(1);

        // Draw title
        img->setTextColor(TFT_YELLOW);
        img->drawString(_title, 5, 0);
        img->drawLine(0, 12, 160, 12, TFT_WHITE);

        uint8_t startIndex = _scrollOffset;
        uint8_t endIndex = min((int)_menuItems.size(), startIndex + _visibleMenuItems);
        uint8_t j = 18;

        for (int i = startIndex; i < endIndex; i++) {
            j = _menuItems[i]->draw(i == _selectedIndex, j);
        }

        img->pushSprite(0, 0);
    }

    Menu* getParent() const { return _parentMenu; }
    void setParent(Menu* parent) { _parentMenu = parent; }
    String getTitle() const { return _title; }
    size_t getItemCount() const { return _menuItems.size(); }
};


#endif // MENU_H
