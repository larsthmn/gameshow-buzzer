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

#define chipSelect SS // ESP32 SD card select pin

#define SOUNDBOARD_DIR "/soundboard"

struct SoundRequest
{
   char filename[128];
   int prio; // Playback prio, if a higher prio request comes in, lower one is stopped
};

static void readFiles(std::vector<SoundBoardPage>& pages)
{
   File root = SD.open(SOUNDBOARD_DIR);
   if(!root) {
      Serial.println("Failed to open " SOUNDBOARD_DIR " directory");
      return;
   }

   pages.clear();
   pages.reserve(10);

   while (File file = root.openNextFile()) {
      if (file.isDirectory()) {
         SoundBoardPage newPage;
         std::string empty;
         newPage.files.reserve(6);

         // Get the full directory name
         std::string dirname = file.name();
         Serial.printf("dirname: %s\n", dirname.c_str());

         // Skip part until the first underscore
         size_t firstUnderscore = dirname.find('_');
         Serial.printf("Underscore @ %i\n", firstUnderscore);
         if (firstUnderscore == std::string::npos) {
            continue;
         }
         int folderIndex = std::stoi(dirname.substr(0, firstUnderscore));
         // Letters as index will end up as zero but that's acceptable
         size_t nextUnderscore = dirname.find('_', firstUnderscore + 1);
         Serial.printf("Index: %i, 2nd Underscore @ %i\n", folderIndex, nextUnderscore);
         newPage.name = dirname.substr(firstUnderscore + 1, nextUnderscore != std::string::npos ? nextUnderscore : dirname.length());
         Serial.printf("folder name: %s\n", newPage.name.c_str());

         std::string filePath = SOUNDBOARD_DIR "/" + dirname;  // Add '/' in front of directory name

         Serial.printf("open subdir %s\n", filePath.c_str());
         File subDir = SD.open(filePath.c_str());

         while (File subFile = subDir.openNextFile()) {
            std::string filename = subFile.name();

            Serial.printf("file %s\n", filename.c_str());

            // Name is like 1_foobar[_somemoretext].wav, find out indices of description
            firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string::npos) continue;
            int index = std::stoi(filename.substr(0, firstUnderscore));
            size_t nameEnd = min(filename.find('_', firstUnderscore + 1), filename.find('.', firstUnderscore + 1));
            if (nameEnd == std::string::npos) continue;
            Serial.printf("index %i\n", index);

            // Fill in info structure
            std::string fullFilename = filePath + '/' + filename;
            SoundBoardSound fileInfo = SoundBoardSound(fullFilename, filePath.size() + firstUnderscore + 1 + 1,
                                                       filePath.size() + nameEnd + 1);
            Serial.printf("%i: %s (%s)\n", index, fileInfo.getFilename().c_str(), fileInfo.getDescription().c_str());
            newPage.files.push_back(fileInfo);
         }
         pages.push_back(newPage);
      }
   }

   // Print folder details
   for (const auto& page : pages)
   {
      Serial.printf("Folder %s\n", page.name.c_str());
      for (int j = 0; j < page.files.size(); j++)
      {
         auto& file = page.files[j];
         Serial.printf("%s: (%i) %s\n", file.getFilename().c_str(), j, file.getDescription().c_str());
      }
   }
}

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

const std::vector<SoundBoardPage>& SoundPlayer::getPages()
{
   return pages;
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
   out.SetGain(1);
   out.begin();
#endif


   while (!SD.begin(chipSelect))
   {
      Serial.println("Failed to initialize SD card");
      delay(100);
   }
   Serial.println("SD card initialized successfully");

   readFiles(pages);

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

void SoundPlayer::start()
{
   playQueue = xQueueCreate(2, sizeof(SoundRequest));
   xTaskCreate(playbackHandlerStub, "PlaybackTask", 8192, this, 2 | portPRIVILEGE_BIT, &playbackTask);
}




