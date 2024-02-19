#ifndef COMMON_H
#define COMMON_H

// common includes
#include <Arduino.h>
#include "pin_defs.h"
#include "general_constants.h"

// forward decleration of classes
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

// counter to note user inactivity seconds
extern uint8_t inactivity_seconds;

// secondCoreControlFlag controls idling and restarting core1 from core0
//    0 = core is idling
//    1 = resume the other core from core0
//    2 = core is running some operation
//    3 = core is done processing and can be idled
extern volatile byte second_core_control_flag;

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

// flag for second core task
enum SecondCoreTask {
  kNoTask = 0,
  kGetWeatherInfo ,
  kUpdateTimeFromNtpServer,
  kTaskCompleted
  };

// second core current task
extern volatile SecondCoreTask second_core_task;

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


// extern all global functions
extern int AvailableRam();
extern void SerialInputFlush();
extern void SerialTimeStampPrefix();
extern void PrepareTimeDayDateArrays();
extern void SerialPrintRtcDateTime();
extern void ProcessSerialInput();
extern void SetPage(ScreenPage page);

#endif // COMMON_H