/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/
#include "alarm_clock.h"
#include "rgb_display.h"

RGBDisplay* display = NULL;
AlarmClock* alarmClock = NULL;

// extern "C" char* sbrk(int incr);

// void display_freeram(){
//   Serial.print(F("- SRAM left: ")); Serial.println(freeRam());
// }

// int freeRam() {
//   char top;
//   return &top - reinterpret_cast<char*>(sbrk(0));
// }

void setup() {
  // idle the other core
  rp2040.idleOtherCore();
  // initialize modules
  alarmClock = new AlarmClock();
  display = new RGBDisplay();
  // setup program
  alarmClock->setup();
  // restart the other core
  rp2040.restartCore1();
}

void loop() {
  // put your main code here, to run repeatedly:
  alarmClock->updateTimePriorityLoop();
}

void setup1() {
  delay(500);
}

void loop1() {
  // put your main code here, to run repeatedly:
  alarmClock->nonPriorityTasksLoop();
  // display_freeram();
}
