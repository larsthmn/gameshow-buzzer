//
// Created by Lars on 04.05.2024.
//

#ifndef ESP32_BUZZER_AUDIOPLAYBACK_H
#define ESP32_BUZZER_AUDIOPLAYBACK_H

#include <cstdint>

void audioInit();
void audioSetVolume(uint8_t vol);
uint8_t audioGetVolume();
bool audioConnecttohost(const char* host);
bool audioConnecttoSD(const char* filename);

#endif //ESP32_BUZZER_AUDIOPLAYBACK_H
