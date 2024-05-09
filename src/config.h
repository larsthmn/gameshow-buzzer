/*
 * Configuration helper class
 * Sets and gets values (in RAM) and save them to flash automatically on change.
 */

#ifndef ESP32_BUZZER_CONFIG_H
#define ESP32_BUZZER_CONFIG_H

#include <cstdint>

enum ConfigValue {
   CFG_BUZZER_BEEP_VOLUME,
   CFG_BUZZER_START_VOLUME,
   CFG_BUZZER_END_VOLUME,
   CFG_SOUNDBOARD_VOLUME,
   CFG_TIME_TO_ANSWER,
   CFG_COUNT
};

class Config {
private:
   int values[CFG_COUNT];

public:
   int getValue(ConfigValue cfg)
   {
      return values[cfg];
   }

   void setValue(ConfigValue cfg, int value)
   {
      if (values[cfg] != value)
      {
         values[cfg] = value;
         save();
      }
   }

   static int defaultValue(ConfigValue cfg);

   void save();
   void load();
};

extern Config config;

#endif //ESP32_BUZZER_CONFIG_H
