//
// Created by Lars on 09.05.2024.
//

#include "menuScreen.h"
#include "config.h"
#include "pins.h"

// Other LcdMenu includes have to be before LcdMenu.h include (not sure if I support this design choice...)
#define USE_STANDARD_LCD
#include "ItemCommand.h"
#include "ItemInput.h"
#include "ItemToggle.h"
#include "ItemProgress.h"
#include <LcdMenu.h>

#define MAX_TIME_TO_ANSWER 20L
#define MIN_TIME_TO_ANSWER 1L
#define STEP_WIDTH(min, max) map(min + 1, min, max, 0, 1000)

LcdMenu menu(4, 20);
static bool requestDebugMenu = false;

static void setBuzzerBeepVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_BEEP_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer beep volume to %d%%\n", config.getValue(CFG_BUZZER_BEEP_VOLUME));
}

static void setBuzzerStartVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_START_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer start volume to %d%%\n", config.getValue(CFG_BUZZER_START_VOLUME));
}

static void setBuzzerEndVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_END_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer end volume to %d%%\n", config.getValue(CFG_BUZZER_END_VOLUME));
}

static void setSoundboardVolume(uint16_t pos)
{
   config.setValue(CFG_SOUNDBOARD_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed soundboard volume to %d%%\n", config.getValue(CFG_SOUNDBOARD_VOLUME));
}

static void setTimeToAnswer(uint16_t pos)
{
   config.setValue(CFG_TIME_TO_ANSWER, pos / STEP_WIDTH(MIN_TIME_TO_ANSWER, MAX_TIME_TO_ANSWER)); // 0..1000 => 1..20
   Serial.printf("changed time to answer to %us\n", config.getValue(CFG_TIME_TO_ANSWER));
}

static void callbackDebugMenu()
{
   requestDebugMenu = true;
}

char* mapToPercent(uint16_t progress)
{
   long mapped = mapProgress(progress, 0, 100L);
   static char buffer[10];
   itoa(mapped, buffer, 10);
   concat(buffer, '%', buffer);
   return buffer;
}

char* mapToSecond(uint16_t progress)
{
   long mapped = mapProgress(progress, 0, 20L);
   static char buffer[10];
   itoa(mapped, buffer, 10);
   concat(buffer, 's', buffer);
   return buffer;
}

// Progress items have an internal range of 0..1000
#define STEP_WIDTH_PERCENT 5
MAIN_MENU(
// Adjust setting the values from config values if altering the order!
   ITEM_PROGRESS("BzrBeep vol", Config::defaultValue(CFG_BUZZER_BEEP_VOLUME) * STEP_WIDTH(0, 100),
                 STEP_WIDTH_PERCENT * STEP_WIDTH(0, 100), mapToPercent, setBuzzerBeepVolume),
   ITEM_PROGRESS("BzrStart vol", Config::defaultValue(CFG_BUZZER_START_VOLUME) * STEP_WIDTH(0, 100),
                 STEP_WIDTH_PERCENT * STEP_WIDTH(0, 100), mapToPercent, setBuzzerStartVolume),
   ITEM_PROGRESS("BzrEnd vol", Config::defaultValue(CFG_BUZZER_END_VOLUME) * STEP_WIDTH(0, 100),
                 STEP_WIDTH_PERCENT * STEP_WIDTH(0, 100), mapToPercent, setBuzzerEndVolume),
   ITEM_PROGRESS("Soundb. vol", Config::defaultValue(CFG_SOUNDBOARD_VOLUME) * STEP_WIDTH(0, 100),
                 STEP_WIDTH_PERCENT * STEP_WIDTH(0, 100), mapToPercent, setSoundboardVolume),
   ITEM_PROGRESS("Answer time", Config::defaultValue(CFG_TIME_TO_ANSWER) * STEP_WIDTH(MIN_TIME_TO_ANSWER, MAX_TIME_TO_ANSWER),
                 STEP_WIDTH(MIN_TIME_TO_ANSWER, MAX_TIME_TO_ANSWER), mapToSecond, setTimeToAnswer),
   ITEM_COMMAND("Debug", callbackDebugMenu)
);


void menu_init()
{
   menu.setupLcdWithMenu(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mainMenu);
   menu.hide();
}


Screen menuScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   if (enter)
   {
      // Load config values to menu - Use indices from MAIN_MENU definition! Index 0 is header
      mainMenu[1]->setProgress(config.getValue(CFG_BUZZER_BEEP_VOLUME) * STEP_WIDTH(0, 100));
      mainMenu[2]->setProgress(config.getValue(CFG_BUZZER_START_VOLUME) * STEP_WIDTH(0, 100));
      mainMenu[3]->setProgress(config.getValue(CFG_BUZZER_END_VOLUME) * STEP_WIDTH(0, 100));
      mainMenu[4]->setProgress(config.getValue(CFG_SOUNDBOARD_VOLUME) * STEP_WIDTH(0, 100));
      mainMenu[5]->setProgress(config.getValue(CFG_TIME_TO_ANSWER) * STEP_WIDTH(MIN_TIME_TO_ANSWER, MAX_TIME_TO_ANSWER));
      menu.show();
   }
   if (values.lcdBtnChanged)
   {

      switch (values.lcdBtn)
      {
         case BUTTON_NONE:
            break;
         case BUTTON_UP:
            // Change values with up/down instead of left/right as we use left as back button
            if (menu.isInEditMode())
            {
               menu.right();
            }
            else
            {
               menu.up();
            }
            break;
         case BUTTON_LEFT:
            menu.back();
            break;
         case BUTTON_DOWN:
            if (menu.isInEditMode())
            {
               menu.left();
            }
            else
            {
               menu.down();
            }
            break;
         case BUTTON_RIGHT:
            if (!menu.isInEditMode()) menu.right();
            break;
         case BUTTON_ENTER:
            menu.enter();
            break;
         default:
            break;
      }
   }

   if (requestDebugMenu)
   {
      requestDebugMenu = false;
      return SCREEN_DEBUG;
   }

   return values.pushBtnChanged ? SCREEN_SOUNDBOARD : SCREEN_MENU;
}
