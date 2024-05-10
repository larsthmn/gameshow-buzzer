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
   CFG_SOUND_RANDOM_PERIOD,
   CFG_SOUND_RANDOM_ADD,
   CFG_SOUND_RANDOM_VOLUME,
   CFG_COUNT
};

struct ConfigDefinition
{
   ConfigValue value;
   const char* key;
   int defaultValue;
   int min;
   int max;
   const char* unit;
   const char* name;
};

class Config {
private:
   int values[CFG_COUNT];
   bool valuesChanged[CFG_COUNT] = { false};

public:
   int getValue(ConfigValue cfg)
   {
      return values[cfg];
   }

   void setValue(ConfigValue cfg, int value)
   {
      if (values[cfg] != value)
      {
         valuesChanged[cfg] = true;
         values[cfg] = value;
         save();
      }
   }

   bool hasChanged(ConfigValue cfg) {
      bool ret = valuesChanged[cfg];
      valuesChanged[cfg] = false;
      return ret;
   }

   void resetHasChanged(ConfigValue cfg) {
      valuesChanged[cfg] = false;
   }

   void save();
   void load();
};

extern Config config;
extern const ConfigDefinition configDef[CFG_COUNT];

#endif //ESP32_BUZZER_CONFIG_H
