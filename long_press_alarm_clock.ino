/**************************************************************************

# Long Press Alarm Clock

When alarm time hits, program requires user to press and hold the main LED push button for 20 (adjustable) seconds continously to turn off the alarm, making sure the user wakes up.
Without the button press, buzzer keeps buzzing. If user lets go of the push button before alarm end time, the buzzer restarts. Max buzzer time of 120 seconds.


Github: https://github.com/pk17r/Long_Press_Alarm_Clock/tree/release


- Software:
  - A fast very low RAM usage FastDrawTwoColorBitmapSpi display function is implemented that converts a full or section of a monochrome frame into 16-bit RGB565 frame with 2 colors 
  and sends data to display via SPI row by row in under 50ms for a full 320x240px frame and in only 20ms for 40% sized frame. This achieves a FPS of 20 frames per second for a
  full frame and a whooping 50 frames per second for 40% sized frames. Using this way of converting a monochrome image into 2 colors row by row saves a lot of RAM on the MCU as now
  we don't have to populate the whole 16-bit RGB565 frame, but only a 1-bit monochrome frame. This way a 153kB RGB565 frame on a 320x240px display is reduced to just 9.6kB, allowing 
  usage of lower RAM MCUs and much faster processing times per frame. A 40% sized canvas of a 320x240px display is made within 7ms on a 240MHz esp32. The screensaver implemented on
  this device achieves a whooping 45-50 frames per second speeds.
  - C++ OOP Based Project
  - All modules fully distributed in independent classes and header files
  - Arduino setup and loop functions in .ino file
  - MCU Selection and Module selections in configuration.h file, pin definitions in pin_defs.h file
  - A common header containing pointers to objects of every module and global functions
  - Adafruit Library used for GFX functions
  - uRTCLib Library for DS3231 updated with AM/PM mode and class size reduced by 3 bytes while adding additional functionality
  - Secure Web Over The Air Firmware Update Functionality
  - Watchdog keeps a check on the program and reboots MCU if it gets stuck
  - Modular programming that fits single core or dual core microcontrollers


- Hardware:
  - Microcontroller: ESP32 S2 Mini (Default) or ESP32 WROOM or Raspberry Pi Pico W
  - Display: 2.8" ST7789V display (Default), other selectable options: ST7735, ILI9341 and ILI9488
  - Touchscreen XPT2046 (not enabled by default)
  - DS3231 RTC Clock IC
  - AT24C32 EEPROM on DS3231 RTC Clock Module
  - A push button with LED
  - 2 push buttons for increase and decrease functions
  - An 85dB passive buzzer for alarm and different frequency tones


- Salient Features
  - There is no alarm snooze button.
  - Time update via NTP server using WiFi once every day to maintain high accuracy
  - DS3231 RTC itself is high accuracy clock having deviation of +/-2 minutes per year
  - Time auto adjusts for time zone and day light savings with location ZIP/PIN and country code
  - Get Weather info using WiFi and display today's weather after alarm
  - Get user input of WiFi details via an on-screen keyboard (when touchscreen is used and enabled)
  - Colorful Smooth Screensaver with a big clock
  - Touchscreen based alarm set page (touchscreen not on by default)
  - Settings saved in EEPROM so not lost on power loss
  - Screen brightness changes according to time of the day, with lowest brightness setting at night time
  - Time critical tasks happen on core0 - time update, screensaver fast motion, alarm time trigger
  - Non Time critical tasks happen on core1 - update weather info using WiFi, update time using NTP server, connect/disconnect WiFi
  - Very Low Power usage of 0.5W during day and 0.3W during night time


- Datasheets:
  - ESP32 Lolin S2 Mini Single Core MCU Board https://www.wemos.cc/en/latest/s2/s2_mini.html
  - ESP32 Lolin S2 Mini Pinouts https://www.studiopieters.nl/wp-content/uploads/2022/08/WEMOS-ESP32-S2-MINI.png
  - 2.8" Touchscreen ST7789V driver https://www.aliexpress.us/item/3256805747165796.html
  - 2.8" Touchscreen ILI9341 driver http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807


Prashant Kumar


***************************************************************************/
#include "common.h"
#include <PushButtonTaps.h>
#include "eeprom.h"
#if defined(WIFI_IS_USED)
  #include "wifi_stuff.h"
#endif
#include "rtc.h"
#include "alarm_clock.h"
#include "rgb_display.h"
#include "touchscreen.h"
#if defined(MCU_IS_ESP32)
  #include <esp_task_wdt.h>   // ESP32 Watchdog header
#endif

// modules - hardware or software
PushButtonTaps* push_button = NULL;   // Push Button object
PushButtonTaps* inc_button = NULL;   // Push Button object
PushButtonTaps* dec_button = NULL;   // Push Button object
EEPROM* eeprom = NULL;    // ptr to External EEPROM HW class object
WiFiStuff* wifi_stuff = NULL;  // ptr to wifi stuff class object that contains WiFi and Weather Fetch functions
RTC* rtc = NULL;  // ptr to class object containing RTC HW
AlarmClock* alarm_clock = NULL;  // ptr to alarm clock class object that controls Alarm functions
RGBDisplay* display = NULL;   // ptr to display class object that manages the display
Touchscreen* ts = NULL;         // Touchscreen class object

// LOCAL PROGRAM VARIABLES

#if defined(ESP32_DUAL_CORE)
  TaskHandle_t Task1;
#endif

SPIClass* spi_obj = NULL;

// random afternoon hour and minute to update firmware
uint16_t ota_update_days_minutes = 0;

// LOCAL FUNCTIONS
void PopulateDisplayPages();

// setup core1
void setup() {

  // make all spi CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  #if defined(TOUCHSCREEN_IS_XPT2046)
    pinMode(TS_CS_PIN, OUTPUT);
    digitalWrite(TS_CS_PIN, HIGH);
  #endif

  // make buzzer pin low
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // pullup debug pin
  pinMode(DEBUG_PIN, INPUT_PULLUP);

  // LED Pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // BUILTIN LED - we use for WiFi Connect Notification
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, LOW);

  Serial.begin(115200);

  // a delay to let currents stabalize
  // delay(500);

  // check if in debug mode
  debug_mode = !digitalRead(DEBUG_PIN);
  if(debug_mode) {
    // while(!Serial) { delay(20); };
    Serial.println(F("\nSerial OK"));
    Serial.println(F("******** DEBUG MODE ******** : watchdog won't be activated!"));
    Serial.flush();
  }
  else {
    Serial.println(F("\nSerial OK"));
    Serial.flush();
    // enable watchdog reset if not in debug mode
    SetWatchdogTime(kWatchdogTimeoutMs);
  }

  // initialize hardware spi
  #if defined(MCU_IS_RP2040)
    spi_obj = &SPI;
    spi_obj->begin();   // Hardware SPI
  #elif defined(MCU_IS_ESP32)
    spi_obj = new SPIClass(HSPI);
    #if defined(TOUCHSCREEN_IS_XPT2046)
      spi_obj->begin(TFT_CLK, TS_CIPO, TFT_COPI, TFT_CS); //SCLK, MISO, MOSI, SS
    #else
      spi_obj->begin(TFT_CLK, -1, TFT_COPI, TFT_CS); //SCLK, MISO, MOSI, SS
    #endif
  #endif

  // initialize push button
  push_button = new PushButtonTaps(BUTTON_PIN);
  inc_button = new PushButtonTaps(INC_BUTTON_PIN);
  dec_button = new PushButtonTaps(DEC_BUTTON_PIN);

  // initialize modules
  eeprom = new EEPROM();
  // check if firmware was updated
  std::string saved_firmware_version = "";
  eeprom->RetrieveSavedFirmwareVersion(saved_firmware_version);
  if(strcmp(saved_firmware_version.c_str(), kFirmwareVersion.c_str()) != 0) {
    firmware_updated_flag_user_information = true;
    Serial.print("Firmware updated from "); Serial.print(saved_firmware_version.c_str()); Serial.print(" to "); Serial.println(kFirmwareVersion.c_str());
    eeprom->SaveCurrentFirmwareVersion();
  }
  // setup ds3231 rtc module
  rtc = new RTC();
  // setup alarm clock
  alarm_clock = new AlarmClock();
  alarm_clock->Setup();
  // prepare date and time arrays and serial print RTC Date Time
  PrepareTimeDayDateArrays();
  // serial print RTC Date Time
  SerialPrintRtcDateTime();
  display = new RGBDisplay();
  PopulateDisplayPages();
  // setup display
  display->Setup();
  #if defined(TOUCHSCREEN_IS_XPT2046)
    ts = new Touchscreen();
  #endif
  // initialize wifi
  #if defined(WIFI_IS_USED)
    wifi_stuff = new WiFiStuff();
  #endif

  // second core task added flag array
  for (int i = 0; i < kNoTask; i++)
    second_core_task_added_flag_array[i] = false;

  // initialize random seed
  unsigned long seed = rtc->minute() * 60 + rtc->second();
  randomSeed(seed);

  // pick random time between 10AM and 6PM for firmware OTA update
  ota_update_days_minutes = random(600, 1080);
  uint8_t ota_update_hour_mode_and_am_pm, ota_update_hr, ota_update_min;
  rtc->DaysMinutesToClockTime(ota_update_days_minutes, ota_update_hour_mode_and_am_pm, ota_update_hr, ota_update_min);
  Serial.printf("OTA Update random 10AM-6PM time %02d:%02d %s\n", ota_update_hr, ota_update_min, (ota_update_hour_mode_and_am_pm == 1 ? kAmLabel : kPmLabel));
  Serial.printf("Active Firmware Version %s\n", kFirmwareVersion.c_str());
  Serial.printf("Active Firmware Date %s\n", kFirmwareDate.c_str());

  // set CPU Speed
  #if defined(MCU_IS_ESP32)
    uint32_t saved_cpu_speed_mhz = eeprom->RetrieveSavedCpuSpeed();
    if(saved_cpu_speed_mhz == 80 || saved_cpu_speed_mhz == 160 || saved_cpu_speed_mhz == 240)
      cpu_speed_mhz = saved_cpu_speed_mhz;
    setCpuFrequencyMhz(cpu_speed_mhz);
    cpu_speed_mhz = getCpuFrequencyMhz();
    Serial.printf("Updated CPU Speed to %u MHz\n", cpu_speed_mhz);
    eeprom->SaveCpuSpeed();
  #endif

  // set screensaver motion
  display->screensaver_bounce_not_fly_horizontally_ = eeprom->RetrieveScreensaverBounceNotFlyHorizontally();

  #if defined(ESP32_DUAL_CORE)
    xTaskCreatePinnedToCore(
        Task1code, /* Function to implement the task */
        "Task1", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        0,  /* Priority of the task */
        &Task1,  /* Task handle. */
        0); /* Core where the task should run */
  #endif
}

uint8_t frames_per_second = 0;

// arduino loop function on core0 - High Priority one with time update tasks
void loop() {
  // check if button pressed or touchscreen touched
  if((inactivity_millis >= kUserInputDelayMs) && (push_button->buttonActiveDebounced() || inc_button->buttonActiveDebounced() || dec_button->buttonActiveDebounced() || (ts != NULL && ts->IsTouched()))) {
    bool ts_input = (ts != NULL && ts->IsTouched());
    // show instant response by turing up brightness
    display->SetMaxBrightness();

    inactivity_millis = 0;

    // instant page change action
    if(current_page == kScreensaverPage)
    { // turn off screensaver if on
      SetPage(kMainPage);
    }
    else if(ts_input) {
      // Touch screen input
      if(current_page == kAlarmSetPage)
      { // if on alarm page, then take alarm set page user inputs
        display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ false, /* dec_button_pressed */ false, /* push_button_pressed */ false);
      }
      else // all other pages
      { // if not on alarm page and user clicked somewhere, get touch input
        ScreenPage userTouchRegion = display->ClassifyUserScreenTouchInput();
        if(userTouchRegion != kNoPageSelected)
          SetPage(userTouchRegion);
      }
    }
    // push/big LED button click action
    else if(push_button->buttonActiveDebounced()) {
      PrintLn("push_button");
      if(current_page == kAlarmSetPage)
        display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ false, /* dec_button_pressed */ false, /* push_button_pressed */ true);
      else {
        if(current_page == kMainPage) {                 // MAIN PAGE
          if(highlight == kMainPageSettingsWheel)
            SetPage(kSettingsPage);
          else if(highlight == kMainPageSetAlarm)
            SetPage(kAlarmSetPage);
        }
        else if(current_page == kSettingsPage) {        // SETTINGS PAGE
          if(highlight == kSettingsPageWiFi)
            SetPage(kWiFiSettingsPage);
          else if(highlight == kSettingsPageWeather) {
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageWeather);
            AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWeatherSettingsPage);
          }
          else if(highlight == kSettingsPageSet) {
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageSet);
            delay(kUserInputDelayMs);
            highlight = kSettingsPageAlarmLongPressSeconds;
            display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
          }
          else if(highlight == kSettingsPageAlarmLongPressSeconds) {
            highlight = kSettingsPageSet;
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageSet);
            eeprom->SaveLongPressSeconds(alarm_clock->alarm_long_press_seconds_);
            delay(kUserInputDelayMs);
            display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
          }
          else if(highlight == kSettingsPageScreensaverMotion) {
            highlight = kSettingsPageScreensaverMotion;
            display->screensaver_bounce_not_fly_horizontally_ = !display->screensaver_bounce_not_fly_horizontally_;
            eeprom->SaveScreensaverBounceNotFlyHorizontally(display->screensaver_bounce_not_fly_horizontally_);
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageScreensaverMotion);
            delay(kUserInputDelayMs);
            display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
          }
          else if(highlight == kSettingsPageScreensaverSpeed) {
            highlight = kSettingsPageScreensaverSpeed;
            CycleCpuFrequency();
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageScreensaverSpeed);
            delay(kUserInputDelayMs);
            display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
          }
          else if(highlight == kSettingsPageRunScreensaver)
            SetPage(kScreensaverPage);
          else if(highlight == kSettingsPageUpdate) {
            highlight = kSettingsPageUpdate;
            display->InstantHighlightResponse(/* color_button = */ kSettingsPageUpdate);
            wifi_stuff->FirmwareVersionCheck();
            display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
          }
          else if(highlight == kSettingsPageCancel)
            SetPage(kMainPage);
        }
        else if(current_page == kWiFiSettingsPage) {          // WIFI SETTINGS PAGE
          if(highlight == kWiFiSettingsPageSetSsidPasswd) {
            display->InstantHighlightResponse(/* color_button = */ kWiFiSettingsPageSetSsidPasswd);
            AddSecondCoreTaskIfNotThere(kStartSetWiFiSoftAP);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kSoftApInputsPage);
          }
          else if(highlight == kWiFiSettingsPageConnect) {
            display->InstantHighlightResponse(/* color_button = */ kWiFiSettingsPageConnect);
            AddSecondCoreTaskIfNotThere(kConnectWiFi);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWiFiSettingsPage);
          }
          else if(highlight == kWiFiSettingsPageDisconnect) {
            display->InstantHighlightResponse(/* color_button = */ kWiFiSettingsPageDisconnect);
            AddSecondCoreTaskIfNotThere(kDisconnectWiFi);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWiFiSettingsPage);
          }
          else if(highlight == kWiFiSettingsPageCancel)
            SetPage(kSettingsPage);
        }
        else if(current_page == kSoftApInputsPage) {          // SOFT AP SET WIFI SSID PASSWD PAGE
          if(highlight == kSoftApInputsPageSave) {
            display->InstantHighlightResponse(/* color_button = */ kSoftApInputsPageSave);
            AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
            WaitForExecutionOfSecondCoreTask();
            wifi_stuff->SaveWiFiDetails();
            SetPage(kWiFiSettingsPage);
          }
          else if(highlight == kSoftApInputsPageCancel) {
            display->InstantHighlightResponse(/* color_button = */ kSoftApInputsPageCancel);
            AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWiFiSettingsPage);
          }
        }
        else if(current_page == kWeatherSettingsPage) {       // WEATHER SETTINGS PAGE
          if(highlight == kWeatherSettingsPageSetLocation) {
            display->InstantHighlightResponse(/* color_button = */ kWeatherSettingsPageSetLocation);
            AddSecondCoreTaskIfNotThere(kStartLocationInputsLocalServer);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kLocationInputsPage);
          }
          else if(highlight == kWeatherSettingsPageSetCountryCode) {
            display->InstantHighlightResponse(/* color_button = */ kWeatherSettingsPageSetCountryCode);
            AddSecondCoreTaskIfNotThere(kStartLocationInputsLocalServer);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kLocationInputsPage);
          }
          else if(highlight == kWeatherSettingsPageUnits) {
            wifi_stuff->weather_units_metric_not_imperial_ = !wifi_stuff->weather_units_metric_not_imperial_;
            display->InstantHighlightResponse(/* color_button = */ kWeatherSettingsPageUnits);
            wifi_stuff->SaveWeatherUnits();
            wifi_stuff->got_weather_info_ = false;
            SetPage(kWeatherSettingsPage);
          }
          else if(highlight == kWeatherSettingsPageFetch) {
            display->InstantHighlightResponse(/* color_button = */ kWeatherSettingsPageFetch);
            AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWeatherSettingsPage);
          }
          else if(highlight == kWeatherSettingsPageUpdateTime) {
            display->InstantHighlightResponse(/* color_button = */ kWeatherSettingsPageUpdateTime);
            AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
            WaitForExecutionOfSecondCoreTask();
            if(wifi_stuff->manual_time_update_successful_)
              SetPage(kMainPage);
            else
              SetPage(kWeatherSettingsPage);
          }
          else if(highlight == kWeatherSettingsPageCancel)
            SetPage(kSettingsPage);
        }
        else if(current_page == kLocationInputsPage) {          // LOCATION INPUTS PAGE
          if(highlight == kLocationInputsPageSave) {
            display->InstantHighlightResponse(/* color_button = */ kLocationInputsPageSave);
            AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
            WaitForExecutionOfSecondCoreTask();
            wifi_stuff->SaveWeatherLocationDetails();
            wifi_stuff->got_weather_info_ = false;
            SetPage(kWeatherSettingsPage);
          }
          else if(highlight == kLocationInputsPageCancel) {
            display->InstantHighlightResponse(/* color_button = */ kLocationInputsPageCancel);
            AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
            WaitForExecutionOfSecondCoreTask();
            SetPage(kWeatherSettingsPage);
          }
        }
      }
    }
    else if(inc_button->buttonActiveDebounced()) {
      PrintLn("inc_button");
      if(current_page == kSettingsPage && highlight == kSettingsPageAlarmLongPressSeconds) {
        display->SettingsPage(true, false);
      }
      else if(current_page != kAlarmSetPage)
        MoveCursor(false);
      else
        display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ true, /* dec_button_pressed */ false, /* push_button_pressed */ false);
    }
    else if(dec_button->buttonActiveDebounced()) {
      PrintLn("dec_button");
      if(current_page == kSettingsPage && highlight == kSettingsPageAlarmLongPressSeconds) {
        display->SettingsPage(false, true);
      }
      else if(current_page != kAlarmSetPage)
        MoveCursor(true);
      else
        display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ false, /* dec_button_pressed */ true, /* push_button_pressed */ false);
    }

    // show firmware updated info only for the first time user uses the device
    firmware_updated_flag_user_information = false;
  }

  // if user presses button, show instant response by turning On LED
  if(push_button->buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // new second! Update Time!
  if (rtc->rtc_hw_sec_update_) {
    rtc->rtc_hw_sec_update_ = false;

    // if time is lost because of power failure
    if(rtc->year() < 2024 && !(wifi_stuff->incorrect_wifi_details_) && !(wifi_stuff->incorrect_zip_code)) {
      PrintLn("**** Update RTC HW Time from NTP Server ****");
      // update time from NTP server
      AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
      WaitForExecutionOfSecondCoreTask();
    }

    // new minute!
    if (rtc->rtc_hw_min_update_) {
      rtc->rtc_hw_min_update_ = false;

      // PrintLn("New Minute!");
      // Serial.print("CPU"); Serial.print(xPortGetCoreID()); Serial.print(" "); Serial.println(getCpuFrequencyMhz());

      // Activate Buzzer if Alarm Time has arrived
      if(rtc->year() >= 2024 && alarm_clock->MinutesToAlarm() == 0) {
        // go to buzz alarm function and show alarm triggered screen!
        alarm_clock->BuzzAlarmFn();
        // returned from Alarm Triggered Screen and Good Morning Screen
        // set main page
        SetPage(kMainPage);
        inactivity_millis = 0;
      }

      // if screensaver is On, then update time on it
      if(current_page == kScreensaverPage) {
        display->refresh_screensaver_canvas_ = true;
        // every new hour, show main page
        if(rtc->minute() == 0) {
          SetPage(kMainPage);
          inactivity_millis = 0;
        }
      }

      #if defined(WIFI_IS_USED)
        // try to get weather info 5 mins before alarm time
        if(alarm_clock->alarm_ON_ && (inactivity_millis > kInactivityMillisLimit) && !(wifi_stuff->incorrect_zip_code) && (alarm_clock->MinutesToAlarm() == 5)) {
          AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
          PrintLn("Get Weather Info!");
        }

        // reset time updated today to false every new day, for auto update of time at 2:01AM
        if(rtc->hourModeAndAmPm() == 1 && rtc->hour() == 12)
          wifi_stuff->auto_updated_time_today_ = false;

        // auto update time at 2AM 1 minute every morning (also accounts for day light savings that kicks in and ends at 2AM in March and November once every year, try for upto 59 times - once per min until successful time update)
        if(!(wifi_stuff->incorrect_zip_code) && !(wifi_stuff->auto_updated_time_today_) && rtc->hourModeAndAmPm() == 1 && rtc->hour() == 2 && rtc->minute() >= 1) {
          // update time from NTP server
          AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
          PrintLn("Get Time Update from NTP Server");
        }

        // check for firmware update everyday
        if(rtc->todays_minutes == ota_update_days_minutes) {
          PrintLn("**** Web OTA Firmware Update Check ****");
          AddSecondCoreTaskIfNotThere(kFirmwareVersionCheck);
          WaitForExecutionOfSecondCoreTask();
        }
      #endif

    }

    // prepare date and time arrays
    PrepareTimeDayDateArrays();

    // update time on main page
    if(current_page == kMainPage)
      display->DisplayTimeUpdate();

    // serial print RTC Date Time
    // SerialPrintRtcDateTime();

    // check for inactivity
    if(inactivity_millis > (((current_page != kSoftApInputsPage) || (current_page != kLocationInputsPage)) ? kInactivityMillisLimit : 5 * kInactivityMillisLimit)) {
      // if softap server is on, then end it
      if(current_page == kSoftApInputsPage)
        AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
      else if(current_page == kLocationInputsPage)
        AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
      #if defined(MCU_IS_ESP32_S3_DEVKIT_C1)
        // check photoresistor brightness and adjust display brightness
        display->CheckPhotoresistorAndSetBrightness();
      #else
        // set display brightness based on time
        display->CheckTimeAndSetBrightness();
      #endif
      // turn screen saver On
      if(current_page != kScreensaverPage)
        SetPage(kScreensaverPage);
    }

    #if defined(WIFI_IS_USED)
      // update firmware if available
      if(wifi_stuff->firmware_update_available_) {
        PrintLn("**** Web OTA Firmware Update ****");
        #if defined(MCU_IS_ESP32)
          // set Web OTA Update Pagte
          SetPage(kFirmwareUpdatePage);
          // Firmware Update
          wifi_stuff->UpdateFirmware();
          // set back main page if Web OTA Update unsuccessful
          SetPage(kMainPage);
        #endif
      }
    #endif

    // watchdog to reboot system if it gets stuck for whatever reason
    ResetWatchdog();

    // print fps
    if(debug_mode && current_page == kScreensaverPage) {
      // Serial.printf("FPS: %d\n", frames_per_second);
      PrintLn("FPS: ", frames_per_second);
      frames_per_second = 0;
    }
  }

  // make screensaver motion fast
  if(current_page == kScreensaverPage) {
    display->Screensaver();
    if(debug_mode) frames_per_second++;
  }

  // accept user serial inputs
  if (Serial.available() != 0) {
    // if(millis() > 2000)
      ProcessSerialInput();
    // else
    //   SerialInputFlush();
  }

  #if defined(MCU_IS_ESP32_S2_MINI)
    // ESP32_S2_MINI is single core MCU
    loop1();
  #endif
}

#if defined(MCU_IS_RP2040)
// setup core0
void setup1() {
  delay(2000);
}
#endif

// arduino loop function on core1 - low priority one with wifi weather update task
void loop1() {
  // run the core only to do specific not time important operations
  while (!second_core_tasks_queue.empty())
  {
    SecondCoreTask current_task = second_core_tasks_queue.front();
    // Serial.print("CPU"); Serial.print(xPortGetCoreID()); Serial.print(" "); Serial.println(getCpuFrequencyMhz());

    bool success = false;

    if(current_task == kGetWeatherInfo && (!wifi_stuff->got_weather_info_ || wifi_stuff->last_fetch_weather_info_time_ms_ == 0 || millis() - wifi_stuff->last_fetch_weather_info_time_ms_ > wifi_stuff->kFetchWeatherInfoMinIntervalMs)) {
      // get today's weather info
      wifi_stuff->GetTodaysWeatherInfo();
      success = wifi_stuff->got_weather_info_;

      // try once more if did not get info
      if(!success) {
        delay(1000);
        wifi_stuff->GetTodaysWeatherInfo();
        success = wifi_stuff->got_weather_info_;
      }
    }
    else if(current_task == kUpdateTimeFromNtpServer && (wifi_stuff->last_ntp_server_time_update_time_ms == 0 || millis() - wifi_stuff->last_ntp_server_time_update_time_ms > 10*1000)) {
      // get time from NTP server
      success = wifi_stuff->GetTimeFromNtpServer();
      // try once more if did not get info
      if(!success) {
        delay(1000);
        success = wifi_stuff->GetTimeFromNtpServer();
      }
      if(success)
        display->redraw_display_ = true;
    }
    else if(current_task == kConnectWiFi) {
      wifi_stuff->TurnWiFiOn();
      success = wifi_stuff->wifi_connected_;
    }
    else if(current_task == kDisconnectWiFi) {
      wifi_stuff->TurnWiFiOff();
      success = !(wifi_stuff->wifi_connected_);
    }
  #if defined(MCU_IS_ESP32)
    else if(current_task == kStartSetWiFiSoftAP) {
      wifi_stuff->StartSetWiFiSoftAP();
      success = true;
    }
    else if(current_task == kStopSetWiFiSoftAP) {
      wifi_stuff->StopSetWiFiSoftAP();
      success = true;
    }
    else if(current_task == kStartLocationInputsLocalServer) {
      wifi_stuff->StartSetLocationLocalServer();
      success = true;
    }
    else if(current_task == kStopLocationInputsLocalServer) {
      wifi_stuff->StopSetLocationLocalServer();
      success = true;
    }
    else if(current_task == kFirmwareVersionCheck) {
      wifi_stuff->TurnWiFiOn();
      ResetWatchdog();
      if(!(wifi_stuff->wifi_connected_))
        wifi_stuff->TurnWiFiOn();
      ResetWatchdog();
      wifi_stuff->FirmwareVersionCheck();
      success = true;
    }
  #endif

    // done processing the task
    // if(success) {
      second_core_tasks_queue.pop();
      delay(1);   // a delay to avoid race condition in dual core MCUs
      second_core_task_added_flag_array[current_task] = false;
    // }
  }
  // turn off WiFi if there are no more requests and User is not using device
  if(wifi_stuff->wifi_connected_ && inactivity_millis >= kInactivityMillisLimit)
    wifi_stuff->TurnWiFiOff();
}

#if defined(ESP32_DUAL_CORE)
void Task1code( void * parameter) {
  for(;;) 
    loop1();
}
#endif

void WaitForExecutionOfSecondCoreTask() {
  #if defined(ESP32_SINGLE_CORE)
    // ESP32_S2_MINI is single core MCU
    loop1();
  #elif defined(MCU_IS_RP2040) || defined(ESP32_DUAL_CORE)
    unsigned long time_start = millis();
    while (!second_core_tasks_queue.empty() && millis() - time_start <  kWatchdogTimeoutMs - 3000) {
      delay(10);
    }
  #endif
}

// GLOBAL VARIABLES AND FUNCTIONS

// debug mode turned On by pulling debug pin Low
bool debug_mode = false;

// firmware updated flag user information
bool firmware_updated_flag_user_information = false;

// CPU Speed for ESP32 CPU
uint32_t cpu_speed_mhz = 80;

// counter to note user inactivity seconds
elapsedMillis inactivity_millis = 0;

// Display Visible Data Structure variables
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// current cursor highlight location on page
Cursor highlight = kCursorNoSelection;

// second core current task queue
std::queue<SecondCoreTask> second_core_tasks_queue;
// second core task added flag
bool second_core_task_added_flag_array[kNoTask];

// function to safely add second core task if not already there
void AddSecondCoreTaskIfNotThere(SecondCoreTask task) {
  if(!second_core_task_added_flag_array[task]) {
    second_core_tasks_queue.push(task);
    second_core_task_added_flag_array[task] = true;
  }
}

int AvailableRam() {
  #if defined(MCU_IS_RP2040)
    // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#int-rp2040-getfreeheap
    return rp2040.getFreeHeap();
  #elif defined(MCU_IS_ESP32)
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html
    return esp_get_free_heap_size();
  #endif
}

void SerialInputWait() {
  while (Serial.available() == 0) // delay until something is received via serial
    delay(20);
  delay(20);  // give sometime to MCU to read serial input
}

void SerialInputFlush() {
  delay(20);  // give sometime to MCU to read serial input
  // if MCU received something, get all of it and discard it
  while (Serial.available()) {
    Serial.read();
    delay(20);  // give sometime to MCU to read next serial input
  }
  // nothing arrived for 20 ms, break off
}

void SerialTimeStampPrefix() {
  Serial.print(millis());
  Serial.print(kCharSpace);
  Serial.print('(');
  if(rtc != NULL) {
    Serial.print(rtc->hour());
    Serial.print(kCharColon);
    if(rtc->minute() < 10) Serial.print(kCharZero);
    Serial.print(rtc->minute());
    Serial.print(kCharColon);
    if(rtc->second() < 10) Serial.print(kCharZero);
    Serial.print(rtc->second());
    Serial.print(kCharSpace);
    if(rtc->hourModeAndAmPm() == 1)
      Serial.print(kAmLabel);
    else if(rtc->hourModeAndAmPm() == 2)
      Serial.print(kPmLabel);
  }
  Serial.print(" :i");
  // if(inactivity_millis < 100) Serial.print(kCharZero);
  // if(inactivity_millis < 10) Serial.print(kCharZero);
  Serial.print(inactivity_millis);
  Serial.print(": RAM "); Serial.print(AvailableRam());
  Serial.print(')');
  Serial.print(kCharSpace);
  Serial.flush();
}

void PrintLn(const char* someText1, const char* someText2) {
  SerialTimeStampPrefix();
  if(someText1 != nullptr)
    Serial.print(someText1);
  Serial.print(kCharSpace);
  if(someText2 != nullptr)
    Serial.print(someText2);
  Serial.println();
  Serial.flush();
}
void PrintLn(const char* someText1) {
  SerialTimeStampPrefix();
  if(someText1 != nullptr)
    Serial.print(someText1);
  Serial.println();
  Serial.flush();
}
void PrintLn(const char* someText1, int someInt) {
  SerialTimeStampPrefix();
  Serial.print(someText1);
  Serial.print(kCharSpace);
  Serial.println(someInt);
  Serial.flush();
}
void PrintLn(std::string someTextStr1, std::string someTextStr2) {
  SerialTimeStampPrefix();
  Serial.print(someTextStr1.c_str());
  Serial.print(kCharSpace);
  Serial.println(someTextStr2.c_str());
  Serial.flush();
}
void PrintLn(std::string &someTextStr1, std::string &someTextStr2) {
  SerialTimeStampPrefix();
  Serial.print(someTextStr1.c_str());
  Serial.print(kCharSpace);
  Serial.println(someTextStr2.c_str());
  Serial.flush();
}
void PrintLn(std::string &someTextStr1) {
  SerialTimeStampPrefix();
  Serial.println(someTextStr1.c_str());
  Serial.flush();
}
void PrintLn() {
  SerialTimeStampPrefix();
  Serial.println();
  Serial.flush();
}

void PrepareTimeDayDateArrays() {
  // HH:MM
  snprintf(new_display_data_.time_HHMM, kHHMM_ArraySize, "%d:%02d", rtc->hour(), rtc->minute());
  // :SS
  snprintf(new_display_data_.time_SS, kSS_ArraySize, ":%02d", rtc->second());
  if(rtc->hourModeAndAmPm() == 0)
    new_display_data_._12_hour_mode = false;
  else if(rtc->hourModeAndAmPm() == 1) {
    new_display_data_._12_hour_mode = true;
    new_display_data_.pm_not_am = false;
  }
  else {
    new_display_data_._12_hour_mode = true;
    new_display_data_.pm_not_am = true;
  }
  // Mon dd Day
  snprintf(new_display_data_.date_str, kDateArraySize, "%s  %d  %s", kDaysTable_[rtc->dayOfWeek() - 1], rtc->day(), kMonthsTable[rtc->month() - 1]);
  if(alarm_clock->alarm_ON_)
    snprintf(new_display_data_.alarm_str, kAlarmArraySize, "%d:%02d %s", alarm_clock->alarm_hr_, alarm_clock->alarm_min_, (alarm_clock->alarm_is_AM_ ? kAmLabel : kPmLabel));
  else
    snprintf(new_display_data_.alarm_str, kAlarmArraySize, "%s %s", kAlarmLabel, kOffLabel);
  new_display_data_.alarm_ON = alarm_clock->alarm_ON_;
}

void SerialPrintRtcDateTime() {
  SerialTimeStampPrefix();
  // full serial print time date day array
  Serial.print(new_display_data_.time_HHMM);
  // snprintf(timeArraySec, SS_ARR_SIZE, ":%02d", second);
  Serial.print(new_display_data_.time_SS);
  if (new_display_data_._12_hour_mode) {
    Serial.print(kCharSpace);
    if (new_display_data_.pm_not_am)
      Serial.print(kPmLabel);
    else
      Serial.print(kAmLabel);
  }
  Serial.print(kCharSpace);
  Serial.print(new_display_data_.date_str);
  Serial.print(kCharSpace);
  Serial.print(rtc->year());
  Serial.print(kCharSpace);
  Serial.println(new_display_data_.alarm_str);
  Serial.flush();
}

// set watchdog time
void SetWatchdogTime(unsigned long ms) {
  PrintLn("SetWatchdogTime ms = ", ms);
  #if defined(MCU_IS_RP2040)
    // watchdog to reboot system if it gets stuck for whatever reason for over 8.3 seconds
    // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#void-rp2040-wdt-begin-uint32-t-delay-ms
    rp2040.wdt_begin(ms);
  #elif defined(MCU_IS_ESP32)
    // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-reference/system/wdts.html
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
    esp_task_wdt_init(ms / 1000, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch
  #endif
}

// reset watchdog within time so it does not reboot system
void ResetWatchdog() {
  // reset MCU if not in debug mode
  if(!debug_mode) {
    #if defined(MCU_IS_RP2040)
      // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#hardware-watchdog
      rp2040.wdt_reset();
    #elif defined(MCU_IS_ESP32)
      // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
      // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-reference/system/wdts.html
      esp_task_wdt_reset();
    #endif
  }
}

void ProcessSerialInput() {
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
      alarm_clock->alarm_ON_ = !alarm_clock->alarm_ON_;
      Serial.print(F("alarmOn = ")); Serial.println(alarm_clock->alarm_ON_);
      break;
    case 'b':   // brightness
      {
        Serial.println(F("**** Set Brightness [0-255] ****"));
        SerialInputWait();
        int brightnessVal = Serial.parseInt();
        SerialInputFlush();
        display->SetBrightness(brightnessVal);
      }
      break;
    case 'c':   // connect to WiFi
      Serial.println(F("**** Connect to WiFi ****"));
      AddSecondCoreTaskIfNotThere(kConnectWiFi);
      break;
    case 'd':   // disconnect WiFi
      Serial.println(F("**** Disconnect WiFi ****"));
      AddSecondCoreTaskIfNotThere(kDisconnectWiFi);
      break;
    case 'e':   // setup ds3231 rtc
      Serial.println(F("**** setup ds3231 rtc ****"));
      rtc->Ds3231RtcSetup();
      Serial.println(F("DS3231 setup."));
      break;
    case 'f':   // toggle 12 / 24 hour mode
      Serial.println(F("**** toggle 12 / 24 hour mode ****"));
      if(rtc->hourModeAndAmPm() == 0)
        rtc->set_12hour_mode(true);
      else
        rtc->set_12hour_mode(false);
      Serial.print(F("RTC hourModeAndAmPm() = ")); Serial.println(rtc->hourModeAndAmPm());
      break;
    case 'g':   // good morning
      display->GoodMorningScreen();
      break;
    case 'i':   // set WiFi details
      {
        Serial.println(F("**** Enter WiFi Details ****"));
        String inputStr;
        Serial.print("SSID: ");
        SerialInputWait();
        inputStr = Serial.readString();
        wifi_stuff->wifi_ssid_ = "";
        for (int i = 0; i < min(inputStr.length(), kWifiSsidPasswordLengthMax); i++)
          if(inputStr[i] != '\0' && inputStr[i] != '\n')
            wifi_stuff->wifi_ssid_ = wifi_stuff->wifi_ssid_ + inputStr[i];
        Serial.println(wifi_stuff->wifi_ssid_.c_str());
        Serial.print("PASSWORD: ");
        SerialInputWait();
        inputStr = Serial.readString();
        wifi_stuff->wifi_password_ = "";
        for (int i = 0; i < min(inputStr.length(), kWifiSsidPasswordLengthMax); i++)
          if(inputStr[i] != '\0' && inputStr[i] != '\n')
            wifi_stuff->wifi_password_ = wifi_stuff->wifi_password_ + inputStr[i];
        Serial.println(wifi_stuff->wifi_password_.c_str());
        wifi_stuff->SaveWiFiDetails();
      }
      break;
    case 'j':   // cycle through CPU speeds
      Serial.println(F("**** cycle through CPU speeds ****"));
      CycleCpuFrequency();
      break;
    case 'k':   // set firmware updated flag true
      Serial.println(F("**** set firmware updated flag true ****"));
      firmware_updated_flag_user_information = true;
      break;
    case 'n':   // get time from NTP server and set on RTC HW
      Serial.println(F("**** Update RTC HW Time from NTP Server ****"));
      // update time from NTP server
      AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
      break;
    case 'o':   // On Screen User Text Input
      {
        Serial.println(F("**** On Screen User Text Input ****"));
        SetPage(kSettingsPage);
        // user input string
        char label[] = "SSID";
        char returnText[kWifiSsidPasswordLengthMax + 1] = "";
        // get user input from screen
        display->GetUserOnScreenTextInput(label, returnText);
        Serial.print("User Input :"); Serial.println(returnText);
        SetPage(kSettingsPage);
      }
      break;
    case 's':   // toggle screensaver
      Serial.println(F("**** toggle Screensaver ****"));
      if(current_page != kScreensaverPage)
        SetPage(kScreensaverPage);
      else
        SetPage(kMainPage);
      break;
    case 't':   // go to buzzAlarm Function
      Serial.println(F("**** buzzAlarm Function ****"));
      // go to buzz alarm function
      alarm_clock->BuzzAlarmFn();
      // set main page back
      SetPage(kMainPage);
      inactivity_millis = 0;
      break;
    case 'u':   // Web OTA Update Available Check
      Serial.println(F("**** Web OTA Update Available Check ****"));
      #if defined(MCU_IS_ESP32)
        wifi_stuff->FirmwareVersionCheck();
        wifi_stuff->firmware_update_available_ = false;
      #endif
      break;
    case 'v':   // Web OTA Update
      Serial.println(F("**** Web OTA Update Check ****"));
      #if defined(MCU_IS_ESP32)
        // if(wifi_stuff->firmware_update_available_) {
          ResetWatchdog();
          PrintLn("**** Web OTA Update Available ****");
          // set Web OTA Update Pagte
          SetPage(kFirmwareUpdatePage);
          // Firmware Update
          wifi_stuff->UpdateFirmware();
          // set back main page if Web OTA Update unsuccessful
          SetPage(kMainPage);
        // }
      #endif
      break;
    case 'w':   // get today's weather info
      Serial.println(F("**** Get Weather Info ****"));
      // get today's weather info
      AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
      break;
    case 'y':   // show alarm triggered screen
      Serial.println(F("**** Show Alarm Triggered Screen ****"));
      // start alarm triggered page
      SetPage(kAlarmTriggeredPage);
      delay(1000);
      display->AlarmTriggeredScreen(false, 24);
      delay(1000);
      display->AlarmTriggeredScreen(false, 12);
      delay(1000);
      display->AlarmTriggeredScreen(false, 5);
      delay(1000);
      // set main page back
      SetPage(kMainPage);
      inactivity_millis = 0;
      break;
    case 'z':   // screensaver settings
      {
        Serial.println(F("**** Screensaver Settings ****"));
        Serial.println(F("Fly Screensaver? (0/1):"));
        SerialInputWait();
        int userInput = Serial.parseInt();
        SerialInputFlush();
        display->screensaver_bounce_not_fly_horizontally_ = (userInput == 0 ? false : true);
        Serial.printf("fly_screensaver_horizontally_ = %d\n", display->screensaver_bounce_not_fly_horizontally_);
        Serial.println(F("Colored Border? (0/1):"));
        SerialInputWait();
        userInput = Serial.parseInt();
        SerialInputFlush();
        display->show_colored_edge_screensaver_ = (userInput == 0 ? false : true);
        Serial.printf("show_colored_edge_screensaver_ = %d\n", display->show_colored_edge_screensaver_);
        display->refresh_screensaver_canvas_ = true;
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}

void CycleCpuFrequency() {
  #if defined(MCU_IS_ESP32)
    cpu_speed_mhz = getCpuFrequencyMhz();
    Serial.printf("Current CPU Speed %u MHz\n", cpu_speed_mhz);
    // cycle through 80, 160 and 240
    if(cpu_speed_mhz == 160) setCpuFrequencyMhz(240);
    else if(cpu_speed_mhz == 240) setCpuFrequencyMhz(80);
    else setCpuFrequencyMhz(160);
    cpu_speed_mhz = getCpuFrequencyMhz();
    eeprom->SaveCpuSpeed();
    Serial.printf("Updated CPU Speed to %u MHz\n", cpu_speed_mhz);
  #endif
}

void SetPage(ScreenPage page) {
  switch(page) {
    case kMainPage:
      // if screensaver is active then clear screensaver canvas to free memory
      if(current_page == kScreensaverPage)
        display->ScreensaverControl(false);
      current_page = kMainPage;         // new page needs to be set before any action
      highlight = kCursorNoSelection;
      display->redraw_display_ = true;
      display->DisplayTimeUpdate();
      break;
    case kScreensaverPage:
      current_page = kScreensaverPage;      // new page needs to be set before any action
      display->ScreensaverControl(true);
      highlight = kCursorNoSelection;
      break;
    case kFirmwareUpdatePage:
      current_page = kFirmwareUpdatePage;
      display->FirmwareUpdatePage();
      break;
    case kAlarmSetPage:
      current_page = kAlarmSetPage;     // new page needs to be set before any action
      highlight = kAlarmSetPageHour;
      // set variables for alarm set screen
      alarm_clock->var_1_ = alarm_clock->alarm_hr_;
      alarm_clock->var_2_ = alarm_clock->alarm_min_;
      alarm_clock->var_3_is_AM_ = alarm_clock->alarm_is_AM_;
      alarm_clock->var_4_ON_ = alarm_clock->alarm_ON_;
      display->SetAlarmScreen(/* process_user_input */ false, /* inc_button_pressed */ false, /* dec_button_pressed */ false, /* push_button_pressed */ false);
      break;
    case kAlarmTriggeredPage:
      current_page = kAlarmTriggeredPage;     // new page needs to be set before any action
      display->AlarmTriggeredScreen(true, alarm_clock->alarm_long_press_seconds_);
      break;
    case kSettingsPage:
      current_page = kSettingsPage;     // new page needs to be set before any action
      highlight = kSettingsPageWiFi;
      // display->SettingsPage(false, false);
      display->DisplayPage(kSettingsPage);
      break;
    case kWiFiSettingsPage:
      current_page = kWiFiSettingsPage;     // new page needs to be set before any action
      highlight = kWiFiSettingsPageConnect;
      display->WiFiSettingsPage();
      break;
    case kSoftApInputsPage:
      current_page = kSoftApInputsPage;     // new page needs to be set before any action
      highlight = kSoftApInputsPageSave;
      display->SoftApInputsPage();
      break;
    case kEnterWiFiSsidPage:
      current_page = kWiFiSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen WiFi SSID Text Input ****");
        // user input string
        char label[] = "WiFi SSID";
        char returnText[kWifiSsidPasswordLengthMax + 1] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText);
        PrintLn(label, returnText);
        if(ret) {
          // set WiFi SSID:
          wifi_stuff->wifi_ssid_ = "";
          int i = 0;
          while(returnText[i] != '\0' && i <= kWifiSsidPasswordLengthMax) {
            wifi_stuff->wifi_ssid_ = wifi_stuff->wifi_ssid_ + returnText[i];
            i++;
          }
          PrintLn("EEPROM wifi_ssid: ", wifi_stuff->wifi_ssid_);
          wifi_stuff->SaveWiFiDetails();
        }
        SetPage(kWiFiSettingsPage);
      }
      break;
    case kEnterWiFiPasswdPage:
      current_page = kWiFiSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen WiFi PASSWD Text Input ****");
        // user input string
        char label[] = "WiFi PASSWD";
        char returnText[kWifiSsidPasswordLengthMax + 1] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText);
        PrintLn(label, returnText);
        if(ret) {
          // set WiFi Passwd:
          wifi_stuff->wifi_password_ = "";
          int i = 0;
          while(returnText[i] != '\0' && i <= kWifiSsidPasswordLengthMax) {
            wifi_stuff->wifi_password_ = wifi_stuff->wifi_password_ + returnText[i];
            i++;
          }
          PrintLn("EEPROM wifi_password_: ", wifi_stuff->wifi_password_);
          wifi_stuff->SaveWiFiDetails();
        }
        SetPage(kWiFiSettingsPage);
      }
      break;
    case kWeatherSettingsPage:
      current_page = kWeatherSettingsPage;     // new page needs to be set before any action
      highlight = kWeatherSettingsPageFetch;
      display->WeatherSettingsPage();
      break;
    case kLocationInputsPage:
      current_page = kLocationInputsPage;     // new page needs to be set before any action
      highlight = kLocationInputsPageSave;
      display->LocationInputsLocalServerPage();
      break;
    case kEnterWeatherLocationZipPage:
      current_page = kWeatherSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen ZIP/PIN Code Text Input ****");
        // user input string
        char label[] = "ZIP/PIN Code";
        char returnText[8] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText);
        PrintLn(label, returnText);
        if(ret) {
          // set Location ZIP code:
          wifi_stuff->location_zip_code_ = atoi(returnText);
          PrintLn("Location ZIP code: ", wifi_stuff->location_zip_code_);
          wifi_stuff->SaveWeatherLocationDetails();
        }
        SetPage(kWeatherSettingsPage);
      }
      break;
    case kEnterWeatherLocationCountryCodePage:
      current_page = kWeatherSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen Country Code Text Input ****");
        // user input string
        char label[] = "2 Letter Country Code";
        char returnText[3] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText);
        PrintLn(label, returnText);
        if(ret) {
          // set country code:
          wifi_stuff->location_country_code_.assign(returnText);
          PrintLn("country code: ", wifi_stuff->location_country_code_);
          wifi_stuff->SaveWeatherLocationDetails();
        }
        SetPage(kWeatherSettingsPage);
      }
      break;
    default:
      Serial.print("Unprogrammed Page "); Serial.print(page); Serial.println('!');
  }
  // if(current_page != kMainPage)
  //   display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
  delay(kUserInputDelayMs);
}

void MoveCursor(bool increment) {
  // Serial.print("MoveCursor top current_page "); Serial.print(current_page); Serial.print(" highlight "); Serial.println(highlight);
  if(current_page == kMainPage) {
    if(increment) {
      if(highlight == kMainPageSetAlarm)
        highlight = kCursorNoSelection;
      else
        highlight++;
    }
    else {
      if(highlight == kCursorNoSelection)
        highlight = kMainPageSetAlarm;
      else
        highlight--;
    }
  }
  else if(current_page == kSettingsPage) {
    if(increment) {
      if(highlight == kSettingsPageCancel)
        highlight = kSettingsPageWiFi;
      else
        highlight++;
    }
    else {
      if(highlight == kSettingsPageWiFi)
        highlight = kSettingsPageCancel;
      else
        highlight--;
    }
  }
  else if(current_page == kAlarmSetPage) {
    if(increment) {
      if(highlight == kCursorNoSelection)
        highlight = kAlarmSetPageHour;
      else if(highlight == kAlarmSetPageCancel)
        highlight = kCursorNoSelection;
      else
        highlight++;
    }
    else {
      if(highlight == kCursorNoSelection)
        highlight = kAlarmSetPageCancel;
      else if(highlight == kAlarmSetPageHour)
        highlight = kCursorNoSelection;
      else
        highlight--;
    }
  }
  else if(current_page == kWiFiSettingsPage) {
    if(increment) {
      if(highlight == kWiFiSettingsPageCancel)
        highlight = kWiFiSettingsPageSetSsidPasswd;
      else
        highlight++;
    }
    else {
      if(highlight == kWiFiSettingsPageSetSsidPasswd)
        highlight = kWiFiSettingsPageCancel;
      else
        highlight--;
    }
  }
  else if(current_page == kSoftApInputsPage) {
    if(highlight == kSoftApInputsPageSave)
      highlight = kSoftApInputsPageCancel;
    else
      highlight = kSoftApInputsPageSave;
  }
  else if(current_page == kWeatherSettingsPage) {
    if(increment) {
      if(highlight == kWeatherSettingsPageCancel)
        highlight = kWeatherSettingsPageSetLocation;
      else
        highlight++;
      #if !defined(TOUCHSCREEN_IS_XPT2046)
        if(highlight == kWeatherSettingsPageSetCountryCode)
          highlight++;
      #endif
    }
    else {
      if(highlight == kWeatherSettingsPageSetLocation)
        highlight = kWeatherSettingsPageCancel;
      else
        highlight--;
      #if !defined(TOUCHSCREEN_IS_XPT2046)
        if(highlight == kWeatherSettingsPageSetCountryCode)
          highlight--;
      #endif
    }
  }
  else if(current_page == kLocationInputsPage) {
    if(highlight == kLocationInputsPageSave)
      highlight = kLocationInputsPageCancel;
    else
      highlight = kLocationInputsPageSave;
  }
  // Serial.print("MoveCursor bottom current_page "); Serial.print(current_page); Serial.print(" highlight "); Serial.println(highlight);
  display->InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
}

void PopulateDisplayPages() {
  // display_pages_map[kMainPage] = std::vector<DisplayButton>{ 
  //   { /* Settings Wheel */ kRowHighlightOnlyClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
  //   { /* Alarms Row     */ kRowHighlightOnlyClickButton, kMainPageSetAlarm,  1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, "", "", false, false },
  // };
  // display_pages_map[kSettingsPage] = std::vector<DisplayButton>{ 
  //   { /* WiFi Details   */ kRowClickButton, kSettingsPageWiFi,  kWiFiSettingsButtonX1, kWiFiSettingsButtonY1, kWiFiSettingsButtonW, kWiFiSettingsButtonH, "WiFi Settings", "", false, false },
  //   { /* Weather Details   */ kRowClickButton, kSettingsPageWeather,  kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, "Weather Settings", "", false, false },
  //   { /* Alarm Long Press Seconds */ kRowValueIncDecButton, kSettingsPageAlarmLongPressSeconds,  kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY1, kAlarmLongPressSecondsW + kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsH, "Alarm Long Press Seconds", "", false, false },
  //   { /* Screensaver Motion Type */ kRowClickCycleButton, kSettingsPageScreensaverMotion,  kScreensaverMotionButtonX1, kScreensaverMotionButtonY1, kScreensaverMotionButtonW, kScreensaverMotionButtonH, "Screensaver Motion Type", "", false, false },
  //   { /* Screensaver Speed */ kRowClickCycleButton, kSettingsPageScreensaverSpeed,  kScreensaverSpeedButtonX1, kScreensaverSpeedButtonY1, kScreensaverSpeedButtonW, kScreensaverSpeedButtonH, "Screensaver Speed", "", false, false },
  //   { /* Run Screensaver */ kRowClickButton, kSettingsPageRunScreensaver,  kRunScreensaverButtonX1, kRunScreensaverButtonY1, kRunScreensaverButtonW, kRunScreensaverButtonH, "Run", "", false, false },
  //   { /* Firmware Update Button */ kRowClickButton, kSettingsPageUpdate,  kUpdateButtonX1, kUpdateButtonY1, kUpdateButtonW, kUpdateButtonH, "Firmware Update", "", false, false },
  //   { /* Cancel Button */ kRowClickButton, kSettingsPageCancel,  kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, "X", "", false, false },
  // };

  // std::map<Cursor, DisplayButton*> mainPageButtons;
  // mainPageButtons[kMainPageSettingsWheel] = new DisplayButton{ /* Settings Wheel */ kRowHighlightOnlyClickButton, 270, 105, 40, 40, "", "", false, false };
  // mainPageButtons[kMainPageSetAlarm] = new DisplayButton{ /* Alarms Row     */ kRowHighlightOnlyClickButton, 1, 160, 318, 79, "", "", false, false };
  // display_pages_map[kMainPage] = mainPageButtons;


  display_pages_vec[kMainPage] = std::vector<DisplayButton*> {
    new DisplayButton{ /* Settings Wheel */ kMainPageSettingsWheel, kIconButton, "", true, 270, 105, 40, 40, "", false },
    new DisplayButton{ /* Alarms Row     */ kMainPageSetAlarm, kIconButton, "", true, 1, 160, 318, 79, "", false },
  };

  display_pages_vec[kSettingsPage] = std::vector<DisplayButton*> {
    new DisplayButton{ /* WiFi Details   */ kSettingsPageWiFi, kRowClickButton, "WiFi Settings:", false, 0,0,0,0, "WIFI", false },
    new DisplayButton{ /* Weather Details   */ kSettingsPageWeather, kRowClickButton, "Weather Settings:", false, 0,0,0,0, "WEATHER", false },
    new DisplayButton{ /* Alarm Long Press Seconds */ kSettingsPageAlarmLongPressSeconds, kRowClickCycleButton, "Alarm Long Press Time:", false, 0,0,0,0, (std::to_string(alarm_clock->alarm_long_press_seconds_) + "sec"), false },
    new DisplayButton{ /* Screensaver Motion Type */ kSettingsPageScreensaverMotion, kRowClickCycleButton, "Screensaver Motion:", false, 0,0,0,0, (display->screensaver_bounce_not_fly_horizontally_ ? bounceScreensaverStr : flyOutScreensaverStr), false },
    new DisplayButton{ /* Screensaver Speed */ kSettingsPageScreensaverSpeed, kRowClickCycleButton, "Screensaver Speed:", false, 0,0,0,0, (cpu_speed_mhz == 80 ? slowStr : (cpu_speed_mhz == 160 ? medStr : fastStr)), false },
    new DisplayButton{ /* Run Screensaver */ kSettingsPageRunScreensaver, kRowClickButton, "Run Screensaver:", false, 0,0,0,0, "RUN", false },
    new DisplayButton{ /* Firmware Update Button */ kSettingsPageUpdate, kRowClickButton, "Firmware Update:", false, 0,0,0,0, "UP", false },
    new DisplayButton{ /* Cancel Button */ kSettingsPageCancel, kRowClickButton, "", true, kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, "X", false },
  };


}

std::vector<std::vector<DisplayButton*>> display_pages_vec(kNoPageSelected);

// std::vector<DisplayPage> display_pages_vec = {
//   // {kMainPage, { 
//   //     { /* Settings Wheel */ kRowHighlightOnlyClickButton, kMainPageSettingsWheel,  270, 105, 40, 40, "", "", false, false },
//   //     { /* Alarms Row     */ kRowHighlightOnlyClickButton, kMainPageSetAlarm,  1, 160, 318, 79, "", "", false, false },
//   //   }
//   // },
//   {kMainPage, { 
//       { /* Settings Wheel */ kRowHighlightOnlyClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
//       { /* Alarms Row     */ kRowHighlightOnlyClickButton, kMainPageSetAlarm,  1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, "", "", false, false },
//     }
//   },
//   {kSettingsPage, {
//       { /* WiFi Details   */ kRowClickButton, kSettingsPageWiFi,  kWiFiSettingsButtonX1, kWiFiSettingsButtonY1, kWiFiSettingsButtonW, kWiFiSettingsButtonH, "WiFi Settings", "", false, false },
//       { /* Weather Details   */ kRowClickButton, kSettingsPageWeather,  kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, "Weather Settings", "", false, false },
//       { /* Alarm Long Press Seconds */ kRowValueIncDecButton, kSettingsPageAlarmLongPressSeconds,  kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY1, kAlarmLongPressSecondsW + kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsH, "Alarm Long Press Seconds", "", false, false },
//       { /* Screensaver Motion Type */ kRowClickCycleButton, kSettingsPageScreensaverMotion,  kScreensaverMotionButtonX1, kScreensaverMotionButtonY1, kScreensaverMotionButtonW, kScreensaverMotionButtonH, "Screensaver Motion Type", "", false, false },
//       { /* Screensaver Speed */ kRowClickCycleButton, kSettingsPageScreensaverSpeed,  kScreensaverSpeedButtonX1, kScreensaverSpeedButtonY1, kScreensaverSpeedButtonW, kScreensaverSpeedButtonH, "Screensaver Speed", "", false, false },
//       { /* Run Screensaver */ kRowClickButton, kSettingsPageRunScreensaver,  kRunScreensaverButtonX1, kRunScreensaverButtonY1, kRunScreensaverButtonW, kRunScreensaverButtonH, "Run", "", false, false },
//       { /* Firmware Update Button */ kRowClickButton, kSettingsPageUpdate,  kUpdateButtonX1, kUpdateButtonY1, kUpdateButtonW, kUpdateButtonH, "Firmware Update", "", false, false },
//       { /* Cancel Button */ kRowClickButton, kSettingsPageCancel,  kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, "X", "", false, false },
//     }
//   },
//   // },
//   // {kSettingsPage, {
//   //     { /* Settings Wheel */ kRowClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
//   //   }
//   // },
//   // {kSettingsPage, {
//   //     { /* Settings Wheel */ kRowClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
//   //   }
//   // },
//   // {kSettingsPage, {
//   //     { /* Settings Wheel */ kRowClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
//   //   }
//   // },
//   // {kSettingsPage, {
//   //     { /* Settings Wheel */ kRowClickButton, kMainPageSettingsWheel,  kTftWidth - 50, kDateRow_Y0 - 35, 40, 40, "", "", false, false },
//   //   }
//   // },
// };








