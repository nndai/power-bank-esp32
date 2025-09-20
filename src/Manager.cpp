#include <Arduino.h>
#include "Manager.h"
#include <LittleFS.h>
#include "Globals.h"

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

    _rootMenu->addMenuItem(new ActionMenuItem("Power Off", []() {
        sleep();
        }
    ));

    // _rootMenu->addMenuItem(new ActionMenuItem("WiFi Setup", []() {
    //     app_log("WiFi setup selected");
    //     }
    // ));

    
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

    settingsMenu->addMenuItem(new SelectableMenuItem(
        "Auto Sleep", sleepOptions, 2,
        []() -> int {
            return powerBankData.indexTimeAutoSleep;
        },
        [](int index) {
            powerBankData.indexTimeAutoSleep = index;
        }
    ));

    settingsMenu->addMenuItem(new ToggleMenuItem(
        "On low brightness", true,
        []()->bool {
            return powerBankData.onLowBright;
        },
        [](bool state) {
            powerBankData.onLowBright = state;
        }
    ));

    settingsMenu->addMenuItem(new ToggleMenuItem(
        "Wifi", false,
        []()->bool{
            return powerBankData.onWifi;
        },
        [](bool state) {
            powerBankData.onWifi = state;
        }
    ));

    settingsMenu->addMenuItem(new ActionMenuItem("Factory Reset", []() {
        brightness->off();
        ip5356->powerOff();
        PowerBankData data;
        data.batCapacity = (ip5356->getBatteryVoltage() - 3.1) * 100; // percent / 100 * 10000
        savePowerBankData(data);
        ESP.restart();
        }
    ));

    BackMenuItem* backToMain = new BackMenuItem();
    backToMain->setTargetMenu(_rootMenu);
    settingsMenu->addMenuItem(backToMain);

    Menu* videosMenu = new Menu("Videos");
    if (!LittleFS.begin(true)) {
        app_log_e("LittleFS mount failed");
    }
    else {
        fs::File dir = LittleFS.open("/videos");
        if (dir && dir.isDirectory()) {
            for (fs::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
                if (f.isDirectory()) continue;
                String name = String(f.name());
                if (!(name.endsWith(".vd") || name.endsWith(".mjpg"))) continue;
                String fullPath = "/videos/" + name.substring(name.lastIndexOf('/') + 1);
                videosMenu->addMenuItem(new ActionMenuItem(name, [this, fullPath]() {
                    if (this->_screenManager) this->_screenManager->playVideo(fullPath, true);
                    }));
            }
        }
    }
    BackMenuItem* backFromVideos = new BackMenuItem();
    backFromVideos->setTargetMenu(_rootMenu);
    videosMenu->addMenuItem(backFromVideos);
    _rootMenu->addMenuItem(new SubmenuMenuItem("Play Gif", videosMenu));


    Menu* ip5356Setting = new Menu("IP5356 Settings");
    ip5356Setting->addMenuItem(new ToggleMenuItem(
        "Reduce I when hot", false,
        []()->bool {
            uint8_t value;
            if (!ip5356->_read(IP5356::NTC_CTL1, &value)) return false;
            if (value & 0b01000000) {
                powerBankData.reduceChargeCurrentWhenHot = true;
                return true;
            }
            powerBankData.reduceChargeCurrentWhenHot = false;
            return false;
        },
        [](bool state) {
            powerBankData.reduceChargeCurrentWhenHot = state;
            ip5356->reduceChargeCurrentWhenHot(state);
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
    _screen[0] = new MainScreen;
    _screen[1] = new InfoScreen;
    _screenIndex = 0;
    _mode = MAIN;
}

void ScreenManager::showMenu() {
    if (!_menuManager) {
        _menuManager = new MenuManager();
        _menuManager->setScreenManager(this);
    }
    setNeedRedraw(false);
    _mode = MENU;
    _menuManager->setCurrentMenu(_menuManager->getRootMenu() ? _menuManager->getRootMenu() : nullptr);
    _menuManager->draw();
}

void ScreenManager::hideMenu() {
    _mode = MAIN;
    setNeedRedraw(false);
}

void ScreenManager::showMain() {
    _mode = MAIN;
    _screenIndex = 0;
    hideMenu();
}

void ScreenManager::nextScreen() {
    _screenIndex = (_screenIndex + 1) % 2;
    setNeedRedraw(true);
}

void ScreenManager::draw() {
    if (_mode != MAIN) {
        return;
    }
    _screen[_screenIndex]->draw();
}

void ScreenManager::handleButton(Action action) {
    if (_mode == MENU && _menuManager) {
        _menuManager->handleAction(action);
        return;
    }

    if (action == Action::DOUBLE_CLICK) {
        tft->setRotation(tft->getRotation() == 1 ? 3 : 1);
        return;
    }
    if (action == Action::LONG_PRESS_START) {
        if (_mode == MAIN) {
            showMenu();
        }
        return;
    }
    if (action == Action::CLICK) {
        if (_mode == PLAY_VIDEO && _videoPlayer) {
            if (_videoPlayer->isPlaying()) {
                _videoPlayer->stop();
            }
            _mode = MENU;
            _menuManager->draw();
            return;
        }
        nextScreen();
        return;
    }
    _screen[_screenIndex]->handleButton(action);
}

bool ScreenManager::getNeedRedraw() const {
    return _mode != MAIN ? true : _screen[_screenIndex]->getNeedRedraw();
}

void ScreenManager::setNeedRedraw(bool needRedraw) {
    if (_mode == MAIN) {
        _screen[_screenIndex]->setNeedRedraw(needRedraw);
    }
}

bool ScreenManager::isMenuActive() const { return _mode == MENU; }
Screen* ScreenManager::currentScreen() const { return _screen[_screenIndex]; }
ScreenManager::ModeScreen ScreenManager::getMode() const { return _mode; }

void ScreenManager::playVideo(String fileName, bool isLoop) {
    
    if (!_videoPlayer) {
        _videoPlayer = new VideoPlayer;
    }
    ModeScreen temp = _mode;
    _mode = PLAY_VIDEO;
    //app_log("Play video " + fileName);
    if (!_videoPlayer->open(fileName)) {
        _mode == temp;
        _menuManager->draw();
        return;
    }
    _videoPlayer->setLoop(isLoop);
    
    _videoPlayer->play();
}


