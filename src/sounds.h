//
// Created by Lars on 14.04.2024.
//

#ifndef ESP32_BUZZER_SOUNDS_H
#define ESP32_BUZZER_SOUNDS_H

#include <string>
#include <Arduino.h>

#define SOUND_TIMER_BEEP "/buzzer/countdown_beep_short.wav"
#define SOUND_TIMER_END  "/buzzer/countdown_beep_long.wav"
#define SOUND_TIMER_START  "/buzzer/ding.wav"
#define SOUND_RANDOM  "/soundboard/0_Divers 1/6_Egon_ReinInDieFutterluke.wav"

#define SOUND_PRIO_BUZZER_START 3
#define SOUND_PRIO_BUZZER_BEEP 2
#define SOUND_PRIO_BUZZER_END 1
#define SOUND_PRIO_SOUNDBOARD 4
#define SOUND_PRIO_RANDOM 4

class SoundPlayer
{
private:
   xQueueHandle playQueue;
   xTaskHandle playbackTask{};
   static void playbackHandlerStub(void* param);
   [[noreturn]] void playbackHandler();
public:
   void begin();

   /**
    * \brief Request playback of a file
    * \param filename Filename to be played
    * \param prio Priority (lower number = higher prio)
    * \param volume Volume in percent
    */
   void requestPlayback(const std::string& filename, int prio, uint8_t volume);
};

extern SoundPlayer soundPlayer;

#endif //ESP32_BUZZER_SOUNDS_H
