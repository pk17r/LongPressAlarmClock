#include "alarm_clock.h"
#include "rgb_display.h"
#include "rtc.h"
// #include "hardware/sync.h"
#include "wifi_stuff.h"
#include "eeprom.h"
#include <PushButtonTaps.h>
#if defined(TOUCHSCREEN_IS_XPT2046)
  #include "touchscreen.h"
#endif

// program setup function
void AlarmClock::Setup() {
  
  // setup alarm clock program

  // initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // retrieve alarm settings
  RetrieveAlarmSettings();
}

// arduino loop function - High Priority one
void AlarmClock::UpdateTimePriorityLoop() {

    // button pressed or touchscreen touched
  if(push_button->checkButtonStatus() != 0 || ts->IsTouched()) {
    // show instant response by turing up brightness
    display->SetMaxBrightness();

    if(current_page == kScreensaverPage)
    { // turn off screensaver if on
      SetPage(kMainPage);
    }
    else if(current_page == kAlarmSetPage)
    { // if on alarm page, then take alarm set page user inputs
      display->SetAlarmScreen(true);
    }
    else if(current_page != kAlarmSetPage && inactivity_seconds >= 1)
    { // if on main page and user clicked somewhere, get touch input
      ScreenPage userTouchRegion = display->ClassifyMainPageTouchInput();
      if(userTouchRegion != kNoPageSelected)
        SetPage(userTouchRegion);
    }
    inactivity_seconds = 0;
  }

  // if user presses button, show response by turning On LED
  if(push_button->buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // process time actions every second
  if (rtc->rtc_hw_sec_update_) {
    rtc->rtc_hw_sec_update_ = false;

    // prepare updated seconds array
    display->UpdateSecondsOnTimeStrArr(rtc->second());

    SerialTimeStampPrefix();

    // get time update on new minute
    if (rtc->rtc_hw_min_update_) {
      rtc->rtc_hw_min_update_ = false;

      rtc->Refresh();

      SerialTimeStampPrefix();

      // if user has been inactive, then set brightness based on time of day
      // set display brightness based on time
      if(inactivity_seconds >= kInactivitySecondsLimit)
        display->CheckTimeAndSetBrightness();

      // if screensaver is On, then update time on it
      if(current_page == kScreensaverPage)
        display->refresh_screensaver_canvas_ = true;

      // 5 mins before alarm time, try to get weather info
      #if defined(MCU_IS_RASPBERRY_PI_PICO_W) || defined(MCU_IS_ESP32)

        if((wifi_stuff->got_weather_info_time_ms == 0 || millis() - wifi_stuff->got_weather_info_time_ms > 60*60*1000) && second_core_control_flag == 0) {
          if(wifi_stuff->got_weather_info_time_ms == 0)
            second_core_control_flag = 1;
          else if(millis() - wifi_stuff->got_weather_info_time_ms > 60*60*1000)
            second_core_control_flag = 1;
          else if(MinutesToAlarm() == 5) // get updated weather info 5 minutes before alarm time
            second_core_control_flag = 1;

          if(second_core_control_flag == 1)
            Serial.println("Time to update weather info!");
        }

      #endif

      // Activate Buzzer at Alarm Time
      if(MinutesToAlarm() == 0) {
        // go to buzz alarm function
        BuzzAlarmFn();
        // returned from Alarm Triggered Screen and Good Morning Screen
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        PrepareTimeDayDateArrays();
        // set main page back
        SetPage(kMainPage);
        inactivity_seconds = 0;
      }
    }

    // prepare date and time arrays
    PrepareTimeDayDateArrays();

    // check for inactivity
    if(inactivity_seconds <= kInactivitySecondsLimit) {
      inactivity_seconds++;
      if(inactivity_seconds >= kInactivitySecondsLimit) {
        // set display brightness based on time
        display->CheckTimeAndSetBrightness();
        // turn screen saver On
        if(current_page != kScreensaverPage)
          SetPage(kScreensaverPage);
      }
    }

    // update time on main page
    if(current_page == kMainPage)
      display->DisplayTimeUpdate();

    // serial print RTC Date Time
    SerialPrintRtcDateTime();
    Serial.println();

    // second core control operations
    switch(second_core_control_flag) {
      case 1: // resume the other core
        // rp2040.restartCore1();
        // rp2040.resumeOtherCore();
        second_core_control_flag = 2;  // core started
        // Serial.println("Resumed core1");
        break;
      case 3: // core1 is done processing and can be idled
        // rp2040.idleOtherCore();
        second_core_control_flag = 0;  // core idled
        // Serial.println("Idled core1");
        break;
    }
  }

  // make screensaver motion fast
  if(current_page == kScreensaverPage)
    display->Screensaver();

  // accept user inputs
  if (Serial.available() != 0)
    ProcessSerialInput();

// #if defined(MCU_IS_ESP32)
//     // if button is inactive, then go to sleep
//     if(!pushBtn->buttonActiveDebounced())
//       putEsp32ToLightSleep();
// #endif

}

// arduino loop function - less priority one
void AlarmClock::NonPriorityTasksLoop() {

  // run the core only to do specific not time important operations
  if (second_core_control_flag == 2) {

    // get today's weather info
    wifi_stuff->GetTodaysWeatherInfo();

    // try once more if did not get info
    if(!wifi_stuff->got_weather_info_)
      wifi_stuff->GetTodaysWeatherInfo();

    // done processing the task
    // set the core up to be idled from core0
    second_core_control_flag = 3;
  }

  // a delay to slow things down and not crash
  delay(1000);
}

void AlarmClock::RetrieveAlarmSettings() {
  
  if(!(eeprom->RetrieveAlarmSettings(alarm_hr_, alarm_min_, alarm_is_AM_, alarm_ON_)))
    SaveAlarm();
}

void AlarmClock::SaveAlarm() {
  alarm_hr_ = var_1_;
  alarm_min_ = var_2_;
  alarm_is_AM_ = var_3_AM_PM_;
  alarm_ON_ = var_4_ON_OFF_;

  // save alarm settings
  eeprom->SaveAlarm(alarm_hr_, alarm_min_, alarm_is_AM_, alarm_ON_);
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
  // setup buzzer timer
  SetupBuzzerTimer();
  //start buzzer!
  BuzzerEnable();
  bool alarmStopped = false, buzzerPausedByUser = false;
  unsigned long alarmStartTimeMs = millis();
  int buttonPressSecondsCounter = kAlarmEndButtonPressAndHoldSeconds;
  while(!alarmStopped) {
    // if user presses button then pauze buzzer and start alarm end countdown!
    if(push_button->buttonActiveDebounced()) {
      if(!buzzerPausedByUser) {
        BuzzerDisable();
        buzzerPausedByUser = true;
      }
      unsigned long buttonPressStartTimeMs = millis(); //note time of button press
      // while button is pressed, display seconds countdown
      while(push_button->buttonActiveDebounced() && !alarmStopped) {
        // display countdown to alarm off
        if(kAlarmEndButtonPressAndHoldSeconds - (millis() - buttonPressStartTimeMs) / 1000 < buttonPressSecondsCounter) {
          buttonPressSecondsCounter--;
          display->AlarmTriggeredScreen(false, buttonPressSecondsCounter);
        }
        // end alarm after holding button for ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS
        if(millis() - buttonPressStartTimeMs > kAlarmEndButtonPressAndHoldSeconds * 1000) {
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
      if(buttonPressSecondsCounter != kAlarmEndButtonPressAndHoldSeconds) {
        // display Alarm On screen with seconds user needs to press and hold button to end alarm
        buttonPressSecondsCounter = kAlarmEndButtonPressAndHoldSeconds;
        display->AlarmTriggeredScreen(false, kAlarmEndButtonPressAndHoldSeconds);
      }
    }
    // if user did not stop alarm within ALARM_MAX_ON_TIME_MS, make sure to stop buzzer
    if(millis() - alarmStartTimeMs > kAlarmMaxON_TimeMs) {
      BuzzerDisable();
      alarmStopped = true;
    }
  }
  DeallocateBuzzerTimer();
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
}

void AlarmClock::SetupBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passive_buzzer_timer_ptr_ = timerBegin(1, 80, true);  // using timer 0, prescaler 80 (1MHz as ESP32 is 80MHz), counting up (true)
    timerAttachInterrupt(passive_buzzer_timer_ptr_, &PassiveBuzzerTimerISR, true);    //attach ISR to timer
    timerAlarmWrite(passive_buzzer_timer_ptr_, 1000000 / (BUZZER_FREQUENCY * 2), true);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    passive_buzzer_timer_ptr_ = new struct repeating_timer;
  #endif

  Serial.println(F("Timer setup successful!"));
}

void AlarmClock::DeallocateBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
  passive_buzzer_timer_ptr_ = NULL;
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    delete passive_buzzer_timer_ptr_;
    passive_buzzer_timer_ptr_ = NULL;
  #endif

  Serial.println(F("Buzzer Timer deallocated."));
}


// #if defined(MCU_IS_ESP32)
// /*
//   Esp32 light sleep function
//   https://lastminuteengineers.com/esp32-deep-sleep-wakeup-sources/
// */
// void AlarmClock::putEsp32ToLightSleep() {
//   /*
//   First we configure the wake up source
//   We set our ESP32 to wake up for an external trigger.
//   There are two types for ESP32, ext0 and ext1 .
//   ext0 uses RTC_IO to wakeup thus requires RTC peripherals
//   to be on while ext1 uses RTC Controller so doesnt need
//   peripherals to be powered on.
//   Note that using internal pullups/pulldowns also requires
//   RTC peripherals to be turned on.
//   */
//   // add a timer to wake up ESP32
//   esp_sleep_enable_timer_wakeup(500000); //0.5 seconds
//   // ext1 button press as wake up source
//   esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
//   //Go to sleep now
//   serialTimeStampPrefix();
//   Serial.println("Go To Light Sleep for 0.5 sec or button press");
//   Serial.flush();
//   // go to light sleep
//   esp_light_sleep_start();
//   // On WAKEUP disable timer as wake up source
//   esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

//   esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
//   //Print the wakeup reason for ESP32
//   serialTimeStampPrefix();
//   print_wakeup_reason(wakeup_reason);

//   // if wakeup reason was timer then add seconds ticker signal to wake up source and go back to sleep
//   if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
//     // add ext0 RTC seconds ticker as wake up source
//     esp_sleep_enable_ext0_wakeup((gpio_num_t)SQW_INT_PIN,1); //Wake up at: 1 = High, 0 = Low
//     //Go to sleep now
//     serialTimeStampPrefix();
//     Serial.println("Go To Light Sleep until seconds tick or button press");
//     //esp_deep_sleep_start();
//     Serial.flush();
//     // go to light sleep
//     esp_light_sleep_start();

//     // On WAKEUP disable EXT0 as wake up source
//     esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);

//     wakeup_reason = esp_sleep_get_wakeup_cause();
//     // if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
//     //   turnBacklightOn();

//     //Print the wakeup reason for ESP32
//     serialTimeStampPrefix();
//     print_wakeup_reason(wakeup_reason);
//   }
// }

// /*
// Method to print the reason by which ESP32
// has been awaken from sleep
// */
// void AlarmClock::print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason){
//   switch(wakeup_reason)
//   {
//     case ESP_SLEEP_WAKEUP_EXT0 : Serial.println(F("Wakeup by ext signal RTC_IO - SECONDS TICK")); break;
//     case ESP_SLEEP_WAKEUP_EXT1 : Serial.println(F("Wakeup by ext signal RTC_CNTL - BUTTON PRESS")); break;
//     case ESP_SLEEP_WAKEUP_TIMER : Serial.println(F("Wakeup caused by TIMER")); break;
//     case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println(F("Wakeup caused by touchpad")); break;
//     case ESP_SLEEP_WAKEUP_ULP : Serial.println(F("Wakeup caused by ULP program")); break;
//     default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
//   }
// }
// #endif
