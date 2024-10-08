#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <Arduino.h>
#include <Wire.h>

#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <SD.h>

#include "pins.h"
#include "sounds.h"

#include "inputs.h"
#include "config.h"
#include "screens/screens.h"

#if RUN_FTP
#include <WiFi.h>
#include "ESP-FTP-Server-Lib.h"
#include "credentials.h"

FTPServer ftp;
#endif

static const char* TAG = "main";
hd44780_I2Cexp lcd16_2(0x27); // declare lcd object: auto locate & auto config expander chip
ScreenManager screens;
enum LastDisplayFunction {
   DISPLAY_BUZZER,
   DISPLAY_RANDOM,
   DISPLAY_INIT
};
static LastDisplayFunction lastDisplayFunction = DISPLAY_INIT;
static bool ftpIsInit = false;

void setup()
{
   Serial.begin(115200);

   while (!SD.begin(SS))
   {
      ESP_LOGE(TAG, "Failed to initialize SD card");
      delay(100);
   }
   ESP_LOGI(TAG, "SD card initialized successfully");

   pinMode(RED_LED_PIN, OUTPUT);
   pinMode(RED_BUZZER_LED, OUTPUT);
   pinMode(BLUE_BUZZER_LED, OUTPUT);

   // Load settings
   config.load();

   soundPlayer.begin();

   inputsInit();

   screens.init();

#if RUN_FTP
   // login into WiFi
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

   lcd16_2.begin(16, 2);
   lcd16_2.setCursor(2, 0);
   lcd16_2.print("Myje stinkt");
   lcd16_2.setCursor(5, 1);
   lcd16_2.print("LOL");
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
            lastDisplayFunction = DISPLAY_BUZZER;
            lcd16_2.clear();
            lcd16_2.setCursor(0, 0);
            if (chooseRed)
            {
               lcd16_2.print("ROT antwortet");
               digitalWrite(RED_BUZZER_LED, HIGH);
               digitalWrite(RED_LED_PIN, HIGH);
               digitalWrite(BLUE_BUZZER_LED, LOW);
            }
            else
            {
               lcd16_2.print("BLAU antwortet");
               digitalWrite(RED_BUZZER_LED, LOW);
               digitalWrite(RED_LED_PIN, LOW);
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

         lastDisplayFunction = DISPLAY_BUZZER;
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

            lastDisplayFunction = DISPLAY_BUZZER;
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

void randomSound() {
   static uint32_t clearDisplayAt = 0;
   static uint32_t nextPlay = 0;
   const char* randomSounds[] = SOUNDS_RANDOM;
   const int numberOfSounds = sizeof(randomSounds) / sizeof(randomSounds[0]);

   if (config.hasChanged(CFG_SOUND_RANDOM_PERIOD) || config.hasChanged(CFG_SOUND_RANDOM_ADD)
       || config.hasChanged(CFG_SOUND_RANDOM_ENABLE))
   {
      ESP_LOGI(TAG, "Reinit random sounds due to config change");
      config.resetHasChanged(CFG_SOUND_RANDOM_PERIOD);
      config.resetHasChanged(CFG_SOUND_RANDOM_ADD);
      config.resetHasChanged(CFG_SOUND_RANDOM_ENABLE);
      nextPlay = 0; // re-init
   }

   if (millis() >= nextPlay && config.getValue(CFG_SOUND_RANDOM_ENABLE))
   {
      if (nextPlay != 0) {
         ESP_LOGI(TAG, "Play random sound");
         soundPlayer.requestPlayback(randomSounds[config.getValue(CFG_SOUND_RANDOM_SELECTION) % numberOfSounds], SOUND_PRIO_RANDOM,
                                     config.getValue(CFG_SOUND_RANDOM_VOLUME));

         lastDisplayFunction = DISPLAY_RANDOM;
         lcd16_2.clear();
         lcd16_2.setCursor(0, 0);
         lcd16_2.write("Schuettet was in");
         lcd16_2.setCursor(0, 1);
         lcd16_2.write("eure Fressluke!");
         clearDisplayAt = millis() + 30000;
      }
      int32_t periodMs = config.getValue(CFG_SOUND_RANDOM_PERIOD) * 60 * 1000L;
      int32_t randomOffsetMs = random(0, config.getValue(CFG_SOUND_RANDOM_ADD) * 60 * 1000L);
      int32_t nextOffset = max(5000, periodMs + randomOffsetMs);
      nextPlay = millis() + nextOffset;
      ESP_LOGI(TAG, "Next random sound in %.2fs (%.2fs + %.2fs)", nextOffset / 1000.0, periodMs / 1000.0,
                    randomOffsetMs / 1000.0);
   }

   if (clearDisplayAt != 0 && millis() > clearDisplayAt)
   {
      // Only clear display if nothing else has written something there in between
      if (lastDisplayFunction == DISPLAY_RANDOM)
      {
         lcd16_2.clear();
         lcd16_2.setCursor(2, 0);
         lcd16_2.write("Vielen Dank,");
         lcd16_2.setCursor(0, 1);
         lcd16_2.write("gerne wieder! 5*");
      }
      clearDisplayAt = 0;
   }
}


void loop()
{
#if RUN_FTP
   if (!ftpIsInit && WiFi.status() == WL_CONNECTED)
   {
      ESP_LOGI(TAG, "Connected to Wifi + init FTP");
      ftp.addUser(FTP_USER, FTP_PASSWORD);
      ftp.addFilesystem("SD", &SD);
      ftp.begin();
      ftpIsInit = true;
   }

   if (ftpIsInit)
   {
      ftp.handle();
   }
#endif

   static InputValues values = {};
   getInputValues(values);

   screens.loop(values);

   lightFirstBuzzer(values.isRedBuzzerPressed, values.isBlueBuzzerPressed, false);

   randomSound();

   delay(5);
}

