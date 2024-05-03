//
// Created by Lars on 13.04.2024.
//

#ifndef ESP32_BUZZER_PINS_H
#define ESP32_BUZZER_PINS_H

#include <Arduino.h>

// The IO13 is ADC14

#define PUSH_BUTTONS_ANALOG_PIN GPIO_NUM_34

// LCD connector:
//  ┌─────────────┬───────────────┐
//  │(1) GND black│(2) 5V red     │
//┌─┤(3) NC       │(4) DB4 brown  │
//│ │(5) RS orange│(6) DB5 yellow │
//└─┤(7) E blue   │(8) DB6 green  │
//  │(9) white    │(10)DB7 purple │
//  └─────────────┴───────────────┘

// LCD display from Anet A8 (ok)
#define LCD_RS GPIO_NUM_27
#define LCD_E  GPIO_NUM_33
#define LCD_D4 GPIO_NUM_14
#define LCD_D5 GPIO_NUM_26
#define LCD_D6 GPIO_NUM_25
#define LCD_D7 GPIO_NUM_32
#define LCD_BUTTONS_ANALOG_PIN GPIO_NUM_35

// Second display is on default I2C pins (21: SDA, 22: SCL)

// LED next to the push buttons
#define RED_LED_PIN GPIO_NUM_4

// Buzzers
#define RED_BUZZER_INPUT GPIO_NUM_36
#define BLUE_BUZZER_INPUT GPIO_NUM_39
#define RED_BUZZER_LED GPIO_NUM_12
#define BLUE_BUZZER_LED GPIO_NUM_13

// I2S amplifier
#define I2S_LRC GPIO_NUM_16
#define I2S_BCLK GPIO_NUM_15
#define I2S_DIN GPIO_NUM_17

#endif //ESP32_BUZZER_PINS_H
