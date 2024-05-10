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
#include "soundboardScreen.h"
#include <LcdMenu.h>

LcdMenu menu(4, 20);
static bool requestDebugMenu = false;

static void callbackDebugMenu()
{
   requestDebugMenu = true;
}

static void callbackRefreshSoundboard()
{
   soundBoardScreenInit();
}

#define STEP_WIDTH(min, max) map(min + 1, min, max, 0, 1000)

char* mapToConfig(uint16_t progress, ConfigValue cfg)
{
   long mapped = mapProgress(progress, (long)configDef[cfg].min, (long)configDef[cfg].max);
   static char buffer[16];
   itoa(mapped, buffer, 10);
   concat(buffer, configDef[cfg].unit, buffer);
   return buffer;
}

static void setConfig(uint16_t pos, ConfigValue cfg)
{
   config.setValue(cfg, pos / STEP_WIDTH(configDef[cfg].min, configDef[cfg].max));
   Serial.printf("changed %s to %u%s\n", configDef[cfg].name, config.getValue(cfg), configDef[cfg].unit);
}

#define ITEM_CONFIG_PROGRESS(text, cfg, stepwidth)  \
      ITEM_PROGRESS(text,                           \
                     configDef[cfg].defaultValue * STEP_WIDTH(configDef[cfg].min, configDef[cfg].max), \
                     stepwidth * STEP_WIDTH(configDef[cfg].min, configDef[cfg].max),                   \
                     [](uint16_t prog){ return mapToConfig(prog, cfg);},                                        \
                     [](uint16_t prog){setConfig(prog, cfg);})


// Progress items have an internal range of 0..1000
MAIN_MENU(
   // Make sure this is exactly the order of the enum (values are loaded in a loop to the entries)
   ITEM_CONFIG_PROGRESS(configDef[CFG_BUZZER_BEEP_VOLUME].name, CFG_BUZZER_BEEP_VOLUME, 5),
   ITEM_CONFIG_PROGRESS(configDef[CFG_BUZZER_START_VOLUME].name, CFG_BUZZER_START_VOLUME, 5),
   ITEM_CONFIG_PROGRESS(configDef[CFG_BUZZER_END_VOLUME].name, CFG_BUZZER_END_VOLUME, 5),
   ITEM_CONFIG_PROGRESS(configDef[CFG_SOUNDBOARD_VOLUME].name, CFG_SOUNDBOARD_VOLUME, 5),
   ITEM_CONFIG_PROGRESS(configDef[CFG_TIME_TO_ANSWER].name, CFG_TIME_TO_ANSWER, 1),
   ITEM_CONFIG_PROGRESS(configDef[CFG_SOUND_RANDOM_PERIOD].name, CFG_SOUND_RANDOM_PERIOD, 1),
   ITEM_CONFIG_PROGRESS(configDef[CFG_SOUND_RANDOM_ADD].name, CFG_SOUND_RANDOM_ADD, 1),
   ITEM_CONFIG_PROGRESS(configDef[CFG_SOUND_RANDOM_VOLUME].name, CFG_SOUND_RANDOM_VOLUME, 5),
   ITEM_COMMAND("Debug", callbackDebugMenu),
   ITEM_COMMAND("Refresh soundboard", callbackRefreshSoundboard)
);


void menuInit()
{
   menu.setupLcdWithMenu(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mainMenu);
   menu.hide();
}


Screen menuScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   if (enter)
   {
      // Load config values to menu - Use indices from MAIN_MENU definition! Index 0 is header
      for (int i = 0; i < static_cast<int>(ConfigValue::CFG_COUNT); ++i)
      {
         auto cfg = static_cast<ConfigValue>(i);
         mainMenu[i + 1]->setProgress(config.getValue(cfg) * STEP_WIDTH(configDef[i].min, configDef[i].max));

      }
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
