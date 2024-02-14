#ifndef ALARM_CLOCK_MAIN_H
#define ALARM_CLOCK_MAIN_H

#if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  // #include "secrets.h"
  #include "WiFi.h"
  #include <HTTPClient.h>
  #include <Arduino_JSON.h>
#endif
#if defined(MCU_IS_TEENSY) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include <EEPROM.h>
#endif
#if defined(MCU_IS_RASPBERRY_PI_PICO_W)   // include files for timer
  #include <stdio.h>
  #include "pico/stdlib.h"
  #include "hardware/timer.h"
  #include "hardware/irq.h"
#endif
#include "pin_defs.h"
#include <PushButtonTaps.h>
#include "rgb_display_class.h"
#include "uRTCLib.h"
#if defined(TOUCHSCREEN_IS_XPT2046)
  #include "touchscreen.h"
#endif

// forward decleration of other classes
class rgb_display_class;


class alarm_clock_main {

public:

  // current page on display
  enum ScreenPage {
    mainPage,
    screensaverPage,
    alarmSetPage,
    alarmTriggeredPage,
    timeSetPage,
    settingsPage,
     } currentPage;

// FUNCTIONS

  // function declerations
  void setup(rgb_display_class* disp_ptr);//, touchscreen* ts_ptr);
  void retrieveSettings();
  void loop();
  void rtc_clock_initialize();
  // clock seconds interrupt ISR
  static void sqwPinInterruptFn();
  void serialTimeStampPrefix();
  void updateSecond();
  bool timeToStartAlarm();
  void buzzAlarmFn();
  void serial_input_flush();
  void processSerialInput();
  void setPage(ScreenPage page);
  void saveAlarm();
  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  void saveWiFiDetails();
  void turn_WiFi_On();
  void turn_WiFi_Off();
  void getTodaysWeatherInfo();
  #endif
  // #if defined(MCU_IS_ESP32)
  // void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason);
  // void putEsp32ToLightSleep();
  // #endif

// OBJECTS and VARIABLES

  // Push Button
  PushButtonTaps pushBtn;

  #if defined(TOUCHSCREEN_IS_XPT2046)
    // touch screen
    touchscreen ts;
  #endif

  // RTC clock object for DC3231 rtc
  uRTCLib rtc;

  // display object
  rgb_display_class* display = NULL;

  // seconds blinker
  bool blink = false;

  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
    const unsigned int WIFI_SSID_PASSWORD_LENGTH_MAX = 32;
    #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
      char* wifi_ssid = MY_WIFI_SSID;
    #else
      char* wifi_ssid = NULL;
    #endif
    #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
      char* wifi_password = MY_WIFI_PASSWD;
    #else
      char* wifi_password = NULL;
    #endif
  #endif

  // seconds counter to track RTC HW seconds
  // we'll refresh RTC time everytime second reaches 60
  // All other parameters of RTC will not change at any other time
  // at 60 seconds, we'll update the time row
  uint8_t second = 0;

  // flag to refresh RTC time from RTC HW
  bool refreshRtcTime = false;

  // counter to note user inactivity seconds
  uint8_t inactivitySeconds = 0;
  const uint8_t INACTIVITY_SECONDS_LIM = 120;

  // seconds flag triggered by interrupt
  static inline volatile bool secondsIncremented;

  // location of Alarm 
  const int16_t alarmScreenAreaMainPageY = 160;

  // flag to set alarm On or Off
  bool alarmOn = true;

  // alarm time
  uint8_t alarmHr = 7;
  uint8_t alarmMin = 0;
  bool alarmIsAm = true;

  // Alarm constants
  const uint8_t ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS = 25;
  const unsigned long ALARM_MAX_ON_TIME_MS = 120*1000;

  // Set Screen variables
  uint8_t var1 = alarmHr;
  uint8_t var2 = alarmMin;
  bool var3AmPm = alarmIsAm;
  bool var4OnOff = alarmOn;

  // weather information
  char* weather_main = NULL;
  char* weather_description = NULL;
  char* weather_temp = NULL;
  char* weather_temp_max = NULL;
  char* weather_temp_min = NULL;
  char* weather_wind_speed = NULL;
  char* weather_humidity = NULL;

// PRIVATE FUNCTIONS AND VARIABLES / CONSTANTS

  // buzzer functions
  void setupBuzzerTimer();
  #if defined(MCU_IS_ESP32)
    void IRAM_ATTR passiveBuzzerTimerISR();
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    static bool passiveBuzzerTimerISR(struct repeating_timer *t);
  #endif
  void buzzer_enable();
  void buzzer_disable();
  void deallocateBuzzerTimer();

  /** the address in the EEPROM **/
  const unsigned int ALARM_ADDRESS_EEPROM = 0; // stores data in order 0 = data is set, 1 = hr, 2 = min, 3 = isAm, 4 = alarmOn
  const unsigned int WIFI_ADDRESS_EEPROM = 5; // stores data in order 5 = wifi_ssid ending with \0 thereafter wifi_password ending with \0

  // Hardware Timer
  #if defined(MCU_IS_ESP32)
    hw_timer_t *passiveBuzzerTimerPtr = NULL;
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    struct repeating_timer *passiveBuzzerTimerPtr = NULL;
  #endif

  const int BUZZER_FREQUENCY = 2048;
  static inline const unsigned long BEEP_LENGTH_MS = 800;

  static inline bool _buzzerSquareWaveToggle = false;
  static inline bool _beepToggle = false;
  static inline unsigned long _beepStartTimeMs = 0;

};


#endif     // ALARM_CLOCK_MAIN_H