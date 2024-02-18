#ifndef COMMON_H
#define COMMON_H

// common includes
#include <Arduino.h>
#include "pin_defs.h"
#include "general_constants.h"

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

// current page on display
extern ScreenPage current_page;

// display time data in char arrays
struct DisplayData {
    char time_HHMM[kHHMM_ArraySize];
    char time_SS[kSS_ArraySize];
    char date_str[kDateArraySize];
    char alarm_str[kAlarmArraySize];
    bool _12_hour_mode;
    bool pm_not_am;
    bool alarm_ON;
  };

// Display Visible Data Struct
extern DisplayData new_display_data_, displayed_data_;



#endif // COMMON_H