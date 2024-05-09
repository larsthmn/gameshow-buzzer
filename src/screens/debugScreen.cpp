//
// Created by Lars on 09.05.2024.
//

#include "debugScreen.h"

#include <Arduino.h>

Screen debugScreen(const InputValues& values, LiquidCrystal& lcd, bool enter)
{
   static uint32_t lastChange = millis();
   static uint32_t lastUpdate = 0;

   if (enter)
   {
      lastChange = millis();
      lcd.clear();
   }

   if (millis() - lastUpdate > 250)  // updating too fast makes it hard to read
   {
      lcd.setCursor(0, 0);
      lcd.print("Red: ");
      lcd.print(values.isRedBuzzerPressed);
      lcd.print(", ");
      lcd.print("Blue: ");
      lcd.print(values.isBlueBuzzerPressed);

      static ButtonType prevPushBtn = BUTTON_NONE;
      lcd.setCursor(0, 1);
      lcd.print("Pshb:");
      lcd.print(values.readingPushButtons);
      lcd.print("~");
      if (values.pushBtn != prevPushBtn && values.pushBtn != BUTTON_NONE) Serial.println(ButtonTypeStr[values.pushBtn]);
      prevPushBtn = values.pushBtn;
      lcd.print(ButtonTypeStr[values.pushBtn]);
      lcd.print("  ");

      static ButtonType prevLcdBtn = BUTTON_NONE;
      lcd.setCursor(0, 2);
      lcd.print("LCDb:");
      lcd.print(values.readingLcdButtons);
      lcd.print("~");
      if (values.lcdBtn != prevLcdBtn && values.lcdBtn != BUTTON_NONE) Serial.println(ButtonTypeStr[values.lcdBtn]);
      prevLcdBtn = values.lcdBtn;
      lcd.print(ButtonTypeStr[values.lcdBtn]);
      lcd.print("  ");
      lastUpdate = millis();
   }

   return (millis() - lastChange > 5000 && (values.lcdBtnChanged && (values.lcdBtn == BUTTON_LEFT))) ? SCREEN_MENU : SCREEN_DEBUG;
}
