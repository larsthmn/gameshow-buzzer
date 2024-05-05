/*
 * @brief User inputs
 */

#include "inputs.h"
#include "pins.h"
#include <Arduino.h>

struct ButtonReading
{
   ButtonType type;
   uint16_t reading;
};

// ADC values corresponding to button value
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

static ButtonType getButtonFromReading(uint16_t reading, const ButtonReading* buttons, std::size_t buttonsLen)
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


/**
 * @class ButtonFilter
 * @brief The ButtonFilter class filters button input and returns the accepted button.
 *
 * The ButtonFilter class filters out noise and debounce button input to provide a reliable
 * and accurate button press detection.
 */
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
   if (values.lcdBtnChanged)
   {
      Serial.print(ButtonTypeStr[values.lcdBtn]);
      Serial.print("->");
      Serial.println(ButtonTypeStr[lcdBtn]);
   }
   values.lcdBtn = lcdBtn;
}

