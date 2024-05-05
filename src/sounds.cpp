//
// Created by Lars on 14.04.2024.
//

#include "sounds.h"
#include "pins.h"

// Tried both libs, ESP8266Audio seems to be better, other one has following problems:
//  - cut off sounds / sounds not even played when too short
//  - failure to play some wav files breaking the lib (no more sounds playable after that)
// Both have some artifacts and different volume levels, but this could be due to HW reasons
#define USE_ESPAUDIO 0

#if USE_ESPAUDIO
#include "Audio.h"
#else
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#endif
#include <Arduino.h>


struct SoundRequest
{
   char filename[128];
   int prio; // Playback prio, if a higher prio request comes in, lower one is stopped
};


void SoundPlayer::playbackHandlerStub(void* param){
   auto* self = static_cast<SoundPlayer*>(param);
   self->playbackHandler();
}

void SoundPlayer::requestPlayback(const std::string& filename, int prio)
{
   SoundRequest request{};
   strcpy(request.filename, filename.c_str());
   request.prio = prio;
   xQueueSend(playQueue, &request, pdMS_TO_TICKS(10));
}

[[noreturn]] void SoundPlayer::playbackHandler()
{
   delay(1000);

#if USE_ESPAUDIO
   Audio audio;
   audio.setVolume(21); // 0...21
   audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
#else
   AudioGeneratorWAV wav;
   AudioOutputI2S out;
   out.SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
   out.SetGain(0.5);
   out.begin();
#endif

   while (true)
   {
      SoundRequest currentRequest{};
      // Wait for a new request to arrive
      if (xQueueReceive(playQueue, &currentRequest, portMAX_DELAY))
      {
         Serial.printf("%lu: Playback of %s (prio %i)\n", millis(), currentRequest.filename, currentRequest.prio);

#if USE_ESPAUDIO
         audio.connecttoSD(currentRequest.filename);
         audio.setVolume(21); // 0...21

         while (audio.isRunning() && !xQueuePeek(playQueue, &currentRequest, 0))
         {
            audio.loop();
         }
         delay(10);
         audio.stopSong();
#else
         auto in = AudioFileSourceSD(currentRequest.filename);
         wav.begin(&in, &out);

         while (wav.isRunning() && !xQueuePeek(playQueue, &currentRequest, 0))
         {
            wav.loop();
            delay(1);
         }
         wav.stop();
#endif
         Serial.println("Finish playback");
      }
   }
}

void SoundPlayer::begin()
{
   playQueue = xQueueCreate(2, sizeof(SoundRequest));
   xTaskCreate(playbackHandlerStub, "PlaybackTask", 8192, this, 2 | portPRIVILEGE_BIT, &playbackTask);
}




