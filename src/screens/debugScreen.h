//
// Created by Lars on 09.05.2024.
//

#ifndef ESP32_BUZZER_DEBUGSCREEN_H
#define ESP32_BUZZER_DEBUGSCREEN_H

#include "screens.h"
#include <LiquidCrystal.h>

Screen debugScreen(const InputValues& values, LiquidCrystal& lcd, bool enter);

#endif //ESP32_BUZZER_DEBUGSCREEN_H
