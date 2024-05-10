//
// Created by Lars on 09.05.2024.
//

#ifndef ESP32_BUZZER_SOUNDBOARDSCREEN_H
#define ESP32_BUZZER_SOUNDBOARDSCREEN_H

#include "screens.h"
#include <LiquidCrystal.h>

Screen soundBoardScreen(const InputValues& values, LiquidCrystal& lcd, bool enter);
void soundBoardScreenInit();

#endif //ESP32_BUZZER_SOUNDBOARDSCREEN_H
