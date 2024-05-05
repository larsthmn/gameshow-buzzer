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
      for (int j = 0; j < page.files.size(); j++)
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
 * @param newPage The SoundBoardPage object to create.
 *
 * @return Returns 0 if the SoundBoardPage is successfully created,
 *         -1 if there is no underscore in dirname and the name is therefore invalid.
 */
static int createPageFromDirname(const std::string& dirname, SoundBoardPage& newPage)
{
   // Skip part until the first underscore
   size_t firstUnderscore = dirname.find('_');
   if (firstUnderscore == std::string::npos) {
      return -1;
   }
   // Letters as index will end up as zero but that's acceptable
   size_t nextUnderscore = dirname.find('_', firstUnderscore + 1);
   newPage.files.reserve(6);
   newPage.index = std::stoi(dirname.substr(0, firstUnderscore));
   newPage.name = dirname.substr(firstUnderscore + 1, nextUnderscore != std::string::npos ? nextUnderscore : dirname.length());
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

   pages.clear();
   pages.reserve(10);

   while (File file = root.openNextFile()) {
      if (file.isDirectory()) {
         SoundBoardPage newPage;
         if (createPageFromDirname(file.name(), newPage) != 0) continue;

         std::string filePath = SOUNDBOARD_DIR "/";
         filePath.append(file.name());

         File subDir = SD.open(filePath.c_str());
         while (File subFile = subDir.openNextFile()) {
            std::string filename = subFile.name();

            // Name is like 1_foobar[_somemoretext].wav, find out indices of description
            size_t firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string::npos) continue;
            int index = std::stoi(filename.substr(0, firstUnderscore));
            size_t nameEnd = min(filename.find('_', firstUnderscore + 1), filename.find('.', firstUnderscore + 1));
            if (nameEnd == std::string::npos) continue;

            // Fill in info structure
            std::string fullFilename = filePath + '/';
            fullFilename.append(filename);
            SoundBoardSound fileInfo = SoundBoardSound(fullFilename, filePath.size() + firstUnderscore + 1 + 1,
                                                       filePath.size() + nameEnd + 1);
            newPage.files.push_back(fileInfo);
         }
         pages.push_back(newPage);
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
   return pages.size();
}

int SoundBoard::getFileCount(int pageIndex)
{
   if (pageIndex >= pages.size())
   {
      return 0;
   }
   return pages[pageIndex].files.size();
}

std::string SoundBoard::getPageName(int index)
{
   return pages[index].name;
}

std::string SoundBoard::getFileName(int pageIndex, int fileIndex)
{
   if (pageIndex >= pages.size() || fileIndex >= pages[pageIndex].files.size())
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getFilename();
}

std::string SoundBoard::getDescription(int pageIndex, int fileIndex)
{
   if (pageIndex >= pages.size() || fileIndex >= pages[pageIndex].files.size())
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getDescription();
}
