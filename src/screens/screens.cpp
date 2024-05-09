//
// Created by Lars on 09.05.2024.
//


#include "screens.h"
#include "menuScreen.h"
#include "debugScreen.h"
#include "soundboardScreen.h"

void ScreenManager::init()
{
   display.begin(20, 4);
   menu_init();
   soundBoardScreen_init();
}

void ScreenManager::loop(const InputValues& values)
{
   // Make sure to keep it in synch with the enum
   screenFunction screenFunctions[SCREEN_COUNT] = {
      soundBoardScreen,
      menuScreen,
      debugScreen,
   };
   static Screen screen = SCREEN_SOUNDBOARD;
   static Screen prevScreen = SCREEN_COUNT;
   bool changed = prevScreen != screen;
   prevScreen = screen;
   screen = screenFunctions[screen](values, display, changed);
}
