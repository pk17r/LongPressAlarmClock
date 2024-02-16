#include "hardware/sync.h"
#include "pin_defs.h"
#include <Arduino.h>
#include "alarm_clock.h"

// program setup function
void AlarmClock::setup() {
  #if defined(MCU_IS_ESP32)
    setCpuFrequencyMhz(160);
  #endif

  Serial.begin(9600);
  delay(100);
  // while(!Serial) { delay(20); };
  Serial.println(F("\nSerial OK"));

  // make all CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TS_CS_PIN, OUTPUT);
  digitalWrite(TS_CS_PIN, HIGH);

  // retrieve alarm settings
  eeprom = new EEPROM();
  retrieveAlarmSettings();

  // retrieve wifi details
  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
    wifiStuff = new WiFiStuff();
    wifiStuff->setup(eeprom);
    wifiStuff->turn_WiFi_Off();
  #endif

  // setup alarm clock program

  // initialize rtc time
  rtc_clock_initialize();

  // setup display
  display->setup();

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // initialize push button
  pushBtn.setButtonPin(BUTTON_PIN);
  // pushBtn.setButtonActiveLow(false);

  #if defined(TOUCHSCREEN_IS_XPT2046)
    // touchscreen setup and calibration
    ts.setupAndCalibrate(220, 3800, 280, 3830, 320, 240);
  #endif

  // seconds interrupt pin
  pinMode(SQW_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INT_PIN), sqwPinInterruptFn, RISING);

  // prepare date and time arrays and serial print RTC Date Time
  display->prepareTimeDayDateArrays();

  // serial print RTC Date Time
  display->serialPrintRtcDateTime();

  // update TFT display
  display->displayTimeUpdate();

  // set display brightness based on time of day
  display->checkTimeAndSetBrightness();

  // set sqw pin to trigger every second
  rtc.sqwgSetMode(URTCLIB_SQWG_1H);
}

// arduino loop function - High Priority one
void AlarmClock::updateTimePriorityLoop() {

    // button pressed or touchscreen touched
  if(pushBtn.checkButtonStatus() != 0 || ts.isTouched()) {
    // show instant response by turing up brightness
    display->setMaxBrightness();

    if(currentPage == screensaverPage)
    { // turn off screensaver if on
      setPage(mainPage);
    }
    else if(currentPage == alarmSetPage)
    { // if on alarm page, then take alarm set page user inputs
      display->setAlarmScreen(false, (ts.getTouchedPixel())->x, (ts.getTouchedPixel())->y);
    }
    else if(currentPage != alarmSetPage && inactivitySeconds >= 1)
    { // if on main page and user clicked somewhere, get touch input
      int userTouchRegion = display->classifyMainPageTouchInput((ts.getTouchedPixel())->x, (ts.getTouchedPixel())->y);
      if(userTouchRegion != -1)
        setPage((ScreenPage)userTouchRegion);
    }
    inactivitySeconds = 0;
  }

  // if user presses button, show response by turning On LED
  if(pushBtn.buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // process time actions every second
  if (AlarmClock::rtcHwSecUpdate) {
    AlarmClock::rtcHwSecUpdate = false;
    // update seconds
    ++second;
    if (second >= 60)
      refreshRtcTime = true;

    // prepare updated seconds array
    display->updateSecondsOnTimeStrArr(second);

    serialTimeStampPrefix();

    // get time update
    if (refreshRtcTime) {
      Serial.println(F("__RTC Refresh__ "));
      rtc.refresh();
      refreshRtcTime = false;

      // make second equal to rtc seconds -> should be 0
      second = rtc.second();

      serialTimeStampPrefix();

      // Check if time is up to date
      if (rtc.lostPower()) {
        Serial.println(F("POWER FAILED. Time is not up to date!"));
        Serial.println(F("Stopping!"));
        exit(1);
      }

      // Check whether OSC is set to use VBAT or not
      if (rtc.getEOSCFlag()) {
        Serial.println(F("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!"));
      }

      serialTimeStampPrefix();

      // if user has been inactive, then set brightness based on time of day
      // set display brightness based on time
      if(inactivitySeconds >= INACTIVITY_SECONDS_LIM)
        display->checkTimeAndSetBrightness();

      // if screensaver is On, then update time on it
      if(currentPage == screensaverPage)
        display->refreshScreensaverCanvas = true;

      // 5 mins before alarm time, try to get weather info
      #if defined(MCU_IS_RASPBERRY_PI_PICO_W) || defined(MCU_IS_ESP32)

        if(!wifiStuff->gotWeatherInfo && secondCoreControlFlag == 0) {

          if(alarmMin >= 5)
            if(rtc.hour() == alarmHr && rtc.minute() == alarmMin - 5)
              secondCoreControlFlag = 1;
          else if(alarmHr > 1)
            if(rtc.hour() == alarmHr - 1 && rtc.minute() == 55)
              secondCoreControlFlag = 1;
          else
            if(rtc.hour() == 12 && rtc.minute() == 55)
              secondCoreControlFlag = 1;
        }

      #endif

      // Activate Buzzer at Alarm Time
      if(timeToStartAlarm() && second < 10) {
        // go to buzz alarm function
        buzzAlarmFn();
        // refresh time
        rtc.refresh();
        // make second equal to rtc seconds
        second = rtc.second();
        // prepare date and time arrays
        display->prepareTimeDayDateArrays();
        // set main page back
        setPage(mainPage);
        inactivitySeconds = 0;
        wifiStuff->gotWeatherInfo = false;
      }
    }

    // prepare date and time arrays
    display->prepareTimeDayDateArrays();

    // check for inactivity
    if(inactivitySeconds <= INACTIVITY_SECONDS_LIM) {
      inactivitySeconds++;
      if(inactivitySeconds >= INACTIVITY_SECONDS_LIM) {
        // set display brightness based on time
        display->checkTimeAndSetBrightness();
        // turn screen saver On
        if(currentPage != screensaverPage)
          setPage(screensaverPage);
      }
    }

    // update time on main page
    if(currentPage == mainPage)
      display->displayTimeUpdate();

    // serial print RTC Date Time
    display->serialPrintRtcDateTime();
    Serial.println();

    // second core control operations
    switch(secondCoreControlFlag) {
      case 1: // resume the other core
        // rp2040.restartCore1();
        // rp2040.resumeOtherCore();
        secondCoreControlFlag = 2;  // core started
        // Serial.println("Resumed core1");
        break;
      case 3: // core1 is done processing and can be idled
        // rp2040.idleOtherCore();
        secondCoreControlFlag = 0;  // core idled
        // Serial.println("Idled core1");
        break;
    }
  }

  // make screensaver motion fast
  if(currentPage == screensaverPage)
    display->screensaver();

  // accept user inputs
  if (Serial.available() != 0)
    processSerialInput();

// #if defined(MCU_IS_ESP32)
//     // if button is inactive, then go to sleep
//     if(!pushBtn.buttonActiveDebounced())
//       putEsp32ToLightSleep();
// #endif

}

// arduino loop function - less priority one
void AlarmClock::nonPriorityTasksLoop() {

  // run the core only to do specific not time important operations
  if (secondCoreControlFlag == 2) {

    // get today's weather info
    wifiStuff->getTodaysWeatherInfo();

    // try once more if did not get info
    if(!wifiStuff->gotWeatherInfo)
      wifiStuff->getTodaysWeatherInfo();

    // done processing the task
    // set the core up to be idled from core0
    secondCoreControlFlag = 3;
  }

  // a delay to slow things down and not crash
  delay(1000);
}

void AlarmClock::setPage(ScreenPage page) {
  switch(page) {
    case mainPage:
      // if screensaver is active then clear screensaver canvas to free memory
      if(currentPage == screensaverPage)
        display->screensaverControl(false);
      currentPage = mainPage;         // new page needs to be set before any action
      display->redrawDisplay = true;
      display->displayTimeUpdate();
      break;
    case screensaverPage:
      currentPage = screensaverPage;      // new page needs to be set before any action
      display->screensaverControl(true);
      break;
    case alarmSetPage:
      currentPage = alarmSetPage;     // new page needs to be set before any action
      // set variables for alarm set screen
      var1 = alarmHr;
      var2 = alarmMin;
      var3AmPm = alarmIsAm;
      var4OnOff = alarmOn;
      display->setAlarmScreen(true, 0, 0);
      break;
    case alarmTriggeredPage:
      currentPage = alarmTriggeredPage;     // new page needs to be set before any action
      display->alarmTriggeredScreen(true, ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS);
      display->setMaxBrightness();
      break;
    case settingsPage:
      currentPage = settingsPage;     // new page needs to be set before any action
      display->settingsPage();
      display->setMaxBrightness();
      break;
    default:
      Serial.print("Unprogrammed Page "); Serial.print(page); Serial.println('!');
  }
}

void AlarmClock::retrieveAlarmSettings() {
  
  if(!(eeprom->retrieveAlarmSettings(alarmHr, alarmMin, alarmIsAm, alarmOn)))
    saveAlarm();
}

void AlarmClock::saveAlarm() {
  alarmHr = var1;
  alarmMin = var2;
  alarmIsAm = var3AmPm;
  alarmOn = var4OnOff;

  // save alarm settings
  eeprom->saveAlarm(alarmHr, alarmMin, alarmIsAm, alarmOn);
}

// interrupt ISR
void AlarmClock::sqwPinInterruptFn() {
  AlarmClock::rtcHwSecUpdate = true;
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

void AlarmClock::rtc_clock_initialize() {

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.begin();

  // set rtc model
  rtc.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231
  rtc.refresh();

  // to setup DS3231 - run this only for the first time during setup, thereafter set back to false and flash the code again so this is never run again
  if (0) {
    // Set Oscillator to use VBAT when VCC turns off
    if(rtc.enableBattery())
      Serial.println(F("Enable Battery Success"));
    else
      Serial.println(F("Enable Battery UNSUCCESSFUL!"));

    // disable SQ wave out
    rtc.disable32KOut();
    Serial.println(F("disable32KOut() done"));

    // stop sq wave on sqw pin
    rtc.sqwgSetMode(URTCLIB_SQWG_OFF_1);
    Serial.println(F("stop sq wave on sqw pin. Mode set: URTCLIB_SQWG_OFF_1"));

    // clear alarms flags
    rtc.alarmClearFlag(URTCLIB_ALARM_1);
    rtc.alarmClearFlag(URTCLIB_ALARM_2);
    Serial.println(F("alarmClearFlag() done"));

    // disable alarms
    rtc.alarmDisable(URTCLIB_ALARM_1);
    rtc.alarmDisable(URTCLIB_ALARM_2);
    Serial.println(F("alarmDisable() done"));

    // Set current time and date
    Serial.println();
    Serial.println(F("Waiting for input from user to set time."));
    Serial.println(F("Provide a keyboard input when set time is equal to real world time..."));
    while (Serial.available() == 0) {};
    // Only used once, then disabled
    rtc.set(0, 30, 2, 6, 26, 1, 24);
    //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
    Serial.println(F("Time set"));
    serial_input_flush();
  }

  // Check if time is up to date
  Serial.print(F("Lost power status: "));
  if (rtc.lostPower()) {
    Serial.println(F("POWER FAILED. Clearing flag..."));
    rtc.lostPowerClear();
  }
  else
    Serial.println(F("POWER OK"));

  // Check whether OSC is set to use VBAT or not
  if (rtc.getEOSCFlag())
    Serial.println(F("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!"));
  else
    Serial.println(F("Oscillator will use VBAT if VCC cuts off."));

  // we won't use RTC for alarm
  rtc.alarmDisable(URTCLIB_ALARM_1);
  rtc.alarmDisable(URTCLIB_ALARM_2);

  // make second equal to rtc second (-1 just to make first rtc refresh come later than when it should, as there is no time sync initially)
  second = rtc.second() - 1;
}

void AlarmClock::serialTimeStampPrefix() {
  Serial.print(F("(s"));
  if(second < 10) Serial.print('0');
  Serial.print(second);
  Serial.print(F(":i"));
  Serial.print(inactivitySeconds);
  Serial.print(F(": SRAM left: ")); Serial.print(freeRam());
  Serial.print(F(") - "));
  Serial.flush();
}

void AlarmClock::serial_input_flush() {
  while (true) {
    delay(20);  // give data a chance to arrive
    if (Serial.available()) {
      // we received something, get all of it and discard it
      while (Serial.available())
        Serial.read();
      continue;  // stay in the main loop
    } else
      break;  // nothing arrived for 20 ms
  }
}

void AlarmClock::processSerialInput() {
  // take user input
  char input = Serial.read();
  serial_input_flush();
  // acceptable user input
  Serial.print(F("User input: "));
  Serial.println(input);
  // process user input
  switch (input) {
    case 'a': // toggle alarm On Off
      Serial.println(F("**** Toggle Alarm ****"));
      alarmOn = !alarmOn;
      Serial.print(F("alarmOn = ")); Serial.println(alarmOn);
      break;
    case 'b':   // brightness
      {
        Serial.println(F("**** Set Brightness [0-255] ****"));
        while (Serial.available() == 0) {};
        int brightnessVal = Serial.parseInt();
        serial_input_flush();
        display->setBrightness(brightnessVal);
      }
      break;
    case 'd':  //disable battery
      {
        Serial.println(F("Disable Battery"));
        bool ret = rtc.disableBattery();
        if (ret) Serial.println(F("Disable Battery Success"));
        else Serial.println(F("Could not Disable Battery!"));
      }
      break;
    case 'e':  //enable battery
      {
        Serial.println(F("Enable Battery"));
        bool ret = rtc.enableBattery();
        if (ret) Serial.println(F("Enable Battery Success"));
        else Serial.println(F("Could not Enable Battery!"));
      }
      break;
    case 'g': // good morning
      {
        display->goodMorningScreen();
      }
      break;
    case 'h': // clock 12/24 hour mode
      {
        Serial.println(F("**** Set clock 12/24 hr mode ****"));
        Serial.println(F("Enter 'twelveHrMode' = 0 or 1"));
        while (Serial.available() == 0) {};
        unsigned int clockModeInp = Serial.parseInt();
        Serial.println(clockModeInp);
        serial_input_flush();
        rtc.set_12hour_mode((bool)clockModeInp);
        // print RTC Date Time and Alarm
        refreshRtcTime = true;
      }
      break;
    case 's':   // screensaver
      {
        Serial.println(F("**** Screensaver ****"));
        setPage(screensaverPage);
      }
      break;
    case 't':   // go to buzzAlarm Function
      {
        Serial.println(F("**** buzzAlarm Function ****"));
        // go to buzz alarm function
        buzzAlarmFn();
        // refresh time
        rtc.refresh();
        // make second equal to rtc seconds
        second = rtc.second();
        // prepare date and time arrays
        display->prepareTimeDayDateArrays();
        // set main page back
        setPage(mainPage);
        inactivitySeconds = 0;
      }
      break;
    case 'y':   // show alarm triggered screen
      {
        Serial.println(F("**** Show Alarm Triggered Screen ****"));
        // start alarm triggered page
        setPage(alarmTriggeredPage);
        delay(1000);
        display->alarmTriggeredScreen(false, 24);
        delay(1000);
        display->alarmTriggeredScreen(false, 13);
        delay(1000);
        display->alarmTriggeredScreen(false, 14);
        delay(1000);
        // refresh time
        rtc.refresh();
        // make second equal to rtc seconds
        second = rtc.second();
        // prepare date and time arrays
        display->prepareTimeDayDateArrays();
        // set main page back
        setPage(mainPage);
        inactivitySeconds = 0;
      }
      break;
    case 'w':   // get today's weather info
      {
        Serial.println(F("**** Get Weather Info ****"));
        // get today's weather info
        secondCoreControlFlag = 1;
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
        if(wifiStuff->wifi_ssid != NULL) {
          delete wifiStuff->wifi_ssid;
          wifiStuff->wifi_ssid = NULL;
        }
        wifiStuff->wifi_ssid = new char[inputStr.length()];   // allocate space
        strcpy(wifiStuff->wifi_ssid,inputStr.c_str());
        Serial.print("PASSWORD: ");
        while(Serial.available() == 0) {
          delay(20);
        }
        delay(20);
        inputStr = Serial.readString();
        inputStr[inputStr.length()-1] = '\0';
        Serial.println(inputStr);
        // fill wifi_ssid
        if(wifiStuff->wifi_password != NULL) {
          delete wifiStuff->wifi_password;
          wifiStuff->wifi_password = NULL;
        }
        wifiStuff->wifi_password = new char[inputStr.length()];   // allocate space
        strcpy(wifiStuff->wifi_password,inputStr.c_str());
        wifiStuff->saveWiFiDetails();
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}

/*
  Check if alarm time is hit
*/
bool AlarmClock::timeToStartAlarm() {

  if(!alarmOn) return false;

  if(rtc.hourModeAndAmPm() == 0) {
    // 24 hour clock mode
    uint8_t alarmHr24hr = alarmHr;
    if(alarmHr == 12) {
      if(alarmIsAm) alarmHr24hr = 0;
      else alarmHr24hr = 12;
    }
    else {
      if(!alarmIsAm) alarmHr24hr -= 12;
    }
    // check if alarm is hit
    if(rtc.hour() == alarmHr24hr && rtc.minute() == alarmMin)
      return true;
    else
      return false;
  }
  else { // 12 hour mode
    // check if alarm is hit
    if((rtc.hourModeAndAmPm() == 1 && alarmIsAm) || (rtc.hourModeAndAmPm() == 2 && !alarmIsAm)) {
      if(rtc.hour() == alarmHr && rtc.minute() == alarmMin)
        return true;
      else
        return false;
    }
    else
      return false;
  }
}

/*
  Function that starts buzzer and Alarm Screen
  It wait for user to press button to pause buzzer
  User needs to continue to press and hold button for
  ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS to end alarm.
  If user stops pressing button before alarm end, it will
  restart buzzer and the alarm end counter.
  If user does not end alarm by ALARM_MAX_ON_TIME_MS milliseconds,
  it will end alarm on its own.
*/
void AlarmClock::buzzAlarmFn() {
  // start alarm triggered page
  setPage(alarmTriggeredPage);
  // setup buzzer timer
  setupBuzzerTimer();
  //start buzzer!
  buzzer_enable();
  bool alarmStopped = false, buzzerPausedByUser = false;
  unsigned long alarmStartTimeMs = millis();
  int buttonPressSecondsCounter = ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS;
  while(!alarmStopped) {
    // if user presses button then pauze buzzer and start alarm end countdown!
    if(pushBtn.buttonActiveDebounced()) {
      if(!buzzerPausedByUser) {
        buzzer_disable();
        buzzerPausedByUser = true;
      }
      unsigned long buttonPressStartTimeMs = millis(); //note time of button press
      // while button is pressed, display seconds countdown
      while(pushBtn.buttonActiveDebounced() && !alarmStopped) {
        // display countdown to alarm off
        if(ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS - (millis() - buttonPressStartTimeMs) / 1000 < buttonPressSecondsCounter) {
          buttonPressSecondsCounter--;
          display->alarmTriggeredScreen(false, buttonPressSecondsCounter);
        }
        // end alarm after holding button for ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS
        if(millis() - buttonPressStartTimeMs > ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS * 1000) {
          alarmStopped = true;
          // good morning screen! :)
          display->goodMorningScreen();
        }
      }
    }
    // activate buzzer if button is not pressed by user
    if(!pushBtn.buttonActiveDebounced() && !alarmStopped) {
      if(buzzerPausedByUser) {
        buzzer_enable();
        buzzerPausedByUser = false;
      }
      // if user lifts button press before alarm end then reset counter and re-display alarm-On screen
      if(buttonPressSecondsCounter != ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS) {
        // display Alarm On screen with seconds user needs to press and hold button to end alarm
        buttonPressSecondsCounter = ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS;
        display->alarmTriggeredScreen(false, ALARM_END_BUTTON_PRESS_AND_HOLD_SECONDS);
      }
    }
    // if user did not stop alarm within ALARM_MAX_ON_TIME_MS, make sure to stop buzzer
    if(millis() - alarmStartTimeMs > ALARM_MAX_ON_TIME_MS) {
      buzzer_disable();
      alarmStopped = true;
    }
  }
  deallocateBuzzerTimer();
}

// Passive Buzzer Timer Interrupt Service Routine
#if defined(MCU_IS_ESP32)
void IRAM_ATTR AlarmClock::passiveBuzzerTimerISR() {
#elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
bool AlarmClock::passiveBuzzerTimerISR(struct repeating_timer *t) {
#endif
  // passiveBuzzerTimerISR() function
  if(millis() - _beepStartTimeMs > BEEP_LENGTH_MS) {
    _beepToggle = !_beepToggle;
    _beepStartTimeMs = millis();
    digitalWrite(LED_PIN, _beepToggle);
  }
  _buzzerSquareWaveToggle = !_buzzerSquareWaveToggle;
  digitalWrite(BUZZER_PIN, _buzzerSquareWaveToggle && _beepToggle);

  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    return true;
  #endif
}

void AlarmClock::buzzer_enable() {
  // Timer Enable
  #if defined(MCU_IS_ESP32)
    timerAlarmEnable(passiveBuzzerTimerPtr);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    int64_t delay_us = 1000000 / (BUZZER_FREQUENCY * 2);
    add_repeating_timer_us(delay_us, passiveBuzzerTimerISR, NULL, passiveBuzzerTimerPtr);
  #endif
}

void AlarmClock::buzzer_disable() {
  // Timer Disable
  #if defined(MCU_IS_ESP32)
    timerAlarmDisable(passiveBuzzerTimerPtr);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    cancel_repeating_timer(passiveBuzzerTimerPtr);
  #endif
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  _buzzerSquareWaveToggle = false;
  _beepToggle = false;
}

void AlarmClock::setupBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passiveBuzzerTimerPtr = timerBegin(1, 80, true);  // using timer 0, prescaler 80 (1MHz as ESP32 is 80MHz), counting up (true)
    timerAttachInterrupt(passiveBuzzerTimerPtr, &passiveBuzzerTimerISR, true);    //attach ISR to timer
    timerAlarmWrite(passiveBuzzerTimerPtr, 1000000 / (BUZZER_FREQUENCY * 2), true);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    passiveBuzzerTimerPtr = new struct repeating_timer;
  #endif

  Serial.println(F("Timer setup successful!"));
}

void AlarmClock::deallocateBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passiveBuzzerTimerPtr = NULL;
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    delete passiveBuzzerTimerPtr;
    passiveBuzzerTimerPtr = NULL;
  #endif

  Serial.println(F("Buzzer Timer deallocated."));
}
