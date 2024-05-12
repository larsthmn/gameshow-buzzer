/*
 * Configuration helper class
 * Sets and gets values (in RAM) and save them to flash automatically on change.
 */

#ifndef ESP32_BUZZER_CONFIG_H
#define ESP32_BUZZER_CONFIG_H

#include <cstdint>
#include <Arduino.h>

enum ConfigValue
{
   CFG_BUZZER_BEEP_VOLUME,
   CFG_BUZZER_START_VOLUME,
   CFG_BUZZER_END_VOLUME,
   CFG_SOUNDBOARD_VOLUME,
   CFG_TIME_TO_ANSWER,
   CFG_SOUND_RANDOM_PERIOD,
   CFG_SOUND_RANDOM_ADD,
   CFG_SOUND_RANDOM_VOLUME,
   CFG_SOUND_RANDOM_ENABLE,
   CFG_SOUND_RANDOM_SELECTION,
   CFG_COUNT
};

enum ConfigType
{
   CFG_TYPE_INT,
   CFG_TYPE_BOOL
};

struct ConfigDefinition
{
   ConfigValue value;
   ConfigType type;
   const char* key;
   int defaultValue;
   int min;
   int max;
   const char* unit;
   const char* name;
};

extern const ConfigDefinition configDef[CFG_COUNT];

class Config
{
private:
   int values[CFG_COUNT];
   bool valuesChanged[CFG_COUNT] = { false };

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
         values[cfg] = max(min(configDef[cfg].max, value), configDef[cfg].min);
         save();
      }
   }

   bool hasChanged(ConfigValue cfg)
   {
      bool ret = valuesChanged[cfg];
      valuesChanged[cfg] = false;
      return ret;
   }

   void resetHasChanged(ConfigValue cfg)
   {
      valuesChanged[cfg] = false;
   }

   void save();
   void load();
   void reset();
};

extern Config config;

#endif //ESP32_BUZZER_CONFIG_H
