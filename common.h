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
extern AlarmClock* alarm_clock;
extern WiFiStuff* wifi_stuff;
extern EEPROM* eeprom;
extern PushButtonTaps* push_button;
extern Touchscreen* ts;

// extern all global functions
extern int AvailableRam();
extern void SerialInputFlush();
extern void SerialTimeStampPrefix();

// flag for display pages
enum ScreenPage {
  kMainPage = 0,
  kScreensaverPage,
  kAlarmSetPage,
  kAlarmTriggeredPage,
  kTimeSetPage,
  kSettingsPage,
  kNoPageSelected
    };

extern ScreenPage current_page;    // current page on display

#endif // COMMON_H