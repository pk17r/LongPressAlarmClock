/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/
#include "alarm_clock_main.h"
#include "rgb_display_class.h"

alarm_clock_main programObj;
rgb_display_class displayObj;

// extern "C" char* sbrk(int incr);

// void display_freeram(){
//   Serial.print(F("- SRAM left: ")); Serial.println(freeRam());
// }

// int freeRam() {
//   char top;
//   return &top - reinterpret_cast<char*>(sbrk(0));
// }

void setup() {
  programObj.setup(&displayObj);
}

void loop() {
  // put your main code here, to run repeatedly:
  programObj.loop();
  // display_freeram();
}
