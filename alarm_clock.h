#ifndef ALARM_CLOCK_H
#define ALARM_CLOCK_H

#include "common.h"
#if defined(MCU_IS_RASPBERRY_PI_PICO_W)   // include files for timer
  #include <stdio.h>
  #include "pico/stdlib.h"
  #include "hardware/timer.h"
  #include "hardware/irq.h"
#endif

class AlarmClock {

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
  void setup();
  void updateTimePriorityLoop();
  void nonPriorityTasksLoop();
  void retrieveAlarmSettings();
  void saveAlarm();
  bool timeToStartAlarm();
  void buzzAlarmFn();
  void processSerialInput();
  void setPage(ScreenPage page);
  // #if defined(MCU_IS_ESP32)
  // void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason);
  // void putEsp32ToLightSleep();
  // #endif

// OBJECTS and VARIABLES

  // secondCoreControlFlag controls idling and restarting core1 from core0
  //    0 = core is idling
  //    1 = resume the other core from core0
  //    2 = core is running some operation
  //    3 = core is done processing and can be idled
  volatile byte secondCoreControlFlag = 0;

  // flag to refresh RTC time from RTC HW
  bool refreshRtcTime = false;

  // counter to note user inactivity seconds
  uint8_t inactivitySeconds = 0;
  const uint8_t INACTIVITY_SECONDS_LIM = 120;

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


// PRIVATE FUNCTIONS AND VARIABLES / CONSTANTS
private:

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


#endif     // ALARM_CLOCK_H