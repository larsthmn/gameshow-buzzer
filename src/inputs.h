/*
 * @brief User inputs
 */

#ifndef ESP32_BUZZER_INPUTS_H
#define ESP32_BUZZER_INPUTS_H

#include <cstdint>

#define PUSH_BUTTON_COUNT 6

enum ButtonType
{
   BUTTON_NONE,

   // LCD buttons
   BUTTON_UP,
   BUTTON_LEFT,
   BUTTON_DOWN,
   BUTTON_RIGHT,
   BUTTON_ENTER,

   // Push buttons
   BUTTON_YELLOW,
   BUTTON_RED,
   BUTTON_BLACK,
   BUTTON_GREEN,
   BUTTON_WHITE,
   BUTTON_BLUE,

   BUTTON_TYPES_COUNT,
};

struct InputValues
{
   uint16_t readingLcdButtons;
   uint16_t readingPushButtons;
   bool isRedBuzzerPressed;
   bool isBlueBuzzerPressed;
   ButtonType pushBtn;
   bool pushBtnChanged;
   ButtonType lcdBtn;
   bool lcdBtnChanged;
};

extern const char* ButtonTypeStr[BUTTON_TYPES_COUNT];

/**
 * @brief Reads input values from different sources and updates the InputValues struct.
 *
 * @param values The InputValues struct to store the input values and flags.
 */
void getInputValues(InputValues& values);

void inputsInit();

#endif //ESP32_BUZZER_INPUTS_H
