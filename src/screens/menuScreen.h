//
// Created by Lars on 09.05.2024.
//

#ifndef ESP32_BUZZER_MENU_H
#define ESP32_BUZZER_MENU_H

#include "screens.h"
#include <LiquidCrystal.h>

void menuInit();
Screen menuScreen(const InputValues& values, LiquidCrystal& lcd, bool enter);;

#endif //ESP32_BUZZER_MENU_H
