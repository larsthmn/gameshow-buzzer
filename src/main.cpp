#include <Arduino.h>
#include <Wire.h>

#define USE_STANDARD_LCD
#include "ItemCommand.h"
#include "ItemInput.h"
#include "ItemToggle.h"
#include <LcdMenu.h>

#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <cstdint>
#include <cmath>

#include "pins.h"
#include "sounds.h"


hd44780_I2Cexp lcd16_2(0x27); // declare lcd object: auto locate & auto config expander chip
LiquidCrystal lcd20_4(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
//LcdMenu menu(LCD_ROWS, LCD_COLS);
SoundPlayer soundPlayer;


enum ButtonType
{
   BUTTON_NONE,

   // LCD buttons
   BUTTON_UP,
   BUTTON_LEFT,
   BUTTON_DOWN,
   BUTTON_RIGHT,
   BUTTON_ENTER,

   // Push buttons
   BUTTON_YELLOW,
   BUTTON_RED,
   BUTTON_BLACK,
   BUTTON_GREEN,
   BUTTON_WHITE,
   BUTTON_BLUE,

   BUTTON_TYPES_COUNT,
};

const char* ButtonTypeStr[BUTTON_TYPES_COUNT] = {
   "NONE",
   "UP",
   "LEFT",
   "DOWN",
   "RIGHT",
   "ENTER",
   "YELLOW",
   "RED",
   "BLACK",
   "GREEN",
   "WHITE",
   "BLUE"
};

struct ButtonReading
{
   ButtonType type;
   uint16_t reading;
};

const ButtonReading lcdButtons[] = {
   { BUTTON_NONE,  3626 },
   { BUTTON_UP,    2384 },
   { BUTTON_LEFT,  150 },
   { BUTTON_DOWN,  471 },
   { BUTTON_RIGHT, 1700 },
   { BUTTON_ENTER, 1012 },
};

const ButtonReading pushButtons[] = {
   { BUTTON_NONE,   3513 },
   { BUTTON_YELLOW, 1643 },
   { BUTTON_RED,    2278 },
   { BUTTON_BLACK,  0 },
   { BUTTON_GREEN,  1073 },
   { BUTTON_WHITE,  2913 },
   { BUTTON_BLUE,   439 },
};

ButtonType getButtonFromReading(uint16_t reading, const ButtonReading* buttons, std::size_t buttonsLen)
{
   for (std::size_t i = 0; i < buttonsLen; i++)
   {
      if (abs(buttons[i].reading - reading) < 150)
      {
         return buttons[i].type;
      }
   }
   return BUTTON_NONE;
}



void setup()
{
   Serial.begin(115200);

   lcd16_2.begin(16, 2);
   lcd20_4.begin(20, 4);

   // Print a message on both lines of the LCD.
   lcd16_2.setCursor(2, 0);   //Set cursor to character 2 on line 0
   lcd16_2.print("Myje stinkt!");

   lcd16_2.setCursor(2, 1);   //Move cursor to character 2 on line 1
   lcd16_2.print("LOL");

   soundPlayer.start();

//   menu.setupLcdWithMenu(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mainMenu); // Standard
   // Configure ADC
//   analogSetAttenuation(ADC_11db);
   analogSetAttenuation(ADC_6db);

   pinMode(RED_LED_PIN, OUTPUT);
   pinMode(RED_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_INPUT, INPUT);
   pinMode(RED_BUZZER_INPUT, INPUT);
}

class MovingAverage
{
public:
   explicit MovingAverage(int size) : size(size), sum(0), index(0), count(0)
   {
      values = new uint16_t[size];
   }

   ~MovingAverage()
   {
      delete[] values;
   }

   void addValue(uint16_t val)
   {
      if (count < size)
      {
         count++;
      }
      else
      {
         sum -= values[index];
      }
      sum += val;
      values[index] = val;
      index = (index + 1) % size;
   }

   uint16_t average() const
   {
      if (count == 0) return 0;
      return sum / count;
   }

   double standardDeviation() const
   {
      double mean = average();
      double deviationSum = 0.0;
      for (int i = 0; i < count; ++i)
      {
         deviationSum += (values[i] - mean) * (values[i] - mean);
      }
      return sqrt(deviationSum / count);
   }

private:
   uint16_t* values;
   int size;
   uint32_t sum;
   int index;
   int count;
};


class ButtonFilter
{
private:
   ButtonType acceptedButton = BUTTON_NONE;
   ButtonType currentButton = BUTTON_NONE;
   uint32_t buttonPressedSince = 0;
   const ButtonReading* buttons{};
   std::size_t buttonsLen;
   uint32_t acceptAfterMs;

public:
   ButtonFilter(const ButtonReading* buttons, int buttonsLen, int acceptAfter);
   void inputValue(uint16_t value);
   ButtonType getButton();
};

void ButtonFilter::inputValue(uint16_t value)
{
   ButtonType btn = getButtonFromReading(value, buttons, buttonsLen);
   if (currentButton != btn)
   {
      buttonPressedSince = millis();
   }
   currentButton = btn;
   if (millis() - buttonPressedSince > acceptAfterMs) acceptedButton = currentButton;
}

ButtonType ButtonFilter::getButton()
{
   return acceptedButton;
}

ButtonFilter::ButtonFilter(const ButtonReading* buttons, int buttonsLen, int acceptAfter = 50) : buttons(buttons), buttonsLen(
   buttonsLen), acceptAfterMs(acceptAfter)
{
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


            soundPlayer.requestPlayback(SOUND_TIMER_START, 5);
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
            soundPlayer.requestPlayback(SOUND_TIMER_BEEP, 8);
            lastBeepAtTimeLeft -= 1000;
         }


         lcd16_2.setCursor(0, 1);
         lcd16_2.print("Timer: ");
         lcd16_2.print((timeLeft) / 1000.0, 1);
         lcd16_2.print("  ");

         if (reset || timeLeft <= 0)
         {
            state = STATE_WAITING;
            digitalWrite(RED_BUZZER_LED, LOW);
            digitalWrite(BLUE_BUZZER_LED, LOW);

            soundPlayer.requestPlayback(SOUND_TIMER_END, 6);

            lcd16_2.clear();
            lcd16_2.setCursor(0, 0);
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

struct InputValues
{
   uint16_t readingLcdButtons;
   uint16_t readingPushButtons;
   bool isRedBuzzerPressed;
   bool isBlueBuzzerPressed;
   ButtonType pushBtn;
   bool pushBtnChanged;
   ButtonType lcdBtn;
   bool lcdBtnChanged;
};

void getInputValues(InputValues& values);

void debugScreen(const InputValues& values)
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
}

void getInputValues(InputValues& values)
{
   values.readingLcdButtons = analogRead(LCD_BUTTONS_ANALOG_PIN);
   values.readingPushButtons = analogRead(PUSH_BUTTONS_ANALOG_PIN);
   values.isRedBuzzerPressed = !digitalRead(RED_BUZZER_INPUT);
   values.isBlueBuzzerPressed = !digitalRead(BLUE_BUZZER_INPUT);
   
   static ButtonFilter pushBtnFilter = ButtonFilter(pushButtons, sizeof pushButtons / sizeof pushButtons[0]);
   pushBtnFilter.inputValue(values.readingPushButtons);
   ButtonType pushBtn = pushBtnFilter.getButton();
   values.pushBtnChanged  = values.pushBtn != pushBtn;
   if (values.pushBtnChanged)
   {
      Serial.print(ButtonTypeStr[values.pushBtn]);
      Serial.print("->");
      Serial.println(ButtonTypeStr[pushBtn]);
   }
   values.pushBtn = pushBtn;

   static ButtonFilter lcdBtnFilter = ButtonFilter(lcdButtons, sizeof lcdButtons / sizeof lcdButtons[0]);
   lcdBtnFilter.inputValue(values.readingLcdButtons);
   ButtonType lcdBtn = lcdBtnFilter.getButton();
   values.lcdBtnChanged = values.lcdBtn != lcdBtn;
   values.lcdBtn = lcdBtn;
}

void soundBoard(const InputValues& values)
{
   static int displayPage = -1;
   static int currentPage = 0;
   auto& soundBoardPages = soundPlayer.getPages();
   unsigned int soundBoardPagesCount = soundPlayer.getPages().size();
   if (soundBoardPagesCount == 0) return;
   if (values.lcdBtnChanged)
   {
      bool isFirstPage = currentPage == 0;
      bool isLastPage = currentPage == soundBoardPagesCount - 1;
      if (values.lcdBtn == BUTTON_LEFT && !isFirstPage)
      {
         currentPage = (int)((currentPage - 1) % soundBoardPagesCount);
      }
      else if (values.lcdBtn == BUTTON_RIGHT && !isLastPage)
      {
         currentPage = (int)((currentPage + 1) % soundBoardPagesCount);
      }
   }
   if (displayPage != currentPage)
   {
      lcd20_4.clear();
      for (int r = 0; r < 3; ++r)
      {
         lcd20_4.setCursor(0, r);
         if (r * 2 < soundBoardPages[currentPage].files.size())
         {
            std::string soundLeft = soundBoardPages[currentPage].files[r * 2].getDescription();
            lcd20_4.print(soundLeft.c_str());
         }

         lcd20_4.setCursor(9, r);
         lcd20_4.print("|");
         if (r * 2 + 1 < soundBoardPages[currentPage].files.size())
         {
            std::string soundRight = soundBoardPages[currentPage].files[r * 2 + 1].getDescription();
            lcd20_4.setCursor(20 - soundRight.size(), r);
            lcd20_4.print(soundRight.c_str());
         }
      }
      lcd20_4.setCursor(0, 3);
      bool isFirstPage = currentPage == 0;
      bool isLastPage = currentPage == soundBoardPagesCount - 1;
      if (!isFirstPage) lcd20_4.print("<");
      lcd20_4.setCursor(2, 3);
      lcd20_4.print(currentPage + 1);
      lcd20_4.print("/");
      lcd20_4.print(soundBoardPagesCount);
      lcd20_4.print(" ");
      lcd20_4.print(soundBoardPages[currentPage].name.c_str());
      lcd20_4.setCursor(19, 4);
      if (!isLastPage) lcd20_4.print(">");
      displayPage = currentPage;
   }
   if (values.pushBtnChanged)
   {
      switch (values.pushBtn)
      {
         // todo: handle out of bounds if there are < 6 files
         case BUTTON_YELLOW:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[0].getFilename(), 2);
            break;
         case BUTTON_RED:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[1].getFilename(), 2);
            break;
         case BUTTON_BLACK:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[2].getFilename(), 2);
            break;
         case BUTTON_GREEN:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[3].getFilename(), 2);
            break;
         case BUTTON_WHITE:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[4].getFilename(), 2);
            break;
         case BUTTON_BLUE:
            soundPlayer.requestPlayback(soundBoardPages[currentPage].files[5].getFilename(), 2);
            break;
         default:
            break;
      }
   }
}

void loop()
{
   static InputValues values = {};
   getInputValues(values);
//   debugScreen(values);
   soundBoard(values);
   lightFirstBuzzer(values.isRedBuzzerPressed, values.isBlueBuzzerPressed, values.lcdBtn == BUTTON_LEFT);

//   if (lastButtonType == BUTTON_NONE)
//   {
//      switch (btn.type)   {
//         case BUTTON_NONE:
//            break;
//         case BUTTON_UP:
//            menu.up();
//            break;
//         case BUTTON_LEFT:
//            menu.back();
//            break;
//         case BUTTON_DOWN:
//            menu.down();
//            break;
//         case BUTTON_RIGHT:
//            menu.right();
//            break;
//         case BUTTON_ENTER:
//            menu.enter();
//            break;
//      }
//   }
//   lastButtonType = btn.type;

   delay(5);
}

