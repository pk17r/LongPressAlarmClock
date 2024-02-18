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
extern int availableRam();
extern void serialInputFlush();
extern void serialTimeStampPrefix();

#endif // COMMON_H