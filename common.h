#ifndef COMMON_H
#define COMMON_H

// common includes
#include <Arduino.h>
#include "pin_defs.h"
#include "general_constants.h"
#include <string>
#include <queue>          // std::queue
#include "SPI.h"

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
extern Touchscreen* ts;

// counter to note user inactivity seconds
extern uint8_t inactivity_seconds;

// flag for display pages
enum ScreenPage {
  kMainPage = 0,
  kScreensaverPage,
  kAlarmSetPage,
  kAlarmTriggeredPage,
  kTimeSetPage,
  kSettingsPage,
  kWiFiSettingsPage,
  kEnterWiFiSsidPage,
  kEnterWiFiPasswdPage,
  kWeatherSettingsPage,
  kEnterWeatherLocationZipPage,
  kEnterWeatherLocationCountryCodePage,
  kNoPageSelected
  };

// current page on display
extern ScreenPage current_page;

// flag for second core task
enum SecondCoreTask {
  kNoTask = 0,
  kGetWeatherInfo ,
  kUpdateTimeFromNtpServer,
  kConnectWiFi,
  kDisconnectWiFi
  };

// second core current task
// extern volatile SecondCoreTask second_core_task;
extern std::queue<SecondCoreTask> second_core_tasks_queue;

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