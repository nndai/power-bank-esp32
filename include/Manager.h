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
public:
    enum ModeScreen {
        MAIN = 0,
        MENU,
        PLAY_VIDEO,
    };
    
private:
    Screen* _screen[2];
    uint8_t _screenIndex;
    MenuManager* _menuManager = nullptr;
    VideoPlayer* _videoPlayer = nullptr;
    ModeScreen _mode;

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
    ModeScreen getMode() const;

    void playVideo(String fileName, bool isLoop);
};

#endif // MANAGER_H
