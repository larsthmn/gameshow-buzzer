#include "config.h"
#include <Preferences.h>

static Preferences preferences;
Config config;

struct ConfigDefinition
{
   const char* key;
   int defaultValue;
};
ConfigDefinition cfgKeys[CFG_COUNT] = {
   { "buzzer-vol", 60 },
   { "buzzer-en", 1 },
   { "soundboard-vol", 100 }
};


void Config::save()
{
   // Save settings
   Serial.println("Save config");
   preferences.begin("buzzer", false);
   for (int i = 0; i < CFG_COUNT; i++)
   {
      preferences.putInt(cfgKeys[i].key, values[i]);
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
      values[i] = preferences.getInt(cfgKeys[i].key, cfgKeys[i].defaultValue);
   }
   preferences.end();
}
