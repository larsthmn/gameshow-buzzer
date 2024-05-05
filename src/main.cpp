#include <Arduino.h>
#include <Wire.h>

#define USE_STANDARD_LCD
#include "ItemCommand.h"
#include "ItemInput.h"
#include "ItemToggle.h"
#include <LcdMenu.h>

#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <SD.h>

#include "pins.h"
#include "sounds.h"

#include "esp_log.h"
#include "inputs.h"
#include "soundboard.h"

hd44780_I2Cexp lcd16_2(0x27); // declare lcd object: auto locate & auto config expander chip
LiquidCrystal lcd20_4(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
LcdMenu menu(4, 20);
SoundPlayer soundPlayer;
SoundBoard soundBoard;

struct Config {
   uint8_t buzzerVolume; // 0..100 %
   uint8_t soundBoardVolume; // 0..100 %
   bool enableBuzzerSounds;
};
Config config;

void toggleBuzzerSounds(uint16_t value) {
   config.enableBuzzerSounds = !!value;
}

MAIN_MENU(
   ITEM_TOGGLE("Buzzer Sounds", "ON", "OFF", toggleBuzzerSounds),
   ITEM_BASIC("foobar"),
   ITEM_BASIC("baz"),
   ITEM_BASIC("ddashdjka"),
   ITEM_BASIC("blaa")
);

void setup()
{
   Serial.begin(115200);

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
   const int timeToAnswerMs = 5000; // todo: parameter
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


            soundPlayer.requestPlayback(SOUND_TIMER_START, SOUND_PRIO_BUZZER_START);
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
            soundPlayer.requestPlayback(SOUND_TIMER_BEEP, SOUND_PRIO_BUZZER_BEEP);
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

            soundPlayer.requestPlayback(SOUND_TIMER_END, SOUND_PRIO_BUZZER_END);

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


bool debugScreen(const InputValues& values, bool enter)
{
   static uint32_t lastChange = millis();
   static uint32_t lastUpdate = 0;

   if (enter)  {
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

   return (millis() - lastChange > 3000 && (values.lcdBtnChanged && (values.lcdBtn == BUTTON_UP || values.lcdBtn == BUTTON_DOWN)));
}


bool soundBoardScreen(const InputValues& values, bool enter)
{
   static int displayPage = -1;
   if (enter) displayPage = -1; // leads to screen update
   static int currentPage = 0;
   int soundBoardPagesCount = soundBoard.getPageCount();
   if (soundBoardPagesCount == 0) return true;
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
      if (fileIndex != -1) {
         std::string filename = soundBoard.getFileName(currentPage, fileIndex);
         if (!filename.empty()) soundPlayer.requestPlayback(filename, SOUND_PRIO_SOUNDBOARD);
      }
   }

   return values.lcdBtnChanged && (values.lcdBtn == BUTTON_UP || values.lcdBtn == BUTTON_DOWN);
}

bool menuScreen(const InputValues& values, bool enter)
{
   if (enter) menu.show();
   if (values.lcdBtnChanged)
   {
      switch (values.lcdBtn)   {
         case BUTTON_NONE:
            break;
         case BUTTON_UP:
            menu.up();
            break;
         case BUTTON_LEFT:
            menu.back();
            break;
         case BUTTON_DOWN:
            menu.down();
            break;
         case BUTTON_RIGHT:
            menu.right();
            break;
         case BUTTON_ENTER:
            menu.enter();
            break;
         default:
            break;
      }
   }

   return values.pushBtnChanged;
}

// return value: should exit the screen
typedef bool (*screenFunction)(const InputValues& values, bool enter);

void loop()
{
   static InputValues values = {};
   getInputValues(values);

   screenFunction screenFunctions[] = {
      soundBoardScreen,
      debugScreen,
      menuScreen,
   };
   static uint32_t screen = 0;
   static uint32_t prevScreen = UINT32_MAX;
   bool leaveScreen = screenFunctions[screen](values, prevScreen != screen);
   prevScreen = screen;
   if (leaveScreen) {
      screen = (screen + 1) % (sizeof(screenFunctions) / sizeof (screenFunctions[0]));
   }

   lightFirstBuzzer(values.isRedBuzzerPressed, values.isBlueBuzzerPressed, values.lcdBtn == BUTTON_LEFT);

   delay(5);
}

