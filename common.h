#ifndef COMMON_H
#define COMMON_H

// common includes
#include <Arduino.h>
#include "pin_defs.h"
#include "general_constants.h"
#include <string>
#include <queue>          // std::queue
#include "SPI.h"
#include <elapsedMillis.h>

// forward decleration of classes
class RTC;
class RGBDisplay;
class AlarmClock;
class WiFiStuff;
class EEPROM;
class PushButtonTaps;
class Touchscreen;

// spi
extern SPIClass* spi_obj;

// extern all global variables
extern RTC* rtc;
extern RGBDisplay* display;
extern AlarmClock* alarm_clock;
extern WiFiStuff* wifi_stuff;
extern EEPROM* eeprom;
extern PushButtonTaps* push_button;
extern PushButtonTaps* inc_button;
extern PushButtonTaps* dec_button;
extern Touchscreen* ts;

// counter to note user inactivity seconds
extern elapsedMillis inactivity_millis;

// flag for display pages
enum ScreenPage {
  kMainPage = 0,
  kScreensaverPage,
  kAlarmSetPage,
  kAlarmTriggeredPage,
  kTimeSetPage,
  kSettingsPage,
  kWiFiSettingsPage,
  kSoftApInputsPage,
  kEnterWiFiSsidPage,
  kEnterWiFiPasswdPage,
  kWeatherSettingsPage,
  kLocationInputsPage,
  kEnterWeatherLocationZipPage,
  kEnterWeatherLocationCountryCodePage,
  kWebOtaUpdatePage,
  kNoPageSelected
  };

// current page on display
extern ScreenPage current_page;

// flag for cursor highlight location
enum Cursor {
  kCursorNoSelection = 0,
  kMainPageSettingsWheel,
  kMainPageSetAlarm,
  kAlarmSetPageHour,
  kAlarmSetPageMinute,
  kAlarmSetPageAmPm,
  kAlarmSetPageOn,
  kAlarmSetPageOff,
  kAlarmSetPageSet,
  kAlarmSetPageCancel,
  kSettingsPageWiFi,
  kSettingsPageWeather,
  kSettingsPageSet,
  kSettingsPageScreensaver,
  kSettingsPageCancel,
  kWiFiSettingsPageSetSsidPasswd,
  kWiFiSettingsPageConnect,
  kWiFiSettingsPageDisconnect,
  kWiFiSettingsPageCancel,
  kSoftApInputsPageSave,
  kSoftApInputsPageCancel,
  kWeatherSettingsPageSetLocation,
  kWeatherSettingsPageUnits,
  kWeatherSettingsPageFetch,
  kWeatherSettingsPageUpdateTime,
  kWeatherSettingsPageCancel,
  kLocationInputsPageSave,
  kLocationInputsPageCancel,
  kCursorMaxValue,    // inc/dec button scroll won't go above this level
  kSettingsPageAlarmLongPressSeconds,
  };

// current cursor highlight location on page
extern Cursor highlight;

// postfix form ++ -- operator overloads for Cursor enum variables
inline Cursor operator++ (Cursor& highlight_location, int) {
  if(static_cast<int>(highlight_location) < static_cast<int>(kCursorMaxValue) - 1)
    highlight_location = static_cast<Cursor>(static_cast<int>(highlight_location) + 1);
  else
    highlight_location = kCursorNoSelection;
  return highlight_location;
}
inline Cursor operator-- (Cursor& highlight_location, int) {
  if(static_cast<int>(highlight_location) > static_cast<int>(kCursorNoSelection))
    highlight_location = static_cast<Cursor>(static_cast<int>(highlight_location) - 1);
  else
    highlight_location = static_cast<Cursor>(static_cast<int>(kCursorMaxValue) - 1);
  return highlight_location;
}


// flag for second core task
enum SecondCoreTask {
  kStartSetWiFiSoftAP = 0,
  kStopSetWiFiSoftAP,
  kStartLocationInputsLocalServer,
  kStopLocationInputsLocalServer,
  kGetWeatherInfo,
  kUpdateTimeFromNtpServer,
  kConnectWiFi,
  kDisconnectWiFi,
  kNoTask,
  };

// second core current task
// extern volatile SecondCoreTask second_core_task;
extern std::queue<SecondCoreTask> second_core_tasks_queue;
extern bool second_core_task_added_flag_array[];

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
extern void AddSecondCoreTaskIfNotThere(SecondCoreTask task);
extern void WaitForExecutionOfSecondCoreTask();
extern int AvailableRam();
extern void SerialInputFlush();
extern void SerialTimeStampPrefix();
extern void PrepareTimeDayDateArrays();
extern void SerialPrintRtcDateTime();
extern void ProcessSerialInput();
extern void SetPage(ScreenPage page);
extern void ResetWatchdog();
extern void PrintLn(const char* someText1, const char* someText2);
extern void PrintLn(const char* someText1);
extern void PrintLn(const char* someText1, int someInt);
extern void PrintLn(std::string someTextStr1, std::string someTextStr2);
extern void PrintLn(std::string &someTextStr1, std::string &someTextStr2);
extern void PrintLn(std::string &someTextStr1);
extern void PrintLn();

#endif // COMMON_H