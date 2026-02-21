#include <Arduino.h>
#include "Menu.h"
#include "Globals.h"

MenuItem::MenuItem(String title, bool enabled) : _title(title), _enabled(enabled) {}

uint8_t MenuItem::draw(bool selected, uint8_t posY) {
    uint16_t bgColor = selected ? TFT_CYAN : TFT_BLACK;
    uint16_t textColor = selected ? TFT_BLACK : (_enabled ? TFT_WHITE : TFT_DARKGREY);

    if (selected) {
        img->fillRect(0, posY - 3, 160, 13, bgColor);
    }

    img->setTextColor(textColor);
    img->drawString(_title, 10, posY);

    return posY + 13;
}

void MenuItem::onLeft() {}
void MenuItem::onRight() {}
void MenuItem::onLongPress() {}

String MenuItem::getLabel() const { return _title; }
bool MenuItem::isEnabled() const { return _enabled; }
void MenuItem::setEnabled(bool enabled) { _enabled = enabled; }

Menu* MenuItem::asSubmenu() { return nullptr; }
bool MenuItem::asBackMenuItem() { return false; }
bool MenuItem::hasValue() const { return false; }
String MenuItem::getValue() const { return ""; }
bool MenuItem::rendersScreen() const { return false; }

ActionMenuItem::ActionMenuItem(String title, std::function<void()> action, bool enabled)
    : MenuItem(title, enabled), _action(action) {}

void ActionMenuItem::onSelect() {
    if (_enabled && _action) _action();
}

ChartMenuItem::ChartMenuItem(String title, std::function<void()> action, bool enabled)
    : MenuItem(title, enabled), _action(action) {}

void ChartMenuItem::onSelect() {
    if (_enabled && _action) _action();
}

bool ChartMenuItem::rendersScreen() const { return true; }

BackMenuItem::BackMenuItem(String title) : MenuItem(title), _targetMenu(nullptr) {}

void BackMenuItem::onSelect() {
}

void BackMenuItem::setTargetMenu(Menu* menu) {
    _targetMenu = menu;
}

Menu* BackMenuItem::getTargetMenu() const {
    return _targetMenu;
}

bool BackMenuItem::asBackMenuItem() {
    return true;
}

SubmenuMenuItem::SubmenuMenuItem(String title, Menu* submenu, bool enabled)
    : MenuItem(title, enabled), _submenu(submenu) {}

void SubmenuMenuItem::onSelect() {
}

Menu* SubmenuMenuItem::asSubmenu() {
    return _enabled ? _submenu : nullptr;
}

ValueMenuItem::ValueMenuItem(String title, String value, bool editable, bool enabled)
    : MenuItem(title, enabled), _value(value), _editable(editable) {}

uint8_t ValueMenuItem::draw(bool selected, uint8_t posY) {
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

void ValueMenuItem::onSelect() {
}

bool ValueMenuItem::hasValue() const { return true; }
String ValueMenuItem::getValue() const { return _value; }
void ValueMenuItem::setValue(String value) { _value = value; }
bool ValueMenuItem::isEditable() const { return _editable; }

SelectableMenuItem::SelectableMenuItem(String title, std::vector<String> options, int defaultIndex,
    std::function<int(void)> loadValue, std::function<void(int)> onChanged)
    : ValueMenuItem(title, options.empty() ? "" : options[defaultIndex], true),
      _options(options), _selectedIndex(defaultIndex), _onValueChanged(onChanged), _loadValue(loadValue) {
    loadFromValue();
    updateDisplayValue();
}

uint8_t SelectableMenuItem::draw(bool selected, uint8_t posY) {
    loadFromValue();
    updateDisplayValue();
    return ValueMenuItem::draw(selected, posY);
}

void SelectableMenuItem::onSelect() {
    _selectedIndex = (_selectedIndex + 1) % _options.size();
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_selectedIndex);
    }
}

void SelectableMenuItem::onLeft() {
    _selectedIndex = (_selectedIndex - 1 + _options.size()) % _options.size();
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_selectedIndex);
    }
}

void SelectableMenuItem::onRight() {
    _selectedIndex = (_selectedIndex + 1) % _options.size();
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_selectedIndex);
    }
}

int SelectableMenuItem::getSelectedIndex() const { return _selectedIndex; }
String SelectableMenuItem::getSelectedOption() const {
    return (_selectedIndex >= 0 && _selectedIndex < (int)_options.size()) ? _options[_selectedIndex] : "";
}

void SelectableMenuItem::updateDisplayValue() {
    if (_selectedIndex >= 0 && _selectedIndex < (int)_options.size()) {
        _value = _options[_selectedIndex];
    }
}

void SelectableMenuItem::loadFromValue() {
    if (_loadValue) {
        int savedIndex = _loadValue();
        if (savedIndex >= 0 && savedIndex < (int)_options.size()) {
            _selectedIndex = savedIndex;
        }
    }
}

ToggleMenuItem::ToggleMenuItem(String title, bool defaultState,
    std::function<bool(void)> loadValue, std::function<void(bool)> onChanged)
    : ValueMenuItem(title, defaultState ? "ON" : "OFF", true),
      _state(defaultState), _onValueChanged(onChanged), _loadValue(loadValue) {
    loadFromValue();
    updateDisplayValue();
}

void ToggleMenuItem::onSelect() {
    _state = !_state;
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_state);
    }
}

uint8_t ToggleMenuItem::draw(bool selected, uint8_t posY) {
    loadFromValue();
    updateDisplayValue();
    return ValueMenuItem::draw(selected, posY);
}

void ToggleMenuItem::onLeft() { onSelect(); }
void ToggleMenuItem::onRight() { onSelect(); }

bool ToggleMenuItem::getState() const { return _state; }
void ToggleMenuItem::setState(bool state) {
    _state = state;
    updateDisplayValue();
}

void ToggleMenuItem::updateDisplayValue() {
    _value = _state ? "ON" : "OFF";
}

void ToggleMenuItem::loadFromValue() {
    if (_loadValue) {
        _state = _loadValue();
    }
}

NumericMenuItem::NumericMenuItem(String title, int defaultValue, int minVal, int maxVal, int step,
    std::function<int(void)> loadValue, std::function<void(int)> onChanged)
    : ValueMenuItem(title, String(defaultValue), true),
      _value_int(defaultValue), _minValue(minVal), _maxValue(maxVal), _step(step),
      _onValueChanged(onChanged), _loadValue(loadValue) {
    loadFromValue();
    updateDisplayValue();
}

void NumericMenuItem::onSelect() {
}

void NumericMenuItem::onLeft() {
    _value_int = max(_minValue, _value_int - _step);
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_value_int);
    }
}

void NumericMenuItem::onRight() {
    _value_int = min(_maxValue, _value_int + _step);
    updateDisplayValue();

    if (_onValueChanged) {
        _onValueChanged(_value_int);
    }
}

int NumericMenuItem::getNumericValue() const { return _value_int; }

void NumericMenuItem::updateDisplayValue() {
    _value = String(_value_int);
}

void NumericMenuItem::loadFromValue() {
    if (_loadValue) {
        int savedValue = _loadValue();
        _value_int = constrain(savedValue, _minValue, _maxValue);
    }
}

Menu::Menu(String title, Menu* parent) :
    _title(title), _selectedIndex(0), _scrollOffset(0), _parentMenu(parent) {}

Menu::~Menu() {
    for (auto item : _menuItems) {
        delete item;
    }
}

void Menu::addMenuItem(MenuItem* item) {
    _menuItems.push_back(item);
}

MenuItem* Menu::getCurrentMenuItem() const {
    if (_selectedIndex < _menuItems.size()) {
        return _menuItems[_selectedIndex];
    }
    return nullptr;
}

void Menu::navigateUp() {
    if (_selectedIndex == 0) {
        _selectedIndex = _menuItems.size() - 1;
    }
    else {
        _selectedIndex--;
    }
    updateScrollOffset();
}

void Menu::navigateDown() {
    if (_selectedIndex == _menuItems.size() - 1) {
        _selectedIndex = 0;
    }
    else {
        _selectedIndex++;
    }
    updateScrollOffset();
}

void Menu::updateScrollOffset() {
    if (_selectedIndex < _scrollOffset) {
        _scrollOffset = _selectedIndex;
    }
    else if (_selectedIndex >= _scrollOffset + _visibleMenuItems) {
        _scrollOffset = _selectedIndex - _visibleMenuItems + 1;
    }
}

Menu* Menu::selectCurrentMenuItem() {
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

void Menu::handleLeft() {
    MenuItem* item = getCurrentMenuItem();
    if (item && item->isEnabled()) {
        item->onLeft();
    }
}

void Menu::handleRight() {
    MenuItem* item = getCurrentMenuItem();
    if (item && item->isEnabled()) {
        item->onRight();
    }
}

void Menu::draw() {
    img->fillSprite(TFT_BLACK);
    img->setFreeFont();
    img->setTextColor(TFT_WHITE);
    img->setTextSize(1);

    img->setTextColor(TFT_YELLOW);
    img->drawString(_title, 5, 2);
    img->drawLine(0, 12, 160, 12, TFT_WHITE);

    uint8_t startIndex = _scrollOffset;
    uint8_t endIndex = min((int)_menuItems.size(), startIndex + _visibleMenuItems);
    uint8_t j = 18;

    for (int i = startIndex; i < (int)endIndex; i++) {
        j = _menuItems[i]->draw(i == _selectedIndex, j);
    }

    img->pushSprite(0, 0);
}

Menu* Menu::getParent() const { return _parentMenu; }
void Menu::setParent(Menu* parent) { _parentMenu = parent; }
String Menu::getTitle() const { return _title; }
size_t Menu::getItemCount() const { return _menuItems.size(); }


