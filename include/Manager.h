#ifndef MANAGER_H
#define MANAGER_H

#include "Screen.h"
#include "Menu.h"

// Forward declaration
class ScreenManager;

// Enhanced MenuManager
class MenuManager {
private:
    Menu* _current;
    Menu* _rootMenu;
    ScreenManager* _screenManager;

public:
    MenuManager();
    ~MenuManager();

    void setScreenManager(ScreenManager* sm);

    void setCurrentMenu(Menu* menu);
    Menu* currentMenu() const;
    Menu* getRootMenu();

    void draw();
    void handleAction(Action action);

private:
    void setupMenus();
};

// ScreenManager
class ScreenManager {
private:
    Screen* _screen[2];
    uint8_t _screenIndex;
    MenuManager* _menuManager;
    bool _menuActive;

public:
    ScreenManager();

    void showMenu();
    void hideMenu();
    void showMain();
    void nextScreen();

    void draw();
    void handleButton(Action action);

    bool getNeedRedraw() const;
    void setNeedRedraw(bool needRedraw);

    bool isMenuActive() const;
    Screen* currentScreen() const;
};


// ---- MenuManager Implementation ----
MenuManager::MenuManager() : _current(nullptr), _rootMenu(nullptr), _screenManager(nullptr) {
    setupMenus();
}

MenuManager::~MenuManager() {
    delete _rootMenu;
}

void MenuManager::setScreenManager(ScreenManager* sm) {
    _screenManager = sm;
}

void MenuManager::setupMenus() {
    _rootMenu = new Menu("Main Menu");
    _current = _rootMenu;

    

    _rootMenu->addMenuItem(new ActionMenuItem("WiFi Setup", []() {
        app_log("WiFi setup selected");
        }
    ));

    Menu* settingsMenu = new Menu("Settings");
    _rootMenu->addMenuItem(new SubmenuMenuItem("Settings", settingsMenu));



    std::vector<String> brightnessOptions = { "10%", "20%", "30%", "40%", "50%",
                                              "60%", "70%", "80%", "90%", "100%" };
    settingsMenu->addMenuItem(new SelectableMenuItem(
        "Brightness", brightnessOptions, 4,
        []() -> int { return brightness->getPercent() / 10 - 1; },
        [](int index) {
            int percent = (index + 1) * 10;
            brightness->setPercent(percent);
        }
    ));

    

    // settingsMenu->addMenuItem(new ToggleMenuItem(
    //     "Sound", true, -1,
    //     [](bool enabled) {
    //         app_log("Sound " + String(enabled ? "enabled" : "disabled"));
    //     }
    // ));

    // settingsMenu->addMenuItem(new NumericMenuItem(
    //     "Volume", 50, 0, 100, 5, -2,
    //     [](int volume) {
    //         app_log("Volume set to: " + String(volume));
    //     }
    // ));
    

    std::vector<String> sleepOptions = { "Never", "30s", "1min", "2min", "5min", "10min" };
    settingsMenu->addMenuItem(new SelectableMenuItem(
        "Auto Sleep", sleepOptions, 2, -3,
        [sleepOptions](int index) {
            app_log(String("Auto sleep set to: ") + sleepOptions[index]);
        }
    ));

    settingsMenu->addMenuItem(new ActionMenuItem("Factory Reset", []() {
        app_log("Factory reset initiated");
        ESP.restart();
        }
    ));

    BackMenuItem* backToMain = new BackMenuItem();
    backToMain->setTargetMenu(_rootMenu);
    settingsMenu->addMenuItem(backToMain);


    Menu* ip5356Setting = new Menu("IP5356 Settings");
    ip5356Setting->addMenuItem(new ToggleMenuItem(
        "Reduce I-Hot NTC", false, []()->bool{
            uint8_t value;
            if(!ip5356->_read(IP5356::NTC_CTL1, &value)) return false;
            if(value & 0b01000000){
                return true;
            }
            return false;
        },
        [](bool state){
            uint8_t value;
            if (!ip5356->_read(IP5356::NTC_CTL1, &value)) return;
            if (value & 0b01000000) {
                if(!state){
                    value &= 0b10111111;
                    ip5356->_write(IP5356::NTC_CTL1, &value);
                }
            }
            else {
                if(state){
                    value |= 0b01000000;
                    ip5356->_write(IP5356::NTC_CTL1, &value);
                }
            }
        }
    ));

    ip5356Setting->addMenuItem(backToMain);

    _rootMenu->addMenuItem(new SubmenuMenuItem("IP5356 Settings", ip5356Setting));

    _rootMenu->addMenuItem(new ActionMenuItem("About", []() {
        app_log("About selected");
        }
    ));

    _rootMenu->addMenuItem(new ActionMenuItem("Back", [this]() {
        if (this->_screenManager) {
            this->_screenManager->showMain();
        }
        }
    ));
}

void MenuManager::setCurrentMenu(Menu* menu) {
    _current = menu;
}

Menu* MenuManager::currentMenu() const {
    return _current;
}

Menu* MenuManager::getRootMenu() {
    return _rootMenu;
}

void MenuManager::draw() {
    if (_current) _current->draw();
}

void MenuManager::handleAction(Action action) {
    if (!_current) return;

    switch (action) {
    case Action::CLICK:
        _current->navigateDown();
        draw();
        app_log("Menu Navigate Down");
        break;

    case Action::DOUBLE_CLICK:
        _current->navigateUp();
        draw();
        app_log("Menu Navigate Up");
        break;

    case Action::LONG_PRESS_START: {
        Menu* submenu = _current->selectCurrentMenuItem();
        if (submenu) setCurrentMenu(submenu);
        draw();
        app_log("Menu Select/Enter");
        break;
    }

    case Action::LEFT:
        _current->handleLeft();
        draw();
        app_log("Menu Left");
        break;

    case Action::RIGHT:
        _current->handleRight();
        draw();
        app_log("Menu Right");
        break;

    default:
        break;
    }
}


// ---- ScreenManager Implementation ----
ScreenManager::ScreenManager() {
    log_i("ScreenManager init");
    _screen[0] = new MainScreen;
    _screen[1] = new InfoScreen;
    _screenIndex = 0;
    _menuManager = new MenuManager();
    _menuManager->setScreenManager(this);
    _menuActive = false;
}

void ScreenManager::showMenu() {
    setNeedRedraw(false);
    _menuActive = true;
    _menuManager->setCurrentMenu(_menuManager->getRootMenu() ? _menuManager->getRootMenu() : nullptr);
    _menuManager->draw();
}

void ScreenManager::hideMenu() {
    _menuActive = false;
    setNeedRedraw(false);
}

void ScreenManager::showMain() {
    _screenIndex = 0;
    hideMenu();
}

void ScreenManager::nextScreen() {
    _screenIndex = (_screenIndex + 1) % 2;
    setNeedRedraw(true);
}

void ScreenManager::draw() {
    if (_menuActive) {
        return;
    }
    _screen[_screenIndex]->draw();
}

void ScreenManager::handleButton(Action action) {
    if (_menuActive) {
        _menuManager->handleAction(action);
        return;
    }

    if (action == Action::DOUBLE_CLICK){
        tft->setRotation(tft->getRotation() == 1 ? 3 : 1);
        return;
    }
    if (action == Action::LONG_PRESS_START) {
        showMenu();
        return;
    }
    if (action == Action::CLICK) {
        nextScreen();
        return;
    }

    _screen[_screenIndex]->handleButton(action);
}

bool ScreenManager::getNeedRedraw() const {
    return _menuActive ? true : _screen[_screenIndex]->getNeedRedraw();
}

void ScreenManager::setNeedRedraw(bool needRedraw) {
    if (_menuActive) {
        //_menuManager->draw();
    }
    else {
        _screen[_screenIndex]->setNeedRedraw(needRedraw);
    }
}

bool ScreenManager::isMenuActive() const { return _menuActive; }
Screen* ScreenManager::currentScreen() const { return _screen[_screenIndex]; }

#endif // MANAGER_H
