#ifndef COMMON_H
#define COMMON_H

// common includes
#include <Arduino.h>
#include "pin_defs.h"

// forward decleration of other classes
class RTC;
class RGBDisplay;
class AlarmClock;
class WiFiStuff;
class EEPROM;
class PushButtonTaps;
class Touchscreen;

// extern all global variables
extern RTC* rtc;
extern RGBDisplay* display;
extern AlarmClock* alarmClock;
extern WiFiStuff* wifiStuff;
extern EEPROM* eeprom;
extern PushButtonTaps pushBtn;
extern Touchscreen ts;

// extern all global functions
extern int freeRam();
extern void serial_input_flush();

#endif // COMMON_H