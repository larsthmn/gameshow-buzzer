
#include <LiquidCrystal.h>
//#include <LcdMenu.h>

// The IO13 is ADC14
#define ANALOG_PIN 13
#define MAX_READING 4095U  // For 12 bit precision
#define REF_VOLTAGE 3300U  // mV

LiquidCrystal lcd(GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_14);

// (1) GND      | (2) 5V
// (3) NC       | (4) DB4 blue
// (5) RS white | (6) DB5 green
// (7) E purple | (8) DB6 yellow
// (9) btn / NC | (10) DB7 orange


void setup() {
   // Set up the LCD's number of columns and rows
   lcd.begin(20, 4);
   // Configure ADC
   analogSetAttenuation(ADC_11db);
}

void loop() {
   // Read from ADC
   uint16_t reading = analogRead(ANALOG_PIN);

   // Convert ADC reading to voltage
   uint16_t voltage = (uint32_t)reading * REF_VOLTAGE / MAX_READING;

   // Set the cursor to the beginning, line 1
   lcd.setCursor(0, 0);
   lcd.print("Voltage on ADC14:");
   lcd.setCursor(0, 1);
   lcd.print(voltage);  // Print voltage with 2 decimal places
   lcd.print("mV");
   // Turn off the blinking cursor:
   delay(500);
}
