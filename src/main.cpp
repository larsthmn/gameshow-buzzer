#include <Arduino.h>
#include <Wire.h>

#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <SD.h>

#include "pins.h"
#include "sounds.h"

#include "esp_log.h"
#include "inputs.h"
#include "config.h"
#include "screens/screens.h"

#define RUN_FTP 1

#if RUN_FTP
#include <WiFi.h>
#include "ESP-FTP-Server-Lib.h"
#include "credentials.h"

FTPServer ftp;
#endif

hd44780_I2Cexp lcd16_2(0x27); // declare lcd object: auto locate & auto config expander chip
ScreenManager screens;

void setup()
{
   Serial.begin(115200);

   while (!SD.begin(SS))
   {
      Serial.println("Failed to initialize SD card");
      delay(100);
   }
   Serial.println("SD card initialized successfully");

   pinMode(RED_LED_PIN, OUTPUT);
   pinMode(RED_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_INPUT, INPUT);
   pinMode(RED_BUZZER_INPUT, INPUT);

   // Load settings
   config.load();

   lcd16_2.begin(16, 2);
   lcd16_2.setCursor(2, 0);   //Set cursor to character 2 on line 0
   lcd16_2.print("Myje stinkt!");
   lcd16_2.setCursor(2, 1);   //Move cursor to character 2 on line 1
   lcd16_2.print("LOL");

   soundPlayer.begin();

   analogSetAttenuation(ADC_6db);

   screens.init();

#if RUN_FTP
   // login into WiFi
   // Change needed!
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   while (WiFi.status() != WL_CONNECTED) {
      delay(10);
   }
   Serial.println("Connected to Wifi!");

   ftp.addUser(FTP_USER, FTP_PASSWORD);
   ftp.addFilesystem("SD", &SD);
   ftp.begin();
#endif
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


void loop()
{
#if RUN_FTP
   ftp.handle();
#endif

   static InputValues values = {};
   getInputValues(values);

   screens.loop(values);

   lightFirstBuzzer(values.isRedBuzzerPressed, values.isBlueBuzzerPressed, values.lcdBtn == BUTTON_LEFT);

   delay(5);
}

