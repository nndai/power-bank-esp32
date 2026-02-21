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
    MenuItem(String title, bool enabled = true);

    virtual uint8_t draw(bool selected, uint8_t posY = 0);

    virtual void onSelect() = 0;
    virtual void onLeft();
    virtual void onRight();
    virtual void onLongPress();

    String getLabel() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);

    virtual Menu* asSubmenu();
    virtual bool asBackMenuItem();
    virtual bool hasValue() const;
    virtual String getValue() const;
    /** Returns true if onSelect() renders its own full-screen content (e.g., chart). */
    virtual bool rendersScreen() const;

    virtual ~MenuItem() = default;
};

// Action MenuItem
class ActionMenuItem : public MenuItem {
private:
    std::function<void()> _action;

public:
    ActionMenuItem(String title, std::function<void()> action, bool enabled = true);

    void onSelect() override;
};

// Chart MenuItem — like ActionMenuItem but signals it renders full-screen content
class ChartMenuItem : public MenuItem {
private:
    std::function<void()> _action;

public:
    ChartMenuItem(String title, std::function<void()> action, bool enabled = true);

    void onSelect() override;
    bool rendersScreen() const override;
};

// Back MenuItem (special action to go back to parent menu)
class BackMenuItem : public MenuItem {
private:
    Menu* _targetMenu;

public:
    BackMenuItem(String title = "Back");

    void onSelect() override;

    void setTargetMenu(Menu* menu);

    Menu* getTargetMenu() const;

    bool asBackMenuItem() override;
};

// Submenu MenuItem
class SubmenuMenuItem : public MenuItem {
private:
    Menu* _submenu;

public:
    SubmenuMenuItem(String title, Menu* submenu, bool enabled = true);

    void onSelect() override;

    Menu* asSubmenu() override;
};

// Value MenuItem (displays a value, can be editable)
class ValueMenuItem : public MenuItem {
protected:
    String _value;
    bool _editable;

public:
    ValueMenuItem(String title, String value, bool editable = false, bool enabled = true);

    uint8_t draw(bool selected, uint8_t posY = 0) override;

    void onSelect() override;

    bool hasValue() const override;
    String getValue() const override;
    void setValue(String value);
    bool isEditable() const;
};

// Selectable MenuItem for single choice from options
class SelectableMenuItem : public ValueMenuItem {
private:
    std::vector<String> _options;
    int _selectedIndex;
    std::function<void(int)> _onValueChanged;
    std::function<int(void)> _loadValue;

public:
    SelectableMenuItem(String title, std::vector<String> options, int defaultIndex = 0,
        std::function<int(void)> loadValue = nullptr, std::function<void(int)> onChanged = nullptr);

    uint8_t draw(bool selected, uint8_t posY = 0) override;

    void onSelect() override;

    void onLeft() override;

    void onRight() override;

    int getSelectedIndex() const;
    String getSelectedOption() const;

private:
    void updateDisplayValue();

    void loadFromValue();
};

// Toggle MenuItem (on/off switch)
class ToggleMenuItem : public ValueMenuItem {
private:
    bool _state;
    std::function<void(bool)> _onValueChanged;
    std::function<bool(void)> _loadValue;

public:
    ToggleMenuItem(String title, bool defaultState = false,
        std::function<bool(void)> loadValue = nullptr, std::function<void(bool)> onChanged = nullptr);

    void onSelect() override;

    uint8_t draw(bool selected, uint8_t posY = 0) override;

    void onLeft() override;
    void onRight() override;

    bool getState() const;
    void setState(bool state);

private:
    void updateDisplayValue();

    void loadFromValue();
};

// Numeric MenuItem (for numeric values with increment/decrement)
class NumericMenuItem : public ValueMenuItem {
private:
    int _value_int;
    int _minValue, _maxValue, _step;
    std::function<void(int)> _onValueChanged;
    std::function<int(void)> _loadValue;

public:
    NumericMenuItem(String title, int defaultValue, int minVal, int maxVal, int step = 1,
        std::function<int(void)> loadValue = nullptr, std::function<void(int)> onChanged = nullptr);

    void onSelect() override;

    void onLeft() override;

    void onRight() override;

    int getNumericValue() const;

private:
    void updateDisplayValue();

    void loadFromValue();
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
    Menu(String title, Menu* parent = nullptr);

    ~Menu();

    void addMenuItem(MenuItem* item);

    MenuItem* getCurrentMenuItem() const;

    void navigateUp();

    void navigateDown();

    void updateScrollOffset();

    Menu* selectCurrentMenuItem();

    void handleLeft();

    void handleRight();

    void draw();

    Menu* getParent() const;
    void setParent(Menu* parent);
    String getTitle() const;
    size_t getItemCount() const;
};


#endif // MENU_H
