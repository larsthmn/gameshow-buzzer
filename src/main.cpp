#include <Arduino.h>
#include <Wire.h>

// Other LcdMenu includes have to be before LcdMenu.h include (not sure if I support this design choice...)
#define USE_STANDARD_LCD
#include "ItemCommand.h"
#include "ItemInput.h"
#include "ItemToggle.h"
#include "ItemProgress.h"
#include <LcdMenu.h>

#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <SD.h>

#include "pins.h"
#include "sounds.h"

#include "esp_log.h"
#include "inputs.h"
#include "soundboard.h"
#include "config.h"

enum Screen
{
   SCREEN_SOUNDBOARD,
   SCREEN_MENU,
   SCREEN_DEBUG,
   SCREEN_COUNT
};

hd44780_I2Cexp lcd16_2(0x27); // declare lcd object: auto locate & auto config expander chip
LiquidCrystal lcd20_4(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
LcdMenu menu(4, 20);
SoundPlayer soundPlayer;
SoundBoard soundBoard;

static bool requestDebugMenu = false;

#define MAX_TIME_TO_ANSWER 20L
#define MIN_TIME_TO_ANSWER 1L
#define STEP_WIDTH(min, max) map(min + 1, min, max, 0, 1000)

void setBuzzerBeepVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_BEEP_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer beep volume to %d%%\n", config.getValue(CFG_BUZZER_BEEP_VOLUME));
}

void setBuzzerStartVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_START_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer start volume to %d%%\n", config.getValue(CFG_BUZZER_START_VOLUME));
}

void setBuzzerEndVolume(uint16_t pos)
{
   config.setValue(CFG_BUZZER_END_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed buzzer end volume to %d%%\n", config.getValue(CFG_BUZZER_END_VOLUME));
}

void setSoundboardVolume(uint16_t pos)
{
   config.setValue(CFG_SOUNDBOARD_VOLUME, pos / STEP_WIDTH(0, 100)); // 0..1000 => 0..100
   Serial.printf("changed soundboard volume to %d%%\n", config.getValue(CFG_SOUNDBOARD_VOLUME));
}

void setTimeToAnswer(uint16_t pos)
{
   config.setValue(CFG_TIME_TO_ANSWER, pos / STEP_WIDTH(MIN_TIME_TO_ANSWER, MAX_TIME_TO_ANSWER)); // 0..1000 => 1..20
   Serial.printf("changed time to answer to %us\n", config.getValue(CFG_TIME_TO_ANSWER));
}

void callbackDebugMenu()
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


void setup()
{
   Serial.begin(115200);

   // Load settings
   config.load();

   menu.setupLcdWithMenu(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mainMenu);
   menu.hide();

   lcd16_2.begin(16, 2);
   lcd20_4.begin(20, 4);

   // Print a message on both lines of the LCD.
   lcd16_2.setCursor(2, 0);   //Set cursor to character 2 on line 0
   lcd16_2.print("Myje stinkt!");

   lcd16_2.setCursor(2, 1);   //Move cursor to character 2 on line 1
   lcd16_2.print("LOL");

   while (!SD.begin(SS))
   {
      Serial.println("Failed to initialize SD card");
      delay(100);
   }
   Serial.println("SD card initialized successfully");

   soundBoard.begin();
   soundPlayer.begin();

   analogSetAttenuation(ADC_6db);

   pinMode(RED_LED_PIN, OUTPUT);
   pinMode(RED_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_INPUT, INPUT);
   pinMode(RED_BUZZER_INPUT, INPUT);
}


void lightFirstBuzzer(bool red, bool blue, bool reset)
{
   enum State
   {
      STATE_WAITING,
      STATE_ANSWERING,
   };
   static uint32_t lastChange = 0;
   static State state = STATE_WAITING;
   static State prevState = STATE_WAITING;
   static bool lastChoiceSameTime = false;
   int timeToAnswerMs = config.getValue(CFG_TIME_TO_ANSWER) * 1000;
   static uint32_t lastBeepAtTimeLeft = 0;

   switch (state)
   {
      case STATE_WAITING:
         if (red || blue)
         {
            state = STATE_ANSWERING;

            // take the one that was pressed, but if both are pressed take the one
            bool chooseRed = red;
            if (red && blue)
            {
               chooseRed = !lastChoiceSameTime;
               lastChoiceSameTime = chooseRed;
            }
            lcd16_2.clear();
            lcd16_2.setCursor(0, 0);
            if (chooseRed)
            {
               lcd16_2.print("ROT antwortet");
               digitalWrite(RED_BUZZER_LED, HIGH);
               digitalWrite(BLUE_BUZZER_LED, LOW);
            }
            else
            {
               lcd16_2.print("BLAU antwortet");
               digitalWrite(RED_BUZZER_LED, LOW);
               digitalWrite(BLUE_BUZZER_LED, HIGH);
            }


            soundPlayer.requestPlayback(SOUND_TIMER_START, SOUND_PRIO_BUZZER_START, config.getValue(CFG_BUZZER_START_VOLUME));
            lastBeepAtTimeLeft = timeToAnswerMs;
         }
         else
         {
            static uint32_t lastBlink = millis();
            if (millis() - lastBlink > 200)
            {
               digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
               lastBlink = millis();
            }
         }
         break;
      case STATE_ANSWERING:
      {
         uint32_t blockedSinceMs = millis() - lastChange;
         auto timeLeft = (int32_t)(timeToAnswerMs - blockedSinceMs);
         if (lastBeepAtTimeLeft - timeLeft > 1000 && timeLeft >= 800)
         {
            soundPlayer.requestPlayback(SOUND_TIMER_BEEP, SOUND_PRIO_BUZZER_BEEP, config.getValue(CFG_BUZZER_BEEP_VOLUME));
            lastBeepAtTimeLeft -= 1000;
         }


         lcd16_2.setCursor(1, 1);
         lcd16_2.print("Timer: ");
         lcd16_2.print((timeLeft) / 1000.0, 1);
         lcd16_2.print("  ");

         if (reset || timeLeft <= 0)
         {
            state = STATE_WAITING;
            digitalWrite(RED_BUZZER_LED, LOW);
            digitalWrite(BLUE_BUZZER_LED, LOW);

            soundPlayer.requestPlayback(SOUND_TIMER_END, SOUND_PRIO_BUZZER_END, config.getValue(CFG_BUZZER_END_VOLUME));

            lcd16_2.clear();
            lcd16_2.setCursor(1, 0);
            lcd16_2.print("Buzzer offen!");
         }
         break;
      }
      default:
         state = STATE_WAITING;
         break;
   }

   if (prevState != state) lastChange = millis();
   prevState = state;
}


Screen debugScreen(const InputValues& values, bool enter)
{
   static uint32_t lastChange = millis();
   static uint32_t lastUpdate = 0;

   if (enter)
   {
      lastChange = millis();
      lcd20_4.clear();
   }

   if (millis() - lastUpdate > 250)  // updating too fast makes it hard to read
   {
      lcd20_4.setCursor(0, 0);
      lcd20_4.print("Red: ");
      lcd20_4.print(values.isRedBuzzerPressed);
      lcd20_4.print(", ");
      lcd20_4.print("Blue: ");
      lcd20_4.print(values.isBlueBuzzerPressed);

      static ButtonType prevPushBtn = BUTTON_NONE;
      lcd20_4.setCursor(0, 1);
      lcd20_4.print("Pshb:");
      lcd20_4.print(values.readingPushButtons);
      lcd20_4.print("~");
      if (values.pushBtn != prevPushBtn && values.pushBtn != BUTTON_NONE) Serial.println(ButtonTypeStr[values.pushBtn]);
      prevPushBtn = values.pushBtn;
      lcd20_4.print(ButtonTypeStr[values.pushBtn]);
      lcd20_4.print("  ");

      static ButtonType prevLcdBtn = BUTTON_NONE;
      lcd20_4.setCursor(0, 2);
      lcd20_4.print("LCDb:");
      lcd20_4.print(values.readingLcdButtons);
      lcd20_4.print("~");
      if (values.lcdBtn != prevLcdBtn && values.lcdBtn != BUTTON_NONE) Serial.println(ButtonTypeStr[values.lcdBtn]);
      prevLcdBtn = values.lcdBtn;
      lcd20_4.print(ButtonTypeStr[values.lcdBtn]);
      lcd20_4.print("  ");
      lastUpdate = millis();
   }

   return (millis() - lastChange > 5000 && (values.lcdBtnChanged && (values.lcdBtn == BUTTON_LEFT))) ? SCREEN_MENU : SCREEN_DEBUG;
}


Screen soundBoardScreen(const InputValues& values, bool enter)
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
      lcd20_4.clear();
      for (int r = 0; r < 3; ++r)
      {
         lcd20_4.setCursor(0, r);
         lcd20_4.print(soundBoard.getDescription(currentPage, r * 2).c_str());

         lcd20_4.setCursor(9, r);
         lcd20_4.print("|");
         std::string soundRight = soundBoard.getDescription(currentPage, r * 2 + 1);
         lcd20_4.setCursor(20 - soundRight.size(), r);
         lcd20_4.print(soundRight.c_str());
      }
      lcd20_4.setCursor(0, 3);
      lcd20_4.print("<");
      lcd20_4.setCursor(2, 3);
      lcd20_4.print(currentPage + 1);
      lcd20_4.print("/");
      lcd20_4.print(soundBoardPagesCount);
      lcd20_4.print(" ");
      lcd20_4.print(soundBoard.getPageName(currentPage).c_str());
      lcd20_4.setCursor(19, 4);
      lcd20_4.print(">");
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

Screen menuScreen(const InputValues& values, bool enter)
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

// return value: next screen
typedef Screen (* screenFunction)(const InputValues& values, bool enter);

void loop()
{
   static InputValues values = {};
   getInputValues(values);

   // Make sure to keep it in synch with the enum
   screenFunction screenFunctions[SCREEN_COUNT] = {
      soundBoardScreen,
      menuScreen,
      debugScreen,
   };
   static Screen screen = SCREEN_SOUNDBOARD;
   static Screen prevScreen = SCREEN_COUNT;
   bool changed = prevScreen != screen;
   prevScreen = screen;
   screen = screenFunctions[screen](values, changed);

   lightFirstBuzzer(values.isRedBuzzerPressed, values.isBlueBuzzerPressed, values.lcdBtn == BUTTON_LEFT);

   delay(5);
}

