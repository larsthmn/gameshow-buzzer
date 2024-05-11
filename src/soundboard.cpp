//
// Created by Lars on 05.05.2024.
//

#include "soundboard.h"
#include <Arduino.h>
#include <SD.h>

#define SOUNDBOARD_DIR "/soundboard"

static void printSoundBoardPages(const std::vector<SoundBoardPage>& pages)
{
   for (const auto& page : pages)
   {
      Serial.printf("Folder %s\n", page.name.c_str());
      for (int j = 0; j < 6; j++)
      {
         auto& file = page.files[j];
         Serial.printf("%s: (%i) %s\n", file.getFilename().c_str(), j, file.getDescription().c_str());
      }
   }
}

/**
 * @brief Creates a SoundBoardPage from a dirname.
 *
 * @param dirname The directory name from which to create the SoundBoardPage.
 * @param page The SoundBoardPage object to manipulate.
 *
 * @return Returns 0 if the SoundBoardPage is successfully created,
 *         -1 if there is no underscore in dirname and the name is therefore invalid.
 */
static int parseDirname(const std::string& dirname, int& index, std::string& name)
{
   // Skip part until the first underscore
   size_t firstUnderscore = dirname.find('_');
   if (firstUnderscore == std::string::npos) {
      return -1;
   }
   // Letters as index will end up as zero but that's acceptable
   size_t nextUnderscore = dirname.find('_', firstUnderscore + 1);
   index = std::stoi(dirname.substr(0, firstUnderscore));
   name = dirname.substr(firstUnderscore + 1, nextUnderscore != std::string::npos ? nextUnderscore : dirname.length());
   return 0;
}


/**
 * @brief Read files from a directory and create SoundBoardPages.
 *
 * @param pages A vector of SoundBoardPage objects to store the created pages.
 */
static void readFiles(std::vector<SoundBoardPage>& pages)
{
   File root = SD.open(SOUNDBOARD_DIR);
   if(!root) {
      Serial.println("Failed to open " SOUNDBOARD_DIR " directory");
      return;
   }

   // Find highest index
   int highestIndex = 0;
   while (File file = root.openNextFile()) {
      if (file.isDirectory()) {
         int index;
         std::string name;
         if (parseDirname(file.name(), index, name) == 0) highestIndex = max(highestIndex, index);
      }
   }
   Serial.printf("Highest index is %i\n", highestIndex);

   // Iterate over directories to extract Soundboard sounds
   pages.clear();
   pages.resize(highestIndex);
   root.rewindDirectory();
   while (File file = root.openNextFile()) {
      if (file.isDirectory()) {
         SoundBoardPage newPage;
         int pageIndex;
         if (parseDirname(file.name(), pageIndex, newPage.name) != 0) continue;
         if (pageIndex < 1 || pageIndex > 64) continue;

         std::string filePath = SOUNDBOARD_DIR "/";
         filePath.append(file.name());

         File subDir = SD.open(filePath.c_str());
         while (File subFile = subDir.openNextFile()) {
            std::string filename = subFile.name();

            // Name is like 1_foobar[_somemoretext].wav, find out indices of description
            size_t firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string::npos) continue;
            int index = std::stoi(filename.substr(0, firstUnderscore));
            if (index < 1 || index > 6) continue;
            size_t nameEnd = min(filename.find('_', firstUnderscore + 1), filename.find('.', firstUnderscore + 1));
            if (nameEnd == std::string::npos) continue;

            // Fill in info structure
            std::string fullFilename = filePath + '/';
            fullFilename.append(filename);
            SoundBoardSound fileInfo = SoundBoardSound(fullFilename, filePath.size() + firstUnderscore + 1 + 1,
                                                       filePath.size() + nameEnd + 1);
            newPage.files[index - 1] = fileInfo;
         }
         pages[pageIndex - 1] = newPage;
      }
   }

   printSoundBoardPages(pages);
}

void SoundBoard::begin()
{
   readFiles(pages);
}

int SoundBoard::getPageCount()
{
   return (int)pages.size();
}

std::string SoundBoard::getPageName(int index)
{
   if (index >= pages.size())
   {
      return "";
   }
   return pages[index].name;
}

std::string SoundBoard::getFileName(int pageIndex, int fileIndex)
{
   if (pageIndex >= pages.size() || fileIndex >= 6)
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getFilename();
}

std::string SoundBoard::getDescription(int pageIndex, int fileIndex)
{
   if (pageIndex >= pages.size() || fileIndex >= 6)
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getDescription();
}
