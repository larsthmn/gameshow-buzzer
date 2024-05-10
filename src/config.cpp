#include "config.h"
#include <Preferences.h>

static Preferences preferences;
Config config;

const ConfigDefinition configDef[CFG_COUNT] = {
   { .value = CFG_BUZZER_BEEP_VOLUME, .key = "buz-beep-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrBeep vol" },
   { .value = CFG_BUZZER_START_VOLUME, .key = "buz-start-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrStart vol" },
   { .value = CFG_BUZZER_END_VOLUME, .key = "buz-end-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrEnd vol" },
   { .value = CFG_SOUNDBOARD_VOLUME, .key = "soundboard-vol", .defaultValue = 100, .min = 0, .max = 100, .unit = "%", .name = "Soundb vol" },
   { .value = CFG_TIME_TO_ANSWER, .key = "time-to-answer", .defaultValue = 5, .min = 1, .max = 20, .unit = "s", .name = "Answer time" },
   { .value = CFG_SOUND_RANDOM_PERIOD, .key = "rs-period", .defaultValue = 30, .min = 0, .max = 100, .unit = "min", .name = "RandSnd freq" },
   { .value = CFG_SOUND_RANDOM_ADD, .key = "rs-random", .defaultValue = 10, .min = 0, .max = 30, .unit = "min", .name = "RandSnd add" },
   { .value = CFG_SOUND_RANDOM_VOLUME, .key = "rs-vol", .defaultValue = 100, .min = 0, .max = 100, .unit = "%", .name = "RandSnd vol" },
};


void Config::save()
{
   // Save settings
   Serial.println("Save config");
   preferences.begin("buzzer", false);
   for (int i = 0; i < CFG_COUNT; i++)
   {
      preferences.putInt(configDef[i].key, values[i]);
   }
   preferences.end();
}

void Config::load()
{
   // Load settings
   Serial.println("Load config");
   preferences.begin("buzzer", false);

   for (int i = 0; i < CFG_COUNT; i++)
   {
      values[i] = preferences.getInt(configDef[i].key, configDef[i].defaultValue);
      // check if value is valid and reset if not
      if (values[i] > configDef[i].max || values[i] < configDef[i].min) {
         values[i] = configDef[i].defaultValue;
         preferences.putInt(configDef[i].key, values[i]);
      }
   }
   preferences.end();
}



