#include "config.h"
#include <Preferences.h>

static Preferences preferences;
Config config;

struct ConfigDefinition
{
   const char* key;
   int defaultValue;
};
static const ConfigDefinition configDefinitions[CFG_COUNT] = {
   { "buz-beep-vol", 60 },
   { "buz-start-vol", 60 },
   { "buz-end-vol", 60 },
   { "soundboard-vol", 100 },
   { "time-to-answer", 5 }
};


void Config::save()
{
   // Save settings
   Serial.println("Save config");
   preferences.begin("buzzer", false);
   for (int i = 0; i < CFG_COUNT; i++)
   {
      preferences.putInt(configDefinitions[i].key, values[i]);
   }
   preferences.end();
}

void Config::load()
{
   // Load settings
   Serial.println("Load config");
   preferences.begin("buzzer", true);

   for (int i = 0; i < CFG_COUNT; i++)
   {
      values[i] = preferences.getInt(configDefinitions[i].key, configDefinitions[i].defaultValue);
   }
   preferences.end();
}

int Config::defaultValue(ConfigValue cfg)
{
   return configDefinitions[cfg].defaultValue;
}


