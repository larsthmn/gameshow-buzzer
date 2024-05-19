//
// Created by Lars on 05.05.2024.
//

#include "soundboard.h"
#include "inputs.h"
#include <Arduino.h>
#include <SD.h>
#include <vector>
#include <cmath>

#define SOUNDBOARD_DIR "/soundboard"

static const char* TAG = "soundboard";

static void printSoundBoardPages(const std::vector<SoundBoardPage>& pages)
{
   uint32_t fileCount = 0;
   for (const auto& page: pages)
   {
      ESP_LOGD(TAG, "Folder %s [%i, %i]", page.name.c_str(), page.quickAccess[0], page.quickAccess[1]);
      for (int j = 0; j < FILES_PER_PAGE; j++)
      {
         auto& file = page.files[j];
         if (file.getFilename() != "") fileCount++;
         ESP_LOGD(TAG, "%s: (%i) %s", file.getFilename().c_str(), j, file.getDescription().c_str());
      }
   }
   ESP_LOGI(TAG, "Loaded %d folders with %d files", pages.size(), fileCount);
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
   if (firstUnderscore == std::string::npos)
   {
      return -1;
   }
   // Letters as index will end up as zero but that's acceptable
   size_t nextUnderscore = dirname.find('_', firstUnderscore + 1);
   index = std::stoi(dirname.substr(0, firstUnderscore));
   name = dirname.substr(firstUnderscore + 1, nextUnderscore != std::string::npos ? nextUnderscore : dirname.length());
   return 0;
}


/**
 * @brief Calculates the sequence of button presses to navigate to a specific page.
 *
 * This function calculates the sequence of button presses needed to navigate to a specific page
 * given the total number of pages, number of buttons, and the size of the sequence array.
 *
 * @param page The page number to navigate to.
 * @param page_count The total number of pages.
 * @param num_buttons The number of buttons available.
 * @param sequence An array to store the calculated sequence.
 * @param len The size of the sequence array.
 * @return 0 if successful, -1 if the destination array is too short.
 */
int get_sequence_for_page(uint32_t page, uint32_t page_count, uint32_t num_buttons, int* sequence, size_t len)
{
   int max_seq_len = 1;
   while (std::pow(num_buttons, max_seq_len) < page_count)
   {
      max_seq_len++;
   }

   // Destination array too short
   if (len < max_seq_len) return -1;

   // Find how many pages can be accesses by a single button press while still having enough sequences left
   auto possible_pages = [&](int buttons_one_press)
   {
      return buttons_one_press + (num_buttons - buttons_one_press) * std::pow(num_buttons, max_seq_len - 1);
   };
   int buttons_one_press = 0;
   while (possible_pages(buttons_one_press + 1) >= page_count)
   {
      buttons_one_press++;
   }

   // Build sequence
   if (page < buttons_one_press)
   {
      // One button press is enough to be unique for the first ones
      sequence[0] = (int)page;
      sequence[1] = -1;
   }
   else
   {
      // Use rest buttons to build unique sequence for each page
      uint32_t remainder = page - buttons_one_press;
      for (int i = 0; i < max_seq_len; i++)
      {
         sequence[max_seq_len - i - 1] = (int)(remainder % num_buttons);
         remainder /= num_buttons;
      }
      sequence[0] += buttons_one_press;
   }

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
   if (!root)
   {
      ESP_LOGE(TAG, "Failed to open " SOUNDBOARD_DIR " directory");
      return;
   }

   // Find highest index
   int highestIndex = 0;
   while (File file = root.openNextFile())
   {
      if (file.isDirectory())
      {
         int index;
         std::string name;
         if (parseDirname(file.name(), index, name) == 0) highestIndex = max(highestIndex, index);
      }
   }
   ESP_LOGD(TAG, "Highest index is %i", highestIndex);

   // Iterate over directories to extract Soundboard sounds
   pages.clear();
   pages.resize(highestIndex);
   root.rewindDirectory();
   while (File file = root.openNextFile())
   {
      if (file.isDirectory())
      {
         SoundBoardPage newPage;
         int pageIndex;
         if (parseDirname(file.name(), pageIndex, newPage.name) != 0) continue;
         if (pageIndex < 1 || pageIndex > MAX_PAGE_COUNT) continue;

         std::string filePath = SOUNDBOARD_DIR "/";
         filePath.append(file.name());

         File subDir = SD.open(filePath.c_str());
         while (File subFile = subDir.openNextFile())
         {
            std::string filename = subFile.name();

            // Name is like 1_foobar[_somemoretext].wav, find out indices of description
            size_t firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string::npos) continue;
            int index = std::stoi(filename.substr(0, firstUnderscore));
            if (index < 1 || index > FILES_PER_PAGE) continue;
            size_t nameEnd = min(filename.find('_', firstUnderscore + 1), filename.find('.', firstUnderscore + 1));
            if (nameEnd == std::string::npos) continue;

            // Fill in info structure
            std::string fullFilename = filePath + '/';
            fullFilename.append(filename);
            SoundBoardSound fileInfo = SoundBoardSound(fullFilename, filePath.size() + firstUnderscore + 1 + 1,
                                                       filePath.size() + nameEnd + 1);
            newPage.files[index - 1] = fileInfo;
         }
         get_sequence_for_page(pageIndex - 1, highestIndex, PUSH_BUTTON_COUNT, newPage.quickAccess, sizeof newPage.quickAccess);
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
   if (pageIndex >= pages.size() || fileIndex >= FILES_PER_PAGE)
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getFilename();
}

std::string SoundBoard::getDescription(int pageIndex, int fileIndex)
{
   if (pageIndex >= pages.size() || fileIndex >= FILES_PER_PAGE)
   {
      return "";
   }
   return pages[pageIndex].files[fileIndex].getDescription();
}

/**
 * @brief Gets the page index from a given sequence
 *
 * @param sequence A pointer to the sequence array
 * @return The index of the page matching the sequence, or -1 if no match is found
 */
int SoundBoard::getPageIndexFromSequence(const int* sequence)
{
   for (int i = 0; i < pages.size(); ++i)
   {
      bool matched = true;
      for (int j = 0; j < MAX_QUICKACCESS_LEN; ++j)
      {
         if (pages[i].quickAccess[j] == -1) break;
         if (sequence[j] != pages[i].quickAccess[j]) matched = false;
      }
      if (matched) return i;
   }
   return -1;
}

int SoundBoard::getPageRangeFromSequence(const int* sequence, int& minPage, int& maxPage)
{
   minPage = INT32_MAX;
   maxPage = INT32_MIN;
   for (int i = 0; i < pages.size(); ++i)
   {
      bool matched = true;
      for (int j = 0; j < MAX_QUICKACCESS_LEN; ++j)
      {
         if (pages[i].quickAccess[j] == -1 || sequence[j] == -1) break;
         if (sequence[j] != pages[i].quickAccess[j]) matched = false;
      }
      if (matched) {
         minPage = min(minPage, i);
         maxPage = max(maxPage, i);
      }
   }
   ESP_LOGD(TAG, "Page range for sequence %i, %i is %i - %i, rc=%i", sequence[0], sequence[1], minPage, maxPage, minPage == INT32_MAX ? -1 : 0);
   return minPage == INT32_MAX ? -1 : 0;
}
