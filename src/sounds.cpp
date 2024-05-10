//
// Created by Lars on 14.04.2024.
//

#include "sounds.h"
#include "pins.h"

// Tried also ESP32-audioI2S, ESP8266Audio seems to be better, other one has following problems:
//  - cut off sounds / sounds not even played when too short
//  - failure to play some wav files breaking the lib (no more sounds playable after that)
// Both have some artifacts and different volume levels, but this could be due to HW reasons

#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include <Arduino.h>

SoundPlayer soundPlayer;

struct SoundRequest
{
   char filename[128];
   int prio; // Playback prio, if a higher prio request comes in, lower one is stopped
   uint8_t volume;
};


void SoundPlayer::playbackHandlerStub(void* param){
   // Needed for C++ compatibility
   auto* self = static_cast<SoundPlayer*>(param);
   self->playbackHandler();
}

void SoundPlayer::requestPlayback(const std::string& filename, int prio, uint8_t volume)
{
   if (volume <= 0) return;
   if (volume > 100) volume = 100;
   SoundRequest request{};
   strcpy(request.filename, filename.c_str());
   request.prio = prio;
   request.volume = volume;
   xQueueSend(playQueue, &request, pdMS_TO_TICKS(10));
}

[[noreturn]] void SoundPlayer::playbackHandler()
{
   delay(1000);

   AudioGeneratorWAV wav;
   AudioOutputI2S out;
   out.SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
   out.begin();

   while (true)
   {
      SoundRequest currentRequest{};
      // Wait for a new request to arrive
      if (xQueueReceive(playQueue, &currentRequest, portMAX_DELAY))
      {
         play:
         Serial.printf("%lu: Playback of %s (prio %i, vol %i%%)\n", millis(), currentRequest.filename, currentRequest.prio, currentRequest.volume);

         out.SetGain((float)currentRequest.volume / 100.0f);

         auto in = AudioFileSourceSD(currentRequest.filename);
         wav.begin(&in, &out);
         int currentPrio = currentRequest.prio;
         char currentPlayback[128];
         strcpy(currentPlayback, currentRequest.filename);

         while (wav.isRunning())
         {
            if (xQueueReceive(playQueue, &currentRequest, 0))
            {
               // Request same playback again = stop
               if (strcmp(currentRequest.filename, currentPlayback) == 0)
               {
                  break;
               }
               // Cancel by higher or same prio playback
               if (currentRequest.prio <= currentPrio) {
                  Serial.printf("current playback cancelled by other playback\n");
                  goto play; // i know you shouldn't but hee hee
               }
            }
            wav.loop();
            delay(1);
         }
         wav.stop();
         Serial.println("Finish playback");
      }
   }
}

void SoundPlayer::begin()
{
   playQueue = xQueueCreate(2, sizeof(SoundRequest));
   xTaskCreatePinnedToCore(playbackHandlerStub, "PlaybackTask", 8192, this, 2 | portPRIVILEGE_BIT, &playbackTask, 0);
}




