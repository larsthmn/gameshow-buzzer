/*
 * Configuration helper class
 * Sets and gets values (in RAM) and save them to flash automatically on change.
 */

#ifndef ESP32_BUZZER_CONFIG_H
#define ESP32_BUZZER_CONFIG_H

#include <cstdint>

enum ConfigValue {
   CFG_BUZZER_VOLUME,
   CFG_BUZZER_ENABLED,
   CFG_SOUNDBOARD_VOLUME,
   CFG_COUNT
};

class Config {
private:
   uint32_t values[CFG_COUNT];

public:
   uint32_t getValue(ConfigValue cfg)
   {
      return values[cfg];
   }

   void setValue(ConfigValue cfg, uint32_t value)
   {
      if (values[cfg] != value)
      {
         values[cfg] = value;
         save();
      }
   }

   void save();
   void load();
};

extern Config config;

#endif //ESP32_BUZZER_CONFIG_H
