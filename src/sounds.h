//
// Created by Lars on 14.04.2024.
//

#ifndef ESP32_BUZZER_SOUNDS_H
#define ESP32_BUZZER_SOUNDS_H

#include <string>
#include <vector>
#include <Arduino.h>

#define SOUND_TIMER_BEEP "/buzzer/beep.wav"
#define SOUND_TIMER_END  "/buzzer/ding.wav"
#define SOUND_TIMER_START  "/buzzer/beep.wav"

class SoundBoardSound
{
private:
   std::string filename;
   size_t descriptionStart, descriptionEnd;

public:
   SoundBoardSound(std::string& filen, size_t start, size_t end)
      : filename(std::move(filen)), descriptionStart(start), descriptionEnd(end)
   {
   }

   std::string getDescription() const
   {
      return filename.substr(descriptionStart, descriptionEnd - descriptionStart);
   }

   // Getter for filename
   const std::string& getFilename() const { return filename; }
};

struct SoundBoardPage
{
   std::string name;
   std::vector<SoundBoardSound> files;
};

class SoundPlayer
{
private:
   std::vector<SoundBoardPage> pages;
   xQueueHandle playQueue;
   xTaskHandle playbackTask{};
   static void playbackHandlerStub(void* param);
   [[noreturn]] void playbackHandler();
public:
   void start();
   void requestPlayback(const std::string& filename, int prio);
   const std::vector<SoundBoardPage>& getPages();
};

#endif //ESP32_BUZZER_SOUNDS_H
