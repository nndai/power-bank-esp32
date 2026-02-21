#include <Arduino.h>
#include "Manager.h"
#include <LittleFS.h>
#include "Globals.h"
#include "LineChart.h"
#include "ChargeLogger.h"

// Helper: draw a chart from a charge log file column
static void drawChargeChart(const String& filename, ChargeLogger::Column col,
                            LineChart::AxisUnit yUnit, uint16_t lineColor) {
    if (!chargeLogger) return;

    int count = chargeLogger->getEntryCount(filename);
    if (count < 2) return;

    float* timeData = new float[count];
    float* yData    = new float[count];

    int n1 = chargeLogger->loadColumn(filename, ChargeLogger::COL_TIME, timeData, count);
    int n2 = chargeLogger->loadColumn(filename, col, yData, count);
    int n  = min(n1, n2);

    if (n >= 2) {
        LineChart chart(img);
        LineChart::Config cfg;
        cfg.x = 0;
        cfg.y = 0;
        cfg.w = 160;
        cfg.h = 80;
        cfg.gridRows = 4;
        cfg.gridCols = 6;
        cfg.lineColor = lineColor;
        cfg.bgColor = TFT_BLACK;
        cfg.yUnit = yUnit;
        cfg.xUnit = LineChart::AxisUnit::SECOND;
        cfg.linearX = true;

        chart.draw(timeData, yData, n, cfg);
        img->pushSprite(0, 0);
    }

    delete[] timeData;
    delete[] yData;
}

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

    // --- Charge Data submenu (dynamically populated) ---
    Menu* chargeDataMenu = new Menu("Charge Data");

    // Populate charge data menu each time user enters it
    // We use an ActionMenuItem as a "Refresh" trigger that rebuilds the list
    chargeDataMenu->addMenuItem(new ActionMenuItem("Refresh list", [this, chargeDataMenu]() {
        // Clear existing items (except first "Refresh" and last "Back")
        // For simplicity, we rebuild the entire menu on demand
        // Note: Menu items are managed by Menu, so we just add new ones

        if (!chargeLogger) return;

        std::vector<String> files;
        chargeLogger->listFiles(files);

        if (files.empty()) {
            img->fillSprite(TFT_BLACK);
            img->setTextColor(TFT_WHITE);
            img->drawString("No charge logs", 10, 36);
            img->pushSprite(0, 0);
            return;
        }

        // Create a temporary menu to show file list
        Menu* fileListMenu = new Menu("Log Files", chargeDataMenu);

        for (size_t i = 0; i < files.size(); i++) {
            String fname = files[i];
            // Each file -> submenu with data type options
            Menu* dataTypeMenu = new Menu(fname, fileListMenu);

            dataTypeMenu->addMenuItem(new ChartMenuItem("Current", [fname]() {
                drawChargeChart(fname, ChargeLogger::COL_CURRENT,
                               LineChart::AxisUnit::MILLIAMP, TFT_GREEN);
            }));
            dataTypeMenu->addMenuItem(new ChartMenuItem("Capacity", [fname]() {
                drawChargeChart(fname, ChargeLogger::COL_CAPACITY,
                               LineChart::AxisUnit::NORMAL, TFT_CYAN);
            }));
            dataTypeMenu->addMenuItem(new ChartMenuItem("Voltage", [fname]() {
                drawChargeChart(fname, ChargeLogger::COL_VOLTAGE,
                               LineChart::AxisUnit::MILLIVOLT, TFT_YELLOW);
            }));
            dataTypeMenu->addMenuItem(new ChartMenuItem("Percent", [fname]() {
                drawChargeChart(fname, ChargeLogger::COL_PERCENT,
                               LineChart::AxisUnit::NORMAL, TFT_MAGENTA);
            }));

            BackMenuItem* backFromDataType = new BackMenuItem();
            backFromDataType->setTargetMenu(fileListMenu);
            dataTypeMenu->addMenuItem(backFromDataType);

            fileListMenu->addMenuItem(new SubmenuMenuItem(fname, dataTypeMenu));
        }

        BackMenuItem* backFromFileList = new BackMenuItem();
        backFromFileList->setTargetMenu(chargeDataMenu);
        fileListMenu->addMenuItem(backFromFileList);

        this->setCurrentMenu(fileListMenu);
    }));

    BackMenuItem* backFromChargeData = new BackMenuItem();
    backFromChargeData->setTargetMenu(_rootMenu);
    chargeDataMenu->addMenuItem(backFromChargeData);

    _rootMenu->addMenuItem(new ActionMenuItem("Power Off", []() {
        sleep();
        }
    ));

    _rootMenu->addMenuItem(new SubmenuMenuItem("Charge Data", chargeDataMenu));

    _rootMenu->addMenuItem(new ActionMenuItem("WiFi Attacks", []() {
        goWifiAttack();
        }
    ));

    // _rootMenu->addMenuItem(new ActionMenuItem("WiFi Setup", []() {
    //     _app_log_i("WiFi setup selected");
    //     }
    // ));

    
    Menu* settingsMenu = new Menu("Settings");
    _rootMenu->addMenuItem(new SubmenuMenuItem("Settings", settingsMenu));

    std::vector<String> brightnessOptions = { "10%", "20%", "30%", "40%", "50%",
                                              "60%", "70%", "80%", "90%", "100%" };

    settingsMenu->addMenuItem(new ToggleMenuItem(
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
        _app_log_e("LittleFS mount failed");
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


    // Menu* ip5356Setting = new Menu("IP5356 Settings");
    // ip5356Setting->addMenuItem(new ToggleMenuItem(
    //     "Reduce I when hot", false,
    //     []()->bool {
    //         uint8_t value;
    //         if (!ip5356->_read(IP5356::NTC_CTL1, &value)) return false;
    //         if (value & 0b01000000) {
    //             powerBankData.reduceChargeCurrentWhenHot = true;
    //             return true;
    //         }
    //         powerBankData.reduceChargeCurrentWhenHot = false;
    //         return false;
    //     },
    //     [](bool state) {
    //         powerBankData.reduceChargeCurrentWhenHot = state;
    //         ip5356->reduceChargeCurrentWhenHot(state);
    //     }
    // ));

    // ip5356Setting->addMenuItem(backToMain);

    // _rootMenu->addMenuItem(new SubmenuMenuItem("IP5356 Settings", ip5356Setting));

    // _rootMenu->addMenuItem(new ActionMenuItem("About", []() {
    //     _app_log_i("About selected");
    //     }
    // ));

    _rootMenu->addMenuItem(new ActionMenuItem("Upload Firmware", []() {
        powerBankData.modeUpdateFirmware = true;
        savePowerBankData(powerBankData);
        ESP.restart();
        }
    ));

    _rootMenu->addMenuItem(new ActionMenuItem("Restart", []() {
        savePowerBankData(powerBankData);
        ESP.restart();
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
        _app_log_i("Menu Navigate Down");
        break;

    case Action::DOUBLE_CLICK:
        _current->navigateUp();
        draw();
        _app_log_i("Menu Navigate Up");
        break;

    case Action::LONG_PRESS_START: {
        MenuItem* curItem = _current->getCurrentMenuItem();
        Menu* submenu = _current->selectCurrentMenuItem();
        if (submenu) {
            setCurrentMenu(submenu);
        }
        // Skip redraw if the action renders its own full-screen content (chart)
        if (!curItem || !curItem->rendersScreen()) {
            draw();
        }
        _app_log_i("Menu Select/Enter");
        break;
    }

    case Action::LEFT:
        _current->handleLeft();
        draw();
        _app_log_i("Menu Left");
        break;

    case Action::RIGHT:
        _current->handleRight();
        draw();
        _app_log_i("Menu Right");
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
    //_app_log_i("Play video " + fileName);
    if (!_videoPlayer->open(fileName)) {
        _mode == temp;
        _menuManager->draw();
        return;
    }
    _videoPlayer->setLoop(isLoop);
    
    _videoPlayer->play();
}


