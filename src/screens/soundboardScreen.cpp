//
// Created by Lars on 09.05.2024.
//

#include "debugScreen.h"
#include "soundboard.h"
#include "sounds.h"
#include "config.h"

SoundBoard soundBoard;

Screen soundBoardScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   static int displayPage = -1;
   if (enter) displayPage = -1; // leads to screen update
   static int currentPage = 0;
   int soundBoardPagesCount = soundBoard.getPageCount();
   if (soundBoardPagesCount == 0) return SCREEN_MENU;
   if (values.lcdBtnChanged)
   {
      if (values.lcdBtn == BUTTON_LEFT)
      {
         bool isFirstPage = currentPage == 0;
         currentPage = isFirstPage ? soundBoardPagesCount - 1 : (currentPage - 1) % soundBoardPagesCount;
      }
      else if (values.lcdBtn == BUTTON_RIGHT)
      {
         currentPage = (currentPage + 1) % soundBoardPagesCount;
      }
   }
   if (displayPage != currentPage)
   {
      lcd.clear();
      for (int r = 0; r < 3; ++r)
      {
         lcd.setCursor(0, r);
         lcd.print(soundBoard.getDescription(currentPage, r * 2).c_str());

         lcd.setCursor(9, r);
         lcd.print("|");
         std::string soundRight = soundBoard.getDescription(currentPage, r * 2 + 1);
         lcd.setCursor(20 - soundRight.size(), r);
         lcd.print(soundRight.c_str());
      }
      lcd.setCursor(0, 3);
      lcd.print("<");
      lcd.setCursor(2, 3);
      lcd.print(currentPage + 1);
      lcd.print("/");
      lcd.print(soundBoardPagesCount);
      lcd.print(" ");
      lcd.print(soundBoard.getPageName(currentPage).c_str());
      lcd.setCursor(19, 4);
      lcd.print(">");
      displayPage = currentPage;
   }
   if (values.pushBtnChanged)
   {
      int fileIndex = -1;
      switch (values.pushBtn)
      {
         case BUTTON_YELLOW:
            fileIndex = 0;
            break;
         case BUTTON_RED:
            fileIndex = 1;
            break;
         case BUTTON_BLACK:
            fileIndex = 2;
            break;
         case BUTTON_GREEN:
            fileIndex = 3;
            break;
         case BUTTON_WHITE:
            fileIndex = 4;
            break;
         case BUTTON_BLUE:
            fileIndex = 5;
            break;
         default:
            break;
      }
      if (fileIndex != -1)
      {
         std::string filename = soundBoard.getFileName(currentPage, fileIndex);
         if (!filename.empty())
         {
            soundPlayer.requestPlayback(filename, SOUND_PRIO_SOUNDBOARD, config.getValue(CFG_SOUNDBOARD_VOLUME));
         }
      }
   }

   return (values.lcdBtnChanged && (values.lcdBtn == BUTTON_UP || values.lcdBtn == BUTTON_DOWN) ? SCREEN_MENU : SCREEN_SOUNDBOARD);
}

void soundBoardScreen_init() {
   soundBoard.begin();
}
