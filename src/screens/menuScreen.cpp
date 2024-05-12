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
#include "ItemSubMenu.h"
#include <ItemList.h>
#include "soundboardScreen.h"
#include "sounds.h"
#include <LcdMenu.h>

LcdMenu menu(4, 20);
static bool requestDebugMenu = false;
void updateMenuValues();

static void callbackDebugMenu()
{
   requestDebugMenu = true;
}

static void callbackReset()
{
   config.reset();
   updateMenuValues();
}

static void callbackRefreshSoundboard()
{
   soundBoardScreenInit();
}

#define mapFloat(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
#define STEP_WIDTH(min, max) mapFloat(min + 1.0, min, max, 0.0, 1000.0)

char* mapToConfig(uint16_t progress, ConfigValue cfg)
{
   int mapped = (int)round(mapProgress(progress, (float)configDef[cfg].min, (float)configDef[cfg].max));
   static char buffer[16];
   itoa(mapped, buffer, 10);
   concat(buffer, configDef[cfg].unit, buffer);
   return buffer;
}

static void setConfigInt(uint16_t pos, ConfigValue cfg)
{
   int mapped = (int)round(mapProgress(pos, (float)configDef[cfg].min, (float)configDef[cfg].max));
   config.setValue(cfg, mapped);
   Serial.printf("changed %s to %u%s\n", configDef[cfg].name, config.getValue(cfg), configDef[cfg].unit);
}

static void setConfigIntNoMap(uint16_t pos, ConfigValue cfg)
{
   config.setValue(cfg, pos);
   Serial.printf("changed %s to %u%s\n", configDef[cfg].name, config.getValue(cfg), configDef[cfg].unit);
}

static void setConfigBool(uint16_t pos, ConfigValue cfg)
{
   config.setValue(cfg, !!pos);
   Serial.printf("changed %s to %s\n", configDef[cfg].name, pos ? "on" : "off");
}

#define ITEM_CONFIG_PROGRESS(text, cfg, stepwidth)                                                               \
      ITEM_PROGRESS(text,                                                                                        \
                     (uint16_t)round(config.getValue(cfg) * STEP_WIDTH(configDef[cfg].min, configDef[cfg].max)), \
                     (uint8_t)round(stepwidth * STEP_WIDTH(configDef[cfg].min, configDef[cfg].max)),             \
                     [](uint16_t prog){ return mapToConfig(prog, cfg);},                                         \
                     [](uint16_t prog){setConfigInt(prog, cfg);})


#define ITEM_CONFIG_TOGGLE(text, cfg)  \
      ITEM_TOGGLE(text, "JA", "NEIN", [](uint16_t prog){setConfigBool(prog, cfg);})

extern MenuItem* buzzerMenu[];
extern MenuItem* randomSoundMenu[];
extern MenuItem* soundboardMenu[];

// Progress items have an internal range of 0..1000
MAIN_MENU(
   ITEM_SUBMENU("Buzzer", buzzerMenu),
   ITEM_SUBMENU("Random sounds", randomSoundMenu),
   ITEM_SUBMENU("Soundboard", soundboardMenu),
   ITEM_COMMAND("Reset", callbackReset),
   ITEM_COMMAND("Debug", callbackDebugMenu)
);

SUB_MENU(buzzerMenu, mainMenu,
         ITEM_CONFIG_PROGRESS("Beep Vol", CFG_BUZZER_BEEP_VOLUME, 5),
         ITEM_CONFIG_PROGRESS("Start Vol.", CFG_BUZZER_START_VOLUME, 5),
         ITEM_CONFIG_PROGRESS("End Vol.", CFG_BUZZER_END_VOLUME, 5),
         ITEM_CONFIG_PROGRESS("Antwortzeit", CFG_TIME_TO_ANSWER, 1)
);

const String randomSounds[] = SOUNDS_RANDOM_NAMES;
const uint8_t randomSoundsCount = sizeof randomSounds / sizeof randomSounds[0];
SUB_MENU(randomSoundMenu, mainMenu,
         ITEM_CONFIG_TOGGLE("Aktivieren", CFG_SOUND_RANDOM_ENABLE),
         ITEM_CONFIG_PROGRESS("Periode", CFG_SOUND_RANDOM_PERIOD, 1),
         ITEM_CONFIG_PROGRESS("Zus. Zufall", CFG_SOUND_RANDOM_ADD, 1),
         ITEM_CONFIG_PROGRESS("Volume", CFG_SOUND_RANDOM_VOLUME, 5),
         ITEM_STRING_LIST("Sound", const_cast<String*>(randomSounds), randomSoundsCount, [](uint16_t prog)
         { setConfigIntNoMap(prog, CFG_SOUND_RANDOM_SELECTION); })
);

SUB_MENU(soundboardMenu, mainMenu,
         ITEM_CONFIG_PROGRESS(configDef[CFG_SOUNDBOARD_VOLUME].name, CFG_SOUNDBOARD_VOLUME, 5),
         ITEM_COMMAND("Soundboard akt.", callbackRefreshSoundboard)
);

void menuInit()
{
   menu.setupLcdWithMenu(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mainMenu);
   menu.hide();
}

static void setProgressFromCfg(MenuItem* menuItem, ConfigValue cfg)
{
   Serial.printf("%s: cfg: %d, step: %f => %f\n", configDef[cfg].name, config.getValue(CFG_TIME_TO_ANSWER),
                 STEP_WIDTH(configDef[CFG_TIME_TO_ANSWER].min, configDef[CFG_TIME_TO_ANSWER].max),
                 config.getValue(CFG_TIME_TO_ANSWER)
                 * STEP_WIDTH(configDef[CFG_TIME_TO_ANSWER].min, configDef[CFG_TIME_TO_ANSWER].max));

   menuItem->setProgress(
      (uint16_t)round((config.getValue(cfg) - configDef[cfg].min) * STEP_WIDTH(configDef[cfg].min, configDef[cfg].max)));
}


void updateMenuValues()
{
   // Load config values to menu - Make sure indices comply with the Main / Sub menu definition!
   setProgressFromCfg(buzzerMenu[1], CFG_BUZZER_BEEP_VOLUME);
   setProgressFromCfg(buzzerMenu[2], CFG_BUZZER_START_VOLUME);
   setProgressFromCfg(buzzerMenu[3], CFG_BUZZER_END_VOLUME);
   setProgressFromCfg(buzzerMenu[4], CFG_TIME_TO_ANSWER);
   randomSoundMenu[1]->setIsOn(config.getValue(CFG_SOUND_RANDOM_ENABLE));
   setProgressFromCfg(randomSoundMenu[2], CFG_SOUND_RANDOM_PERIOD);
   setProgressFromCfg(randomSoundMenu[3], CFG_SOUND_RANDOM_ADD);
   setProgressFromCfg(randomSoundMenu[4], CFG_SOUND_RANDOM_VOLUME);
   randomSoundMenu[5]->setItemIndex(config.getValue(CFG_SOUND_RANDOM_SELECTION));
   setProgressFromCfg(soundboardMenu[1], CFG_SOUNDBOARD_VOLUME);
}

Screen menuScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   if (enter)
   {
      updateMenuValues();
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
