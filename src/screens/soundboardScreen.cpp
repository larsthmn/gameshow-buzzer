//
// Created by Lars on 09.05.2024.
//

#include "debugScreen.h"
#include "soundboard.h"
#include "sounds.h"
#include "config.h"

SoundBoard soundBoard;

enum SoundboardControlMode
{
   SB_CTRL_SOUNDS,
   SB_CTRL_PAGEJUMP,
};

static inline void displaySoundBoardPage(LiquidCrystal& lcd, int currentPage, int soundBoardPagesCount)
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
   lcd.print("< ");
   lcd.print(currentPage + 1);
   lcd.print("/");
   lcd.print(soundBoardPagesCount);
   lcd.print(":");
   lcd.print(soundBoard.getPageName(currentPage).c_str());
   lcd.setCursor(18, 3);
   lcd.print(" >");
}

/**
 * @brief Get the index corresponding to a push button.
 * *
 * @param values The InputValues struct containing the push button information.
 * @return The index of the corresponding push button. Returns -1 if push button has not changed.
 */
static inline int getIndexFromPushButton(const InputValues& values)
{
   int index = -1;
   if (values.pushBtnChanged)
   {
      switch (values.pushBtn)
      {
         case BUTTON_YELLOW:
            index = 0;
            break;
         case BUTTON_RED:
            index = 1;
            break;
         case BUTTON_BLACK:
            index = 2;
            break;
         case BUTTON_GREEN:
            index = 3;
            break;
         case BUTTON_WHITE:
            index = 4;
            break;
         case BUTTON_BLUE:
            index = 5;
            break;
         default:
            break;
      }
   }
   return index;
}

static inline void playSoundOnButtonPress(const InputValues& values, int currentPage)
{
   int fileIndex = getIndexFromPushButton(values);
   if (fileIndex != -1)
   {
      std::string filename = soundBoard.getFileName(currentPage, fileIndex);
      if (!filename.empty())
      {
         soundPlayer.requestPlayback(filename, SOUND_PRIO_SOUNDBOARD, config.getValue(CFG_SOUNDBOARD_VOLUME));
      }
   }
}

static inline void displayPageOverview(LiquidCrystal& lcd, int range6)
{
   lcd.clear();
   for (int r = 0; r < 3; ++r)
   {
      lcd.setCursor(0, r);
      lcd.print(soundBoard.getPageName(6 * range6 + r * 2).c_str());

      lcd.setCursor(9, r);
      lcd.print("|");
      std::string stringRight = soundBoard.getPageName(6 * range6 + r * 2 + 1);
      lcd.setCursor(20 - stringRight.size(), r);
      lcd.print(stringRight.c_str());
   }

   lcd.setCursor(0, 3);
   lcd.print("< Seiten ");
   lcd.print(range6 * 6);
   lcd.print(" - ");
   lcd.print((range6 + 1) * 6 - 1);
   lcd.setCursor(18, 3);
   lcd.print(" >");

}

Screen soundBoardScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   static SoundboardControlMode controlMode = SB_CTRL_SOUNDS;
   static SoundboardControlMode prevControlMode = SB_CTRL_PAGEJUMP;
   static int displayPage = -1;
   static int displayRange = -1;
   static int currentPage = 0;
   if (enter)
   {
      displayPage = -1; // leads to screen update
      displayRange = -1; // leads to screen update
      controlMode = SB_CTRL_SOUNDS;
   }

   int soundBoardPagesCount = soundBoard.getPageCount();
   if (soundBoardPagesCount == 0) return SCREEN_MENU;

   bool modeChanged = prevControlMode != controlMode;
   prevControlMode = controlMode;
   if (controlMode == SB_CTRL_SOUNDS)
   {
      // Change page by one on left/right
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
         else if (values.lcdBtn == BUTTON_ENTER)
         {
            controlMode = SB_CTRL_PAGEJUMP;
         }
      }
      // Update displayed sounds
      if (displayPage != currentPage || modeChanged)
      {
         displaySoundBoardPage(lcd, currentPage, soundBoardPagesCount);
         displayPage = currentPage;
      }

      playSoundOnButtonPress(values, currentPage);
   }
   else
   {
      static int currentPagesRange = 0; // one range is 6 pages as we can display 6 at once
      int rangeCount = (soundBoardPagesCount + 5) / 6;
      if (values.lcdBtnChanged)
      {
         if (values.lcdBtn == BUTTON_LEFT)
         {
            bool isFirstRange = currentPagesRange == 0;
            currentPagesRange = isFirstRange ? rangeCount - 1 : (currentPagesRange - 1) % rangeCount;
         }
         else if (values.lcdBtn == BUTTON_RIGHT)
         {
            currentPagesRange = (currentPagesRange + 1) % rangeCount;
         }
         else if (values.lcdBtn == BUTTON_ENTER)
         {
            controlMode = SB_CTRL_SOUNDS;
         }
      }
      if (currentPagesRange != displayRange || modeChanged)
      {
         displayPageOverview(lcd, currentPagesRange);

         displayRange = currentPagesRange;
      }

      int pageIndex = getIndexFromPushButton(values);
      if (pageIndex != -1)
      {
         int jumpPage = pageIndex + 6 * currentPagesRange;
         if (jumpPage < soundBoardPagesCount) {
            currentPage = jumpPage;
            controlMode = SB_CTRL_SOUNDS; // After selection, go back to sound play mode
         }
      }
   }

   return (values.lcdBtnChanged && (values.lcdBtn == BUTTON_UP || values.lcdBtn == BUTTON_DOWN) ? SCREEN_MENU : SCREEN_SOUNDBOARD);
}

void soundBoardScreenInit()
{
   soundBoard.begin();
}
