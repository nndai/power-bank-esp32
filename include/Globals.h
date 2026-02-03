#ifndef GLOBALS_H
#define GLOBALS_H

#include <IP5356.h>
#include <TFT_eSPI.h>
#include <Eeprom24C.h>
#include <Log.h>
#include <vector>
#include "Brightness.h"


#define TFT_EN_PIN 4
#define TFT_BL_PIN 16

#define BUTTON_PIN 39
#define KEY_PIN 13

#define SDA_I2C_PIN 33
#define SCL_I2C_PIN 32
#define INT_PIN 35
#define NTC_DIE_PIN 36

#define BITMASK_DEEPSLEEP ((1ULL << BUTTON_PIN) | (1ULL << INT_PIN))

#define EEPROM_DATA_ADDR 0

struct PowerBankData {
    uint8_t brightness = 50;
    uint8_t percentCharged = 0;
    uint16_t chargeCycle = 0;
    float batCapacity = 0.0f;
    uint8_t indexTimeAutoSleep = 2;
    bool onLowBright = true;
    bool reduceChargeCurrentWhenHot = false;
    bool onWifi = true;
};

extern std::vector<String> sleepOptions;
extern std::vector<uint8_t> sleepValueOptions;

extern volatile PowerBankData powerBankData;

extern IP5356* ip5356;
extern TFT_eSPI* tft;
extern TFT_eSprite* img;
extern Eeprom24C* eeprom;
extern Brightness* brightness;

static const unsigned char PROGMEM imageQuickCharge[] = { 0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xc0,0x01,0x80,0x03,0x80,0x07,0x00,0x0f,0xe0,0x01,0xc0,0x03,0x80,0x03,0x00,0x06,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const unsigned char PROGMEM imageWeatherTemperature[] = { 0x1c,0x00,0x22,0x02,0x2b,0x05,0x2a,0x02,0x2b,0x38,0x2a,0x60,0x2b,0x40,0x2a,0x40,0x2a,0x60,0x49,0x38,0x9c,0x80,0xae,0x80,0xbe,0x80,0x9c,0x80,0x41,0x00,0x3e,0x00 };


void _app_log_e(String mess, String end = "\n");
void _app_log_i(String mess, String end = "\n");

void savePowerBankData(PowerBankData data);
void sleep();

#endif //GLOBALS_H
