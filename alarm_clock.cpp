#include "alarm_clock.h"
#include "rgb_display.h"
#include "rtc.h"
#include "wifi_stuff.h"
#include "nvs_preferences.h"
#include <PushButtonTaps.h>
#include "touchscreen.h"

// program setup function
void AlarmClock::Setup() {

  // setup alarm clock program

  // initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // retrieve alarm settings
  nvs_preferences->RetrieveAlarmSettings(alarm_hr_, alarm_min_, alarm_is_AM_, alarm_ON_);

  // retrieve long press seconds
  nvs_preferences->RetrieveLongPressSeconds(alarm_long_press_seconds_);

  // setup buzzer timer
  SetupBuzzerTimer();

  PrintLn("Alarm Clock Initialized!");
}

void AlarmClock::SaveAlarm() {
  alarm_hr_ = var_1_;
  alarm_min_ = var_2_;
  alarm_is_AM_ = var_3_is_AM_;
  alarm_ON_ = var_4_ON_;

  // save alarm settings
  nvs_preferences->SaveAlarm(alarm_hr_, alarm_min_, alarm_is_AM_, alarm_ON_);

  PrintLn("Alarm Settings Saved!");
}

int16_t AlarmClock::MinutesToAlarm() {

  if(!alarm_ON_) return -1;

  uint16_t minutesToday, alarmAtMinute;

  // calculate alarmAtMinute
  if(alarm_hr_ == 12)
    alarmAtMinute = alarm_min_;
  else
    alarmAtMinute = alarm_hr_ * 60 + alarm_min_;
  if(!alarm_is_AM_) 
    alarmAtMinute += 12 * 60;

  // calculate minutesToday
  if(rtc->hourModeAndAmPm() == 0) {
    // 24 hour clock mode
    minutesToday = rtc->hour() * 60 + rtc->minute();
  }
  else { // 12 hour mode
    if(rtc->hour() == 12)
      minutesToday = rtc->minute();
    else
      minutesToday = rtc->hour() * 60 + rtc->minute();
    if(rtc->hourModeAndAmPm() == 2) 
      minutesToday += 12 * 60;
  }

  return alarmAtMinute - minutesToday;
}

// Function that starts buzzer and Alarm Screen
// It wait for user to press button to pause buzzer
// User needs to continue to press and hold button for
// ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS to end alarm.
// If user stops pressing button before alarm end, it will
// restart buzzer and the alarm end counter.
// If user does not end alarm by ALARM_MAX_ON_TIME_MS milliseconds,
// it will end alarm on its own.
void AlarmClock::BuzzAlarmFn() {
  // start alarm triggered page
  SetPage(kAlarmTriggeredPage);
  //start buzzer!
  BuzzerEnable();
  bool alarmStopped = false, buzzerPausedByUser = false;
  unsigned long alarmStartTimeMs = millis();
  int buttonPressSecondsCounter = alarm_long_press_seconds_;
  while(!alarmStopped) {
    ResetWatchdog();
    // if user presses button then pauze buzzer and start alarm end countdown!
    if(push_button->buttonActiveDebounced()) {
      if(!buzzerPausedByUser) {
        BuzzerDisable();
        buzzerPausedByUser = true;
      }
      unsigned long buttonPressStartTimeMs = millis(); //note time of button press
      // while button is pressed, display seconds countdown
      while(push_button->buttonActiveDebounced() && !alarmStopped) {
        ResetWatchdog();
        // display countdown to alarm off
        if(alarm_long_press_seconds_ - (millis() - buttonPressStartTimeMs) / 1000 < buttonPressSecondsCounter) {
          buttonPressSecondsCounter--;
          display->AlarmTriggeredScreen(false, buttonPressSecondsCounter);
        }
        // end alarm after holding button for ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS
        if(millis() - buttonPressStartTimeMs > alarm_long_press_seconds_ * 1000) {
          alarmStopped = true;
          // good morning screen! :)
          display->GoodMorningScreen();
        }
      }
    }
    // activate buzzer if button is not pressed by user
    if(!push_button->buttonActiveDebounced() && !alarmStopped) {
      if(buzzerPausedByUser) {
        BuzzerEnable();
        buzzerPausedByUser = false;
      }
      // if user lifts button press before alarm end then reset counter and re-display alarm-On screen
      if(buttonPressSecondsCounter != alarm_long_press_seconds_) {
        // display Alarm On screen with seconds user needs to press and hold button to end alarm
        buttonPressSecondsCounter = alarm_long_press_seconds_;
        display->AlarmTriggeredScreen(false, alarm_long_press_seconds_);
      }
    }
    // if user did not stop alarm within ALARM_MAX_ON_TIME_MS, make sure to stop buzzer
    if(millis() - alarmStartTimeMs > kAlarmMaxON_TimeMs) {
      BuzzerDisable();
      alarmStopped = true;
    }
  }
}

// Passive Buzzer Timer Interrupt Service Routine
#if defined(MCU_IS_ESP32)
void IRAM_ATTR AlarmClock::PassiveBuzzerTimerISR() {
#elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
bool AlarmClock::PassiveBuzzerTimerISR(struct repeating_timer *t) {
#endif
  // PassiveBuzzerTimerISR() function
  if(millis() - beep_start_time_ms_ > kBeepLengthMs) {
    beep_toggle_ = !beep_toggle_;
    beep_start_time_ms_ = millis();
    digitalWrite(LED_PIN, beep_toggle_);
  }
  buzzer_square_wave_toggle_ = !buzzer_square_wave_toggle_;
  digitalWrite(BUZZER_PIN, buzzer_square_wave_toggle_ && beep_toggle_);

  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    return true;
  #endif
}

void AlarmClock::BuzzerEnable() {
  // Timer Enable
  #if defined(MCU_IS_ESP32)
    timerAlarmEnable(passive_buzzer_timer_ptr_);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    int64_t delay_us = 1000000 / (kBuzzerFrequency * 2);
    add_repeating_timer_us(delay_us, PassiveBuzzerTimerISR, NULL, passive_buzzer_timer_ptr_);
  #endif

  PrintLn("BuzzerEnable!");
}

void AlarmClock::BuzzerDisable() {
  // Timer Disable
  #if defined(MCU_IS_ESP32)
    timerAlarmDisable(passive_buzzer_timer_ptr_);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    cancel_repeating_timer(passive_buzzer_timer_ptr_);
  #endif
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  buzzer_square_wave_toggle_ = false;
  beep_toggle_ = false;

  PrintLn("BuzzerDisable!");
}

void AlarmClock::SetupBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passive_buzzer_timer_ptr_ = timerBegin(1, 80, true);  // using timer 0, prescaler 80 (1MHz as ESP32 is 80MHz), counting up (true)
    timerAttachInterrupt(passive_buzzer_timer_ptr_, &PassiveBuzzerTimerISR, true);    //attach ISR to timer
    timerAlarmWrite(passive_buzzer_timer_ptr_, 1000000 / (kBuzzerFrequency * 2), true);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    passive_buzzer_timer_ptr_ = new struct repeating_timer;
  #endif

  PrintLn("Buzzer Timer setup successful!");
}

void AlarmClock::DeallocateBuzzerTimer() {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    delete passive_buzzer_timer_ptr_;
    passive_buzzer_timer_ptr_ = NULL;

    PrintLn("Buzzer Timer deallocated.");
  #endif
}

// inspired from rp2040 Tone example; Non-blocking
// Play a note of the specified frequency and for the specified duration.
// Hold is an optional bool that specifies if this note should be held a
// little longer, i.e. for eighth notes that are tied together.
// While waiting for a note to play the waitBreath delay function is used
// so breath detection and pixel animation continues to run.  No tones
// will play if the slide switch is in the -/off position or all the
// candles have been blown out.
void AlarmClock::playNote(int frequency, int duration, bool hold) {
  if (hold) {
    // For a note that's held play it a little longer than the specified duration
    // so it blends into the next tone (but there's still a small delay to
    // hear the next note).
    tone(BUZZER_PIN, frequency, duration + duration / 32);
  } else {
    // For a note that isn't held just play it for the specified duration.
    tone(BUZZER_PIN, frequency, duration);
  }
}

// inspired from rp2040 Tone example; Non-blocking
// Song to play when the candles are blown out.
void AlarmClock::celebrateSong(int &tone_note_index, unsigned long &next_tone_change_time) {
  // Play a little charge melody, from:
  //  https://en.wikipedia.org/wiki/Charge_(fanfare)
  // Note the explicit boolean parameters in particular the measure=false
  // at the end.  This means the notes will play without any breath measurement
  // logic.  Without this false value playNote will try to keep waiting for candles
  // to blow out during the celebration song!

  int duration = 0;
  switch(tone_note_index) {
    case 0: { duration = 183; playNote(392, duration, true); } break;
    case 1: { duration = 183; playNote(523, duration, true); } break;
    case 2: { duration = 183; playNote(659, duration, false); } break;
    case 3: { duration = 275; playNote(784, duration, true); } break;
    case 4: { duration = 137; playNote(659, duration, false); } break;
    case 5: { duration = 1100; playNote(784, duration, false); } break;
    default: { duration = 2000; }
  }
  tone_note_index++;
  next_tone_change_time += duration;
}