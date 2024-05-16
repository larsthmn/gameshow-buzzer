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

/**
 * @brief Get the page description for a given sequence of button presses. (like "Page 1 - 5" or "foobar")
 * @param sequences A pointer to an array of integers representing button presses.
 * @return The page description as a string. Returns an empty string if an error occurs.
 */
static std::string getPageDescriptionForSequence(const int* sequences)
{
   int minPage, maxPage;
   if (soundBoard.getPageRangeFromSequence(sequences, minPage, maxPage) < 0)
   {
      return "";
   }
   else if (minPage == maxPage)
   {
      // Button would give a single page
      return soundBoard.getPageName(minPage);
   }
   else
   {
      // add one to indices since human beings start counting from 1
      std::string s = "Page " + std::to_string(minPage + 1) + "-" + std::to_string(maxPage + 1);
      return s;
   }
}

static inline void displayPageJump(LiquidCrystal& lcd, const int* pressedSequence)
{
   lcd.clear();
   int sequences[MAX_QUICKACCESS_LEN];
   int nextPressIndex = -1;
   for (int i = 0; i < MAX_QUICKACCESS_LEN; ++i)
   {
      sequences[i] = pressedSequence[i];
      if (nextPressIndex == -1 && pressedSequence[i] == -1) nextPressIndex = i;
   }
   for (int r = 0; r < 3; ++r)
   {
      lcd.setCursor(0, r);
      sequences[nextPressIndex] = r * 2;
      lcd.print(getPageDescriptionForSequence(sequences).c_str());

      lcd.setCursor(9, r);
      lcd.print("|");

      sequences[nextPressIndex] = r * 2 + 1;
      std::string stringRight = getPageDescriptionForSequence(sequences);
      lcd.setCursor(20 - stringRight.size(), r);
      lcd.print(stringRight.c_str());
   }

   lcd.setCursor(0, 3);
   lcd.print(" Wait for button");
}

/**
 * @brief Adds a button index to the pressed button sequence.
 *
 * @param[in,out] pressedButtonSequence The pressed button sequence array.
 * @param[in] buttonIndex The button index to be added to the sequence.
 *
 * @return 0 if the button index was successfully added, -1 otherwise.
 */
static int addToSequence(int* pressedButtonSequence, int buttonIndex)
{
   for (int i = 0; i < MAX_QUICKACCESS_LEN; i++)
   {
      if (pressedButtonSequence[i] == -1)
      {
         pressedButtonSequence[i] = buttonIndex;
         return 0;
      }
   }
   return -1;
}

static void jumpToPageBySequence(const int* pressedButtonSequence, int soundBoardPagesCount, int& currentPage,
                          SoundboardControlMode& controlMode)
{
   // Get pages that are given by this sequence and jump if it's only one
   int minPage, maxPage;
   int rc = soundBoard.getPageRangeFromSequence(pressedButtonSequence, minPage, maxPage);
   if (rc < 0)
   {
      // Invalid sequence, just go back to current sound page
      Serial.println("Invalid sequence");
      controlMode = SB_CTRL_SOUNDS; // After selection, go back to sound play mode
   }
   else if (minPage == maxPage)
   {
      // Sequence represents a unique page
      if (minPage < soundBoardPagesCount && minPage >= 0)
      {
         // Valid page, go to it
         currentPage = minPage;
      }
      else
      {
         // Invalid page (this should not happen tho)
         Serial.println("Invalid jump page from sequence");
      }
      controlMode = SB_CTRL_SOUNDS; // After selection, go back to sound play mode
   }
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

/**
 * @brief Display the soundboard screen.
 *    Displays soundboard pages with sounds that can be played on a button press.
 *    Offers the ability to quick access pages with a sequence of push button presses.
 * @param values Input values
 * @param lcd LCD instance
 * @param enter Flag if screen was entered and needs to be updated.
 * @return Next screen
 */
Screen soundBoardScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   static SoundboardControlMode controlMode = SB_CTRL_SOUNDS;
   static SoundboardControlMode prevControlMode = SB_CTRL_PAGEJUMP;
   static int displayPage = -1;
   static int currentPage = 0;
   static int pressedButtonSequence[MAX_QUICKACCESS_LEN] = { -1 };
   static int displayButtonSequence[MAX_QUICKACCESS_LEN] = { INT32_MAX };
   if (enter)
   {
      displayPage = -1; // leads to screen update
      displayButtonSequence[0] = INT32_MAX;
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
            std::fill_n(pressedButtonSequence, MAX_QUICKACCESS_LEN, -1);
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
      if (values.lcdBtnChanged)
      {
         if (values.lcdBtn == BUTTON_ENTER)
         {
            controlMode = SB_CTRL_SOUNDS;
         }
      }
      if (memcmp(pressedButtonSequence, displayButtonSequence, sizeof pressedButtonSequence) != 0 || modeChanged)
      {
         displayPageJump(lcd, pressedButtonSequence);
         memcpy(displayButtonSequence, pressedButtonSequence, sizeof pressedButtonSequence);
      }

      int buttonIndex = getIndexFromPushButton(values);
      if (buttonIndex != -1)
      {
         addToSequence(pressedButtonSequence, buttonIndex);
         jumpToPageBySequence(pressedButtonSequence, soundBoardPagesCount, currentPage, controlMode);
      }
   }

   return (values.lcdBtnChanged && (values.lcdBtn == BUTTON_UP || values.lcdBtn == BUTTON_DOWN) ? SCREEN_MENU : SCREEN_SOUNDBOARD);
}

void soundBoardScreenInit()
{
   soundBoard.begin();
}
