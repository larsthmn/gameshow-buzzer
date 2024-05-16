//
// Created by Lars on 05.05.2024.
//

#ifndef ESP32_BUZZER_SOUNDBOARD_H
#define ESP32_BUZZER_SOUNDBOARD_H

#include <string>
#include <vector>

constexpr int FILES_PER_PAGE = 6;
constexpr int MAX_QUICKACCESS_LEN = 2;
template<int BASE, int EXP>
struct Pow {
   static constexpr int result = BASE * Pow<BASE, EXP - 1>::result;
};
template<int BASE>
struct Pow<BASE, 0> {
   enum { result = 1 };
};
constexpr int MAX_PAGE_COUNT = Pow<FILES_PER_PAGE, MAX_QUICKACCESS_LEN>::result;


class SoundBoardSound
{
private:
   std::string filename;
   size_t descriptionStart, descriptionEnd;

public:
   SoundBoardSound() : descriptionStart(0), descriptionEnd(0) {}

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
   SoundBoardSound files[FILES_PER_PAGE];
   int quickAccess[MAX_QUICKACCESS_LEN];
};

class SoundBoard
{
private:
   std::vector<SoundBoardPage> pages;
public:
   void begin();
   int getPageCount();
   std::string getPageName(int index);
   std::string getFileName(int pageIndex, int fileIndex);
   std::string getDescription(int pageIndex, int fileIndex);
   int getPageIndexFromSequence(const int* sequence);
   int getPageRangeFromSequence(const int* sequence, int& minPage, int& maxPage);
};


#endif //ESP32_BUZZER_SOUNDBOARD_H
