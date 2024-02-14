#include "pin_defs.h"
#include <Arduino.h>
#include "alarm_clock_main.h"

// program setup function
void alarm_clock_main::setup(rgb_display_class* disp_ptr) {
  #if defined(MCU_IS_ESP32)
    setCpuFrequencyMhz(160);
  #endif

  Serial.begin(9600);
  delay(100);
  // while(!Serial) {};
  Serial.println(F("\nSerial OK"));

  // make all CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TS_CS_PIN, OUTPUT);
  digitalWrite(TS_CS_PIN, HIGH);

  #if defined(MCU_IS_RASPBERRY_PI_PICO_W) || defined(MCU_IS_ESP32)
    turn_WiFi_Off();
  #endif

  // retrieve saved settings
  retrieveSettings();

  // setup alarm clock program

  // store display object pointer
  this->display = disp_ptr;

  // initialize rtc time
  rtc_clock_initialize();

  // setup display
  display->setup(this);

  // seconds blink LED
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

// arduino loop function
void alarm_clock_main::loop() {
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

  if(pushBtn.buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);


  // process time actions every second
  if (alarm_clock_main::secondsIncremented) {
    alarm_clock_main::secondsIncremented = false;
    // update seconds
    ++second;
    if (second >= 60)
      refreshRtcTime = true;

    // prepare updated seconds array
    display->setSeconds(second);

    serialTimeStampPrefix();

    // blink LED every second
    blink = !blink;
    // digitalWrite(LED_PIN, blink);

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

    // update TFT display for changes
    if(currentPage == mainPage)
      display->displayTimeUpdate();
    else if(currentPage == screensaverPage)
      display->screensaver();

    // serial print RTC Date Time
    display->serialPrintRtcDateTime();

    Serial.println();

// #if defined(MCU_IS_ESP32)
//     // if button is inactive, then go to sleep
//     if(!pushBtn.buttonActiveDebounced())
//       putEsp32ToLightSleep();
// #endif
  }
  else if(currentPage == screensaverPage)
    display->screensaver();   // continous motion clock
  
  // accept user inputs
  if (Serial.available() != 0)
    processSerialInput();
}

void alarm_clock_main::setPage(ScreenPage page) {
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

void alarm_clock_main::retrieveSettings() {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);
  #endif

  #if defined(MCU_IS_TEENSY) || defined(MCU_IS_RASPBERRY_PI_PICO_W)

    // start reading from the first byte (address 0) of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    byte value;
    // read a byte from the current address of the EEPROM
    value = EEPROM.read(address);
    if(value == 1) {
      // alarm data is stored in EEPROM
      // retrieve it
      address++;
      alarmHr = EEPROM.read(address); address++;
      alarmMin = EEPROM.read(address); address++;
      alarmIsAm = EEPROM.read(address); address++;
      alarmOn = EEPROM.read(address); address++;

      Serial.println(F("Alarm settings retrieved from EEPROM."));

      #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
        // read WiFi SSID and Password
        char eeprom_read_array[WIFI_SSID_PASSWORD_LENGTH_MAX + 1];

        // read wifi_ssid
        int char_arr_start_address = address;
        while(1) {
          char eeprom_char_read = EEPROM.read(address);
          eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
          address++;
          // break at null character
          if(eeprom_char_read == '\0')
            break;
          // limit to force out of while loop, won't reach here in normal operation
          if(address >= char_arr_start_address + WIFI_SSID_PASSWORD_LENGTH_MAX) {
            eeprom_read_array[address - char_arr_start_address] = '\0';
            break;
          }
        }
        // fill wifi_ssid
        if(wifi_ssid != NULL) {
          delete wifi_ssid;
          wifi_ssid = NULL;
        }
        wifi_ssid = new char[address - char_arr_start_address];   // allocate space
        strcpy(wifi_ssid,eeprom_read_array);

        // read wifi_password
        char_arr_start_address = address;
        while(1) {
          char eeprom_char_read = EEPROM.read(address);
          eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
          address++;
          // break at null character
          if(eeprom_char_read == '\0')
            break;
          // limit to force out of while loop, won't reach here in normal operation
          if(address >= char_arr_start_address + WIFI_SSID_PASSWORD_LENGTH_MAX) {
            eeprom_read_array[address - char_arr_start_address] = '\0';
            break;
          }
        }
        // fill wifi_password
        if(wifi_password != NULL) {
          delete wifi_password;
          wifi_password = NULL;
        }
        wifi_password = new char[address - char_arr_start_address];   // allocate space
        strcpy(wifi_password,eeprom_read_array);

        // End reading EEPROM on Raspberry Pi Pico
        EEPROM.end();

        Serial.println(F("WiFi details retrieved from EEPROM."));

      #endif
    }
    else {
      #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
        // End reading EEPROM on Raspberry Pi Pico
        EEPROM.end();
      #endif
      // write alarm on EEPROM
      saveAlarm();
      // write WiFi details on EEPROM
      saveWiFiDetails();
    }
  #endif
}

void alarm_clock_main::saveAlarm() {
  alarmHr = var1;
  alarmMin = var2;
  alarmIsAm = var3AmPm;
  alarmOn = var4OnOff;

  #if defined(MCU_IS_TEENSY)

    // start writing from the first byte of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    // write alarm on EEPROM
    EEPROM.update(address, 1); address++;
    EEPROM.update(address, alarmHr); address++;
    EEPROM.update(address, alarmMin); address++;
    EEPROM.update(address, alarmIsAm); address++;
    EEPROM.update(address, alarmOn);

    Serial.println("Alarm written to EEPROM");

  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)

    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);

    // start writing from the first byte of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    // write alarm on EEPROM
    EEPROM.write(address, 1); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmHr); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmMin); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmIsAm); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmOn);
    EEPROM.commit();

    // End reading EEPROM on Raspberry Pi Pico
    EEPROM.end();
    Serial.println("Alarm written to EEPROM");

  #endif

}

// interrupt ISR
void alarm_clock_main::sqwPinInterruptFn() {
  alarm_clock_main::secondsIncremented = true;
}

#if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
void alarm_clock_main::saveWiFiDetails() {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);

    // start writing from the first byte of the EEPROM
    unsigned int address = WIFI_ADDRESS_EEPROM;

    // write wifi_ssid on EEPROM
    int i = 0;
    while(1) {
      char c = *wifi_ssid + i;
      EEPROM.write(address, c); address++;
      EEPROM.commit();
      i++;
      // break at null character
      if(c == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
        EEPROM.write(address, '\0'); address++;
        EEPROM.commit();
        break;
      }
    }
    
    // write wifi_password on EEPROM
    i = 0;
    while(1) {
      char c = *wifi_password + i;
      EEPROM.write(address, c); address++;
      EEPROM.commit();
      i++;
      // break at null character
      if(c == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
        EEPROM.write(address, '\0'); address++;
        EEPROM.commit();
        break;
      }
    }

    // End reading EEPROM on Raspberry Pi Pico
    EEPROM.end();

    Serial.println(F("WiFi ssid and password written to EEPROM"));

  #elif defined(MCU_IS_ESP32)

    Serial.println(F("WiFi details saving on ESP32 is not Implemented yet."));

  #endif

}

void alarm_clock_main::turn_WiFi_On() {
  Serial.println(F("Connecting to WiFi"));
  WiFi.persistent(true);
  delay(1);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("wifi_ssid "); Serial.println(wifi_ssid);
  Serial.print("wifi_password "); Serial.println(wifi_password);
  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    i++;
    if(i >= 10) break;
  }
  if(WiFi.status() == WL_CONNECTED)
    Serial.println(F("WiFi Connected."));
  else
    Serial.println(F("Could NOT connect to WiFi."));
}

void alarm_clock_main::turn_WiFi_Off() {
  WiFi.persistent(false);
  delay(1);
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  Serial.println(F("WiFi Off."));
}

void alarm_clock_main::getTodaysWeatherInfo() {
  // turn On Wifi
  turn_WiFi_On();

  // Your Domain name with URL path or IP address with path
  String openWeatherMapApiKey = "0fad3740b3a6b502ad57504f6fc3521e";

  // Replace with your country code and city
  String city = "San Diego";
  String countryCode = "840";

  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=imperial";

    WiFiClient client;
    HTTPClient http;
      
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    
    String jsonBuffer = "{}"; 
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      jsonBuffer = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
    
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
    }
    else {
      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Temperature: ");
      Serial.println(myObject["main"]["temp"]);
      Serial.print("Pressure: ");
      Serial.println(myObject["main"]["pressure"]);
      Serial.print("Humidity: ");
      Serial.println(myObject["main"]["humidity"]);
      Serial.print("Wind Speed: ");
      Serial.println(myObject["wind"]["speed"]);
    }
  }
  else {
    Serial.println("WiFi not connected");
  }

  // turn off WiFi
  turn_WiFi_Off();
}
#endif

// #if defined(MCU_IS_ESP32)
// /*
//   Esp32 light sleep function
//   https://lastminuteengineers.com/esp32-deep-sleep-wakeup-sources/
// */
// void alarm_clock_main::putEsp32ToLightSleep() {
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
// void alarm_clock_main::print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason){
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

void alarm_clock_main::rtc_clock_initialize() {

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

void alarm_clock_main::serialTimeStampPrefix() {
  Serial.print(F("("));
  Serial.print(millis());
  Serial.print(F(":s"));
  if(second < 10) Serial.print('0');
  Serial.print(second);
  Serial.print(F(":i"));
  Serial.print(inactivitySeconds);
  Serial.print(F(") - "));
  Serial.flush();
}

void alarm_clock_main::serial_input_flush() {
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

void alarm_clock_main::processSerialInput() {
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
        // start alarm triggered page
        getTodaysWeatherInfo();
        // refresh time
        refreshRtcTime = true;
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}

/*
  Check if alarm time is hit
*/
bool alarm_clock_main::timeToStartAlarm() {

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
void alarm_clock_main::buzzAlarmFn() {
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
void IRAM_ATTR alarm_clock_main::passiveBuzzerTimerISR() {
#elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
bool alarm_clock_main::passiveBuzzerTimerISR(struct repeating_timer *t) {
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

void alarm_clock_main::buzzer_enable() {
  // Timer Enable
  #if defined(MCU_IS_ESP32)
    timerAlarmEnable(passiveBuzzerTimerPtr);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    int64_t delay_us = 1000000 / (BUZZER_FREQUENCY * 2);
    add_repeating_timer_us(delay_us, passiveBuzzerTimerISR, NULL, passiveBuzzerTimerPtr);
  #endif
}

void alarm_clock_main::buzzer_disable() {
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

void alarm_clock_main::setupBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passiveBuzzerTimerPtr = timerBegin(1, 80, true);  // using timer 0, prescaler 80 (1MHz as ESP32 is 80MHz), counting up (true)
    timerAttachInterrupt(passiveBuzzerTimerPtr, &passiveBuzzerTimerISR, true);    //attach ISR to timer
    timerAlarmWrite(passiveBuzzerTimerPtr, 1000000 / (BUZZER_FREQUENCY * 2), true);
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    passiveBuzzerTimerPtr = new struct repeating_timer;
  #endif

  Serial.println(F("Timer setup successful!"));
}

void alarm_clock_main::deallocateBuzzerTimer() {

  #if defined(MCU_IS_ESP32)
    passiveBuzzerTimerPtr = NULL;
  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)
    delete passiveBuzzerTimerPtr;
    passiveBuzzerTimerPtr = NULL;
  #endif

  Serial.println(F("Buzzer Timer deallocated."));
}
