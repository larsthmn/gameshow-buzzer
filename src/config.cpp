#include "config.h"
#include <Preferences.h>

static Preferences preferences;
Config config;

const ConfigDefinition configDef[CFG_COUNT] = {
   { .value = CFG_BUZZER_BEEP_VOLUME, .type = CFG_TYPE_INT, .key = "buz-beep-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrBeep vol" },
   { .value = CFG_BUZZER_START_VOLUME, .type = CFG_TYPE_INT, .key = "buz-start-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrStart vol" },
   { .value = CFG_BUZZER_END_VOLUME, .type = CFG_TYPE_INT, .key = "buz-end-vol", .defaultValue = 60, .min = 0, .max = 100, .unit = "%", .name = "BzrEnd vol" },
   { .value = CFG_SOUNDBOARD_VOLUME, .type = CFG_TYPE_INT, .key = "soundboard-vol", .defaultValue = 100, .min = 0, .max = 100, .unit = "%", .name = "Soundb vol" },
   { .value = CFG_TIME_TO_ANSWER, .type = CFG_TYPE_INT, .key = "time-to-answer", .defaultValue = 5, .min = 1, .max = 21, .unit = "s", .name = "Answer time" },
   { .value = CFG_SOUND_RANDOM_PERIOD, .type = CFG_TYPE_INT, .key = "rs-period", .defaultValue = 30, .min = 0, .max = 200, .unit = "min", .name = "RandSnd freq" },
   { .value = CFG_SOUND_RANDOM_ADD, .type = CFG_TYPE_INT, .key = "rs-random", .defaultValue = 10, .min = 0, .max = 125, .unit = "min", .name = "RandSnd add" },
   { .value = CFG_SOUND_RANDOM_VOLUME, .type = CFG_TYPE_INT, .key = "rs-vol", .defaultValue = 100, .min = 0, .max = 100, .unit = "%", .name = "RandSnd vol" },
   { .value = CFG_SOUND_RANDOM_ENABLE, .type = CFG_TYPE_BOOL, .key = "rs-en", .defaultValue = 1, .min = 0, .max = 1, .unit = "", .name = "RandSnd en" },
   { .value = CFG_SOUND_RANDOM_SELECTION, .type = CFG_TYPE_INT, .key = "rs-select", .defaultValue = 0, .min = 0, .max = 2, .unit = "", .name = "RandSnd select" },
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
      int newVal = preferences.getInt(configDef[i].key, configDef[i].defaultValue);
      // check if value is valid and reset if not
      if (newVal > configDef[i].max || newVal < configDef[i].min) {
         newVal = configDef[i].defaultValue;
         preferences.putInt(configDef[i].key, configDef[i].defaultValue);
      }
      valuesChanged[i] = newVal != values[i];
      values[i] = newVal;
   }
   preferences.end();
}

void Config::reset()
{
   for (int i = 0; i < CFG_COUNT; i++)
   {
      valuesChanged[i] = values[i] != configDef[i].defaultValue;
      values[i] = configDef[i].defaultValue;
   }
   save();
}



