//
// Created by Lars on 09.05.2024.
//

#ifndef ESP32_BUZZER_SCREENS_H
#define ESP32_BUZZER_SCREENS_H

#include "inputs.h"
#include "pins.h"
#include <LiquidCrystal.h>

enum Screen
{
   SCREEN_SOUNDBOARD,
   SCREEN_MENU,
   SCREEN_DEBUG,
   SCREEN_COUNT
};

// return value: next screen
typedef Screen (* screenFunction)(const InputValues& values, LiquidCrystal& lcd, bool enter);

class ScreenManager {
private:
   LiquidCrystal display;

public:
   ScreenManager() : display(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7) {}
   void init();
   void loop(const InputValues& values);
};

#endif //ESP32_BUZZER_SCREENS_H
