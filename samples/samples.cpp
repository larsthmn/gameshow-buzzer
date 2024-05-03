//#define WAV_TEST

#ifdef WAV_TEST

#include <Arduino.h>

#include "AudioGeneratorAAC.h"

#include "AudioOutputI2S.h"
#include "AudioOutputI2SNoDAC.h"

#include "AudioFileSourceSD.h"

#include "viola.h"
#include "AudioGeneratorWAV.h"

#define Bit_Clock_BCLK 17
#define Word_Select_WS 16
#define Serial_Data_SD 15
#define GAIN 0.5

AudioFileSourceSD* in;
AudioGeneratorAAC* aac;
AudioOutputI2S* out;

AudioGeneratorWAV *wav;

#if defined(ESP32)
#define chipSelect SS // ESP32 SD card select pin
#else
#define chipSelect 4  // ESP8266 SD card select pin (used as an example)
#endif

void setup()
{

   Serial.begin(115200);

   if (!SD.begin(chipSelect)) {
      Serial.println("Failed to initialize SD card");
      return;
   }
   else {
      Serial.println("SD card initialized successfully");
   }

   in = new AudioFileSourceSD("/StarWars60.wav");

   out = new AudioOutputI2S();

   out->SetGain(GAIN);
   out->SetPinout(Bit_Clock_BCLK, Word_Select_WS, Serial_Data_SD);

   audioLogger = &Serial;
   wav = new AudioGeneratorWAV();
   wav->begin(in, out);

//   aac->begin(in, out);
}

void loop()
{
   if (wav->isRunning()) {
      if (!wav->loop()) wav->stop();
   } else {
      Serial.printf("WAV done\n");
      delay(1000);
   }
}

#endif

#ifdef LCD_CODE

// vi:ts=4
// ----------------------------------------------------------------------------
// HelloWorld - simple demonstration of lcd
// Created by Bill Perry 2016-07-02
// bperrybap@opensource.billsworld.billandterrie.com
//
// This example code is unlicensed and is released into the public domain
// ----------------------------------------------------------------------------
//
// This sketch is for LCDs with PCF8574 or MCP23008 chip based backpacks
// WARNING:
//	Use caution when using 3v only processors like arm and ESP8266 processors
//	when interfacing with 5v modules as not doing proper level shifting or
//	incorrectly hooking things up can damage the processor.
//
// Sketch prints "Hello, World!" on the lcd
//
// If initialization of the LCD fails and the arduino supports a built in LED,
// the sketch will simply blink the built in LED.
//
// NOTE:
//	If the sketch fails to produce the expected results, or blinks the LED,
//	run the included I2CexpDiag sketch to test the i2c signals and the LCD.
//
// ----------------------------------------------------------------------------
// LiquidCrystal compability:
// Since hd44780 is LiquidCrystal API compatible, most existing LiquidCrystal
// sketches should work with hd44780 hd44780_I2Cexp i/o class once the
// includes are changed to use hd44780 and the lcd object constructor is
// changed to use the hd44780_I2Cexp i/o class.

#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

hd44780_I2Cexp lcd(0x27); // declare lcd object: auto locate & auto config expander chip

// If you wish to use an i/o expander at a specific address, you can specify the
// i2c address and let the library auto configure it. If you don't specify
// the address, or use an address of zero, the library will search for the
// i2c address of the device.
// hd44780_I2Cexp lcd(i2c_address); // specify a specific i2c address
//
// It is also possible to create multiple/seperate lcd objects
// and the library can still automatically locate them.
// Example:
// hd4480_I2Cexp lcd1;
// hd4480_I2Cexp lcd2;
// The individual lcds would be referenced as lcd1 and lcd2
// i.e. lcd1.home() or lcd2.clear()
//
// It is also possible to specify the i2c address
// when declaring the lcd object.
// Example:
// hd44780_I2Cexp lcd1(0x20);
// hd44780_I2Cexp lcd2(0x27);
// This ensures that each each lcd object is assigned to a specific
// lcd device rather than letting the library automatically asign it.

// LCD geometry
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

void setup()
{
   int status;
   Serial.begin(115200);

   while(!Serial);

   // initialize LCD with number of columns and rows:
   // hd44780 returns a status from begin() that can be used
   // to determine if initalization failed.
   // the actual status codes are defined in <hd44780.h>
   // See the values RV_XXXX
   //
   // looking at the return status from begin() is optional
   // it is being done here to provide feedback should there be an issue
   //
   // note:
   //	begin() will automatically turn on the backlight
   //
   lcd.setExecTimes(4000, 60);
   status = lcd.begin(LCD_COLS, LCD_ROWS);
   if(status) // non zero status means it was unsuccesful
   {
      Serial.println(status);
      // hd44780 has a fatalError() routine that blinks an led if possible
      // begin() failed so blink error code using the onboard LED if possible
      hd44780::fatalError(status); // does not return
   }

   // initalization was successful, the backlight should be on now
   Serial.println("ok");

   // Print a message to the LCD
   lcd.print("Hello, World!");
}

void loop() {}

#endif


#ifdef LCD_CODE_LIQUIDCRYSTAL

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

// create the lcd object using the default LCD I2C address 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
   int sdaPin = 16;
   int sclPin = 17;
   Wire.begin(sdaPin, sclPin); // Initialize Wire library with SDA = IO16 and SCL = IO17

   lcd.init();
   lcd.clear();
   lcd.backlight();      // Make sure backlight is on

   // Print a message on both lines of the LCD.
   lcd.setCursor(2,0);   //Set cursor to character 2 on line 0
   lcd.print("Myje stinkt!");

   lcd.setCursor(2,1);   //Move cursor to character 2 on line 1
   lcd.print("LOL");
}

void loop() {

}


#endif

