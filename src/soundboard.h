//
// Created by Lars on 05.05.2024.
//

#ifndef ESP32_BUZZER_SOUNDBOARD_H
#define ESP32_BUZZER_SOUNDBOARD_H

#include <string>
#include <vector>

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
   int index;
};

class SoundBoard
{
   std::vector<SoundBoardPage> pages;
public:
   void begin();
   int getPageCount();
   int getFileCount(int pageIndex);
   std::string getPageName(int index);
   std::string getFileName(int pageIndex, int fileIndex);
   std::string getDescription(int pageIndex, int fileIndex);
};


#endif //ESP32_BUZZER_SOUNDBOARD_H
