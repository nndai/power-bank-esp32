#ifndef SCREEN_H
#define SCREEN_H

#include <TFT_eSPI.h>
#include <IP5356.h>
#include "Org_01.h"
#include "ButtonManager.h"
#include "Menu.h"
#include "Globals.h"
#include "VideoPlayer.h"


/**
 * @brief Abstract base class for different screens in the application.
 */
class Screen {
protected:
    bool _needRedraw = true;

public:
    virtual void draw() = 0;
    virtual void handleButton(Action action) = 0;
    void setNeedRedraw(bool needRedraw = true);

    bool getNeedRedraw() const;

    virtual ~Screen() = default;
};


/**
 * @brief Main screen of the application.
 */
class MainScreen : public Screen {
public:
    void draw() override;

    void handleButton(Action action) override;
};

/**
 * @brief Info screen of the application.
 */
class InfoScreen : public Screen {
public:
    void draw() override;

    void handleButton(Action action) override;
};


#endif // SCREEN_H
