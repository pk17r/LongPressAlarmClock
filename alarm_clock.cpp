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
    else if(current_page != kAlarmSetPage && inactivity_seconds_ >= 1)
    { // if on main page and user clicked somewhere, get touch input
      ScreenPage userTouchRegion = display->ClassifyMainPageTouchInput();
      if(userTouchRegion != kNoPageSelected)
        SetPage(userTouchRegion);
    }
    inactivity_seconds_ = 0;
  }

  // if user presses button, show response by turning On LED
  if(push_button->buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // process time actions every second
  if (rtc->rtc_hw_sec_update_) {
    rtc->rtc_hw_sec_update_ = false;
    if (rtc->second() >= 60)
      refresh_rtc_time_ = true;

    // prepare updated seconds array
    display->UpdateSecondsOnTimeStrArr(rtc->second());

    SerialTimeStampPrefix();

    // get time update
    if (refresh_rtc_time_) {
      rtc->Refresh();
      refresh_rtc_time_ = false;

      SerialTimeStampPrefix();

      // if user has been inactive, then set brightness based on time of day
      // set display brightness based on time
      if(inactivity_seconds_ >= kInactivitySecondsLimit)
        display->CheckTimeAndSetBrightness();

      // if screensaver is On, then update time on it
      if(current_page == kScreensaverPage)
        display->refresh_screensaver_canvas_ = true;

      // 5 mins before alarm time, try to get weather info
      #if defined(MCU_IS_RASPBERRY_PI_PICO_W) || defined(MCU_IS_ESP32)

        if(!wifi_stuff->got_weather_info_ && second_core_control_flag_ == 0) {

          if(alarm_min_ >= 5)
            if(rtc->hour() == alarm_hr_ && rtc->minute() == alarm_min_ - 5)
              second_core_control_flag_ = 1;
          else if(alarm_hr_ > 1)
            if(rtc->hour() == alarm_hr_ - 1 && rtc->minute() == 55)
              second_core_control_flag_ = 1;
          else
            if(rtc->hour() == 12 && rtc->minute() == 55)
              second_core_control_flag_ = 1;
        }

      #endif

      // Activate Buzzer at Alarm Time
      if(TimeToStartAlarm() && rtc->second() < 10) {
        // go to buzz alarm function
        BuzzAlarmFn();
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        display->PrepareTimeDayDateArrays();
        // set main page back
        SetPage(kMainPage);
        inactivity_seconds_ = 0;
        wifi_stuff->got_weather_info_ = false;
      }
    }

    // prepare date and time arrays
    display->PrepareTimeDayDateArrays();

    // check for inactivity
    if(inactivity_seconds_ <= kInactivitySecondsLimit) {
      inactivity_seconds_++;
      if(inactivity_seconds_ >= kInactivitySecondsLimit) {
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
    display->SerialPrintRtcDateTime();
    Serial.println();

    // second core control operations
    switch(second_core_control_flag_) {
      case 1: // resume the other core
        // rp2040.restartCore1();
        // rp2040.resumeOtherCore();
        second_core_control_flag_ = 2;  // core started
        // Serial.println("Resumed core1");
        break;
      case 3: // core1 is done processing and can be idled
        // rp2040.idleOtherCore();
        second_core_control_flag_ = 0;  // core idled
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
  if (second_core_control_flag_ == 2) {

    // get today's weather info
    wifi_stuff->GetTodaysWeatherInfo();

    // try once more if did not get info
    if(!wifi_stuff->got_weather_info_)
      wifi_stuff->GetTodaysWeatherInfo();

    // done processing the task
    // set the core up to be idled from core0
    second_core_control_flag_ = 3;
  }

  // a delay to slow things down and not crash
  delay(1000);
}

void AlarmClock::SetPage(ScreenPage page) {
  switch(page) {
    case kMainPage:
      // if screensaver is active then clear screensaver canvas to free memory
      if(current_page == kScreensaverPage)
        display->ScreensaverControl(false);
      current_page = kMainPage;         // new page needs to be set before any action
      display->redraw_display_ = true;
      display->DisplayTimeUpdate();
      break;
    case kScreensaverPage:
      current_page = kScreensaverPage;      // new page needs to be set before any action
      display->ScreensaverControl(true);
      break;
    case kAlarmSetPage:
      current_page = kAlarmSetPage;     // new page needs to be set before any action
      // set variables for alarm set screen
      var_1_ = alarm_hr_;
      var_2_ = alarm_min_;
      var_3_AM_PM_ = alarm_is_AM_;
      var_4_ON_OFF_ = alarm_ON_;
      display->SetAlarmScreen(false);
      break;
    case kAlarmTriggeredPage:
      current_page = kAlarmTriggeredPage;     // new page needs to be set before any action
      display->AlarmTriggeredScreen(true, kAlarmEndButtonPressAndHoldSeconds);
      display->SetMaxBrightness();
      break;
    case kSettingsPage:
      current_page = kSettingsPage;     // new page needs to be set before any action
      display->SettingsPage();
      display->SetMaxBrightness();
      break;
    default:
      Serial.print("Unprogrammed Page "); Serial.print(page); Serial.println('!');
  }
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

void AlarmClock::ProcessSerialInput() {
  // take user input
  char input = Serial.read();
  SerialInputFlush();
  // acceptable user input
  Serial.print(F("User input: "));
  Serial.println(input);
  // process user input
  switch (input) {
    case 'a':   // toggle alarm On Off
      Serial.println(F("**** Toggle Alarm ****"));
      alarm_ON_ = !alarm_ON_;
      Serial.print(F("alarmOn = ")); Serial.println(alarm_ON_);
      break;
    case 'b':   // brightness
      {
        Serial.println(F("**** Set Brightness [0-255] ****"));
        while (Serial.available() == 0) {};
        int brightnessVal = Serial.parseInt();
        SerialInputFlush();
        display->SetBrightness(brightnessVal);
      }
      break;
    case 'g':   // good morning
      {
        display->GoodMorningScreen();
      }
      break;
    case 's':   // screensaver
      {
        Serial.println(F("**** Screensaver ****"));
        SetPage(kScreensaverPage);
      }
      break;
    case 't':   // go to buzzAlarm Function
      {
        Serial.println(F("**** buzzAlarm Function ****"));
        // go to buzz alarm function
        BuzzAlarmFn();
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        display->PrepareTimeDayDateArrays();
        // set main page back
        SetPage(kMainPage);
        inactivity_seconds_ = 0;
      }
      break;
    case 'y':   // show alarm triggered screen
      {
        Serial.println(F("**** Show Alarm Triggered Screen ****"));
        // start alarm triggered page
        SetPage(kAlarmTriggeredPage);
        delay(1000);
        display->AlarmTriggeredScreen(false, 24);
        delay(1000);
        display->AlarmTriggeredScreen(false, 13);
        delay(1000);
        display->AlarmTriggeredScreen(false, 14);
        delay(1000);
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        display->PrepareTimeDayDateArrays();
        // set main page back
        SetPage(kMainPage);
        inactivity_seconds_ = 0;
      }
      break;
    case 'w':   // get today's weather info
      {
        Serial.println(F("**** Get Weather Info ****"));
        // get today's weather info
        second_core_control_flag_ = 1;
      }
      break;
    case 'i':   // set WiFi details
      {
        Serial.println(F("**** Enter WiFi Details ****"));
        String inputStr;
        Serial.print("SSID: ");
        while(Serial.available() == 0) {
          delay(20);
        }
        delay(20);
        inputStr = Serial.readString();
        inputStr[inputStr.length()-1] = '\0';
        Serial.println(inputStr);
        // fill wifi_ssid
        if(wifi_stuff->wifi_ssid_ != NULL) {
          delete wifi_stuff->wifi_ssid_;
          wifi_stuff->wifi_ssid_ = NULL;
        }
        wifi_stuff->wifi_ssid_ = new char[inputStr.length()];   // allocate space
        strcpy(wifi_stuff->wifi_ssid_,inputStr.c_str());
        Serial.print("PASSWORD: ");
        while(Serial.available() == 0) {
          delay(20);
        }
        delay(20);
        inputStr = Serial.readString();
        inputStr[inputStr.length()-1] = '\0';
        Serial.println(inputStr);
        // fill wifi_ssid
        if(wifi_stuff->wifi_password_ != NULL) {
          delete wifi_stuff->wifi_password_;
          wifi_stuff->wifi_password_ = NULL;
        }
        wifi_stuff->wifi_password_ = new char[inputStr.length()];   // allocate space
        strcpy(wifi_stuff->wifi_password_,inputStr.c_str());
        wifi_stuff->SaveWiFiDetails();
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}

// Check if alarm time is hit
bool AlarmClock::TimeToStartAlarm() {

  if(!alarm_ON_) return false;

  if(rtc->hourModeAndAmPm() == 0) {
    // 24 hour clock mode
    uint8_t alarmHr24hr = alarm_hr_;
    if(alarm_hr_ == 12) {
      if(alarm_is_AM_) alarmHr24hr = 0;
      else alarmHr24hr = 12;
    }
    else {
      if(!alarm_is_AM_) alarmHr24hr -= 12;
    }
    // check if alarm is hit
    if(rtc->hour() == alarmHr24hr && rtc->minute() == alarm_min_)
      return true;
    else
      return false;
  }
  else { // 12 hour mode
    // check if alarm is hit
    if((rtc->hourModeAndAmPm() == 1 && alarm_is_AM_) || (rtc->hourModeAndAmPm() == 2 && !alarm_is_AM_)) {
      if(rtc->hour() == alarm_hr_ && rtc->minute() == alarm_min_)
        return true;
      else
        return false;
    }
    else
      return false;
  }
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
