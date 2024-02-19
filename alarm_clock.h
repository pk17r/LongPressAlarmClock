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

// FUNCTIONS

  // function declerations
  void Setup();
  void RetrieveAlarmSettings();
  void SaveAlarm();
  int16_t MinutesToAlarm();
  void BuzzAlarmFn();
  // #if defined(MCU_IS_ESP32)
  // void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason);
  // void putEsp32ToLightSleep();
  // #endif

// OBJECTS and VARIABLES

  // alarm time
  uint8_t alarm_hr_ = 7;
  uint8_t alarm_min_ = 0;
  bool alarm_is_AM_ = true;
  bool alarm_ON_ = true;    // flag to set alarm On or Off

  // Alarm constants
  const uint8_t kAlarmEndButtonPressAndHoldSeconds = 2;
  const unsigned long kAlarmMaxON_TimeMs = 120*1000;

  // Set Screen variables
  uint8_t var_1_ = alarm_hr_;
  uint8_t var_2_ = alarm_min_;
  bool var_3_AM_PM_ = alarm_is_AM_;
  bool var_4_ON_OFF_ = alarm_ON_;


// PRIVATE FUNCTIONS AND VARIABLES / CONSTANTS

private:

  // buzzer functions
  // buzzer used is a passive buzzer which is run using timers
  void SetupBuzzerTimer();
  #if defined(MCU_IS_ESP32)
    void IRAM_ATTR PassiveBuzzerTimerISR();
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    static bool PassiveBuzzerTimerISR(struct repeating_timer *t);
  #endif
  void BuzzerEnable();
  void BuzzerDisable();
  void DeallocateBuzzerTimer();

  // Hardware Timer
  #if defined(MCU_IS_ESP32)
    hw_timer_t *passive_buzzer_timer_ptr_ = NULL;
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    struct repeating_timer *passive_buzzer_timer_ptr_ = NULL;
  #endif

  const int kBuzzerFrequency = 2048;
  static inline const unsigned long kBeepLengthMs = 800;

  static inline bool buzzer_square_wave_toggle_ = false;
  static inline bool beep_toggle_ = false;
  static inline unsigned long beep_start_time_ms_ = 0;

};


#endif     // ALARM_CLOCK_H