//
// Created by Lars on 14.04.2024.
//

#ifndef ESP32_BUZZER_SOUNDS_H
#define ESP32_BUZZER_SOUNDS_H

#include <string>
#include <Arduino.h>

#define SOUND_TIMER_BEEP "/buzzer/contdown1_beep.wav"
//#define SOUND_TIMER_END  "/soundboard/3_Divers 2/1_AnzeigBel_anzeigenhauptmeister-das-gibt-ne-anzeige-wegen-beleidigung.wav"
#define SOUND_TIMER_END  "/buzzer/contdown1_beepHighpitch.wav"
#define SOUND_TIMER_START  "/buzzer/ding.wav"

#define SOUND_PRIO_BUZZER_START 2
#define SOUND_PRIO_BUZZER_BEEP 2
#define SOUND_PRIO_BUZZER_END 2
#define SOUND_PRIO_SOUNDBOARD 1

class SoundPlayer
{
private:
   xQueueHandle playQueue;
   xTaskHandle playbackTask{};
   static void playbackHandlerStub(void* param);
   [[noreturn]] void playbackHandler();
public:
   void begin();
   void requestPlayback(const std::string& filename, int prio);
};

#endif //ESP32_BUZZER_SOUNDS_H
