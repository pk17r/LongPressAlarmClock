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
  - Settings saved in ESP32 NVM so not lost on power loss
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
#include "nvs_preferences.h"
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
#include <Adafruit_I2CDevice.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// modules - hardware or software
PushButtonTaps* push_button = NULL;   // Push Button object
PushButtonTaps* inc_button = NULL;   // Push Button object
PushButtonTaps* dec_button = NULL;   // Push Button object
NvsPreferences* nvs_preferences = NULL;    // ptr to NVS Preferences class object
WiFiStuff* wifi_stuff = NULL;  // ptr to wifi stuff class object that contains WiFi and Weather Fetch functions
RTC* rtc = NULL;  // ptr to class object containing RTC HW
AlarmClock* alarm_clock = NULL;  // ptr to alarm clock class object that controls Alarm functions
RGBDisplay* display = NULL;   // ptr to display class object that manages the display
Touchscreen* ts = NULL;         // Touchscreen class object

// LOCAL PROGRAM VARIABLES

#if defined(ESP32_DUAL_CORE)
  TaskHandle_t Task1;
#endif

// Arduino SPI Class Object
SPIClass* spi_obj = NULL;

// random afternoon hour and minute to update firmware
uint16_t ota_update_days_minutes = 0;

// RGB LED Strip Neopixels
Adafruit_NeoPixel* rgb_led_strip = NULL;
int rgb_strip_led_count = 4;  // rgb_led_strip
bool rgb_led_strip_on = false;
uint16_t current_led_strip_color = 0x6D9D;    // RGB565_Argentinian_blue
const uint32_t kDefaultLedStripColor = 0xFFFFFF;       // White
uint8_t rgb_strip_led_brightness = 255;

// LOCAL FUNCTIONS
// populate all pages in display_pages_vec
void PopulateDisplayPages();
int DisplayPagesVecCurrentButtonIndex();
int DisplayPagesVecButtonIndex(ScreenPage button_page, Cursor button_cursor);
void LedButtonClickUiResponse(int response_type);
void InitializeRgbLed();
void RunRgbLedAccordingToSettings();
const char* RgbLedSettingString();

// setup core1
void setup() {

  // make all spi CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  pinMode(TS_CS_PIN, OUTPUT);
  digitalWrite(TS_CS_PIN, HIGH);

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

  // TFT_RST - pull it low
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);

  // a delay to let currents stabalize and not have phantom serial inputs
  delay(1000);
  Serial.begin(115200);
  Serial.println(F("\nSerial OK"));

  // check if in debug mode
  debug_mode = !digitalRead(DEBUG_PIN);
  // debug_mode = true;
  if(debug_mode) {
    // while(!Serial) { delay(20); };   // Do not uncomment during commit!
    Serial.println(F("******** DEBUG MODE ******** : watchdog won't be activated!"));
  }
  else {
    // enable watchdog reset if not in debug mode
    SetWatchdogTime(kWatchdogTimeoutMs);
  }
  Serial.flush();

  // initialize hardware spi
  #if defined(MCU_IS_RP2040)
    spi_obj = &SPI;
    spi_obj->begin();   // Hardware SPI
  #elif defined(MCU_IS_ESP32)
    spi_obj = new SPIClass(HSPI);
    spi_obj->begin(TFT_CLK, TS_CIPO, TFT_COPI, TFT_CS); //SCLK, MISO, MOSI, SS
  #endif

  // initialize push button
  push_button = new PushButtonTaps(BUTTON_PIN);
  inc_button = new PushButtonTaps(INC_BUTTON_PIN);
  dec_button = new PushButtonTaps(DEC_BUTTON_PIN);

  // initialize modules
  // setup nvs preferences data (needs to be first)
  nvs_preferences = new NvsPreferences();
  // check if firmware was updated
  std::string saved_firmware_version = "";
  nvs_preferences->RetrieveSavedFirmwareVersion(saved_firmware_version);
  if(strcmp(saved_firmware_version.c_str(), kFirmwareVersion.c_str()) != 0) {
    firmware_updated_flag_user_information = true;
    Serial.print("Firmware updated from "); Serial.print(saved_firmware_version.c_str()); Serial.print(" to "); Serial.println(kFirmwareVersion.c_str());
    nvs_preferences->SaveCurrentFirmwareVersion();
  }
  // setup ds3231 rtc (needs to be before alarm clock)
  rtc = new RTC();
  // setup alarm clock (needs to be before display)
  alarm_clock = new AlarmClock();
  alarm_clock->Setup();
  // prepare date and time arrays and serial print RTC Date Time
  PrepareTimeDayDateArrays();
  // serial print RTC Date Time
  SerialPrintRtcDateTime();
  // initialize wifi (needs to be before display setup)
  #if defined(WIFI_IS_USED)
    wifi_stuff = new WiFiStuff();
  #endif
  // check if hardware has LDR
  use_photoresistor = nvs_preferences->RetrieveUseLdr();
  // initialize display class object
  display = new RGBDisplay();
  // setup and populate display
  display->Setup();
  if(nvs_preferences->RetrieveIsTouchscreen())
    ts = new Touchscreen();

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
    uint32_t saved_cpu_speed_mhz = nvs_preferences->RetrieveSavedCpuSpeed();
    if(saved_cpu_speed_mhz == 80 || saved_cpu_speed_mhz == 160 || saved_cpu_speed_mhz == 240)
      cpu_speed_mhz = saved_cpu_speed_mhz;
    setCpuFrequencyMhz(cpu_speed_mhz);
    cpu_speed_mhz = getCpuFrequencyMhz();
    Serial.printf("Updated CPU Speed to %u MHz\n", cpu_speed_mhz);
    nvs_preferences->SaveCpuSpeed();
  #endif

  // set screensaver motion
  display->screensaver_bounce_not_fly_horizontally_ = nvs_preferences->RetrieveScreensaverBounceNotFlyHorizontally();

  // initialize rgb led strip neopixels
  InitializeRgbLed();
  RunRgbLedAccordingToSettings();

  PopulateDisplayPages(); // needs to be after all saved values have been retrieved

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

  ResetWatchdog();
}

uint8_t frames_per_second = 0;

// arduino loop function on core0 - High Priority one with time update tasks
void loop() {
  // note if button pressed or touchscreen touched
  bool push_button_pressed = push_button->buttonActiveDebounced();
  bool inc_button_pressed = inc_button->buttonActiveDebounced();
  bool dec_button_pressed = dec_button->buttonActiveDebounced();

  // if user presses main LED Push button, show instant response by turning On LED
  if(push_button_pressed)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // if a button or touchscreen is pressed then take action
  if((inactivity_millis >= kUserInputDelayMs) && (push_button_pressed || inc_button_pressed || dec_button_pressed || (ts != NULL && ts->IsTouched()))) {
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
        current_cursor = display->CheckButtonTouch();
        if(current_cursor != kCursorNoSelection)
          LedButtonClickAction();
      }
    }
    // push/big LED button click action
    else if(push_button_pressed) {
      PrintLn("push_button");
      LedButtonClickAction();
    }
    else if(inc_button_pressed || dec_button_pressed) {
      if(inc_button_pressed) {
        PrintLn("inc_button_pressed");
        dec_button_pressed = false; // just having things clean, only have 1 button pressed at a time
      }
      else
        PrintLn("dec_button_pressed");
      // inc/dec button action
      if(current_page != kAlarmSetPage)
        MoveCursor(inc_button_pressed);
      else
        display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ inc_button_pressed, /* dec_button_pressed */ dec_button_pressed, /* push_button_pressed */ false);
    }

    // show firmware updated info only for the first time user uses the device
    firmware_updated_flag_user_information = false;
  }

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
        display->new_minute_ = true;
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

        // reset time updated today to false at midnight, for auto update of time at 2:05AM
        if(rtc->hourModeAndAmPm() == 1 && rtc->hour() == 12)
          wifi_stuff->auto_updated_time_today_ = false;

        // auto update time at 3:05 AM  every morning
        // (daylight savings time that kicks in and ends at 2AM in March and November once every year. At exactly 2AM, server time might not have updated)
        // try for upto 55 times - once per min until successful time update
        // time update will be checked using wifi_stuff->auto_updated_time_today_
        if(!(wifi_stuff->incorrect_zip_code) && !(wifi_stuff->auto_updated_time_today_) && (rtc->hourModeAndAmPm() == 1 && rtc->hour() == 3 && rtc->minute() >= 5)) {
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

        // auto disconnect wifi if connected and inactivity millis is over limit
        if(wifi_stuff->wifi_connected_ && (inactivity_millis > kInactivityMillisLimit)) {
          PrintLn("**** Auto disconnect WiFi ****");
          AddSecondCoreTaskIfNotThere(kDisconnectWiFi);
        }
      #endif

      // set rgb led strip
      RunRgbLedAccordingToSettings();
    }

    // prepare date and time arrays
    PrepareTimeDayDateArrays();

    // update time on main page
    if(current_page == kMainPage)
      display->DisplayTimeUpdate();

    // serial print RTC Date Time
    // SerialPrintRtcDateTime();

    // check for inactivity
    if(inactivity_millis > (((current_page == kSoftApInputsPage) || (current_page == kLocationInputsPage)) ? 20 * kInactivityMillisLimit : kInactivityMillisLimit)) {
      // if softap server is on, then end it
      if(current_page == kSoftApInputsPage)
        AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
      else if(current_page == kLocationInputsPage)
        AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
      if(use_photoresistor)
        // check photoresistor brightness and adjust display brightness
        display->CheckPhotoresistorAndSetBrightness();
      else
        // set display brightness based on time
        display->CheckTimeAndSetBrightness();
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

    // reset watchdog
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
  if (Serial.available() != 0)
    ProcessSerialInput();

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

// bool blink = false;
// unsigned long last_inactivity_millis = 0;

// arduino loop function on core1 - low priority one with wifi weather update task
void loop1() {
  ResetWatchdog();

  // color LED Strip sequentially
  if(rgb_led_strip_on && (current_led_strip_color != display->kColorPickerWheel[display->current_random_color_index_]))
    SetRgbStripColor(display->kColorPickerWheel[display->current_random_color_index_], /* set_color_sequentially = */ true);

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
        ResetWatchdog();
        wifi_stuff->GetTodaysWeatherInfo();
        success = wifi_stuff->got_weather_info_;
      }
    }
    else if(current_task == kUpdateTimeFromNtpServer && (wifi_stuff->last_ntp_server_time_update_time_ms == 0 || millis() - wifi_stuff->last_ntp_server_time_update_time_ms > 10*1000)) {
      // get time from NTP server
      success = wifi_stuff->GetTimeFromNtpServer();
      PrintLn("loop1(): wifi_stuff->GetTimeFromNtpServer() success = ", success);
      // try once more if did not get info
      if(!success) {
        delay(1000);
        ResetWatchdog();
        PrintLn("loop1(): wifi_stuff->GetTimeFromNtpServer() Trying again...");
        success = wifi_stuff->GetTimeFromNtpServer();
        PrintLn("loop1(): wifi_stuff->GetTimeFromNtpServer() success = ", success);
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
    else if(current_task == kScanNetworks) {
      success = wifi_stuff->WiFiScanNetworks();
    }
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
    while (!second_core_tasks_queue.empty() && millis() - time_start <  kWatchdogTimeoutMs - 2000) {
      delay(10);
    }
  #endif
}

// initialize RGB LED requires NVS Preferences to be loaded
void InitializeRgbLed() {
  if(rgb_led_strip != NULL) {
    rgb_led_strip->clear();
    delete rgb_led_strip;
  }
  rgb_strip_led_count = nvs_preferences->RetrieveRgbStripLedCount();
  rgb_strip_led_brightness = nvs_preferences->RetrieveRgbStripLedBrightness();
  rgb_led_strip = new Adafruit_NeoPixel(rgb_strip_led_count, RGB_LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);
  rgb_led_strip->begin();
  autorun_rgb_led_strip_mode = nvs_preferences->RetrieveAutorunRgbLedStripMode();
}

void RunRgbLedAccordingToSettings() {
  // set rgb led strip
  if(autorun_rgb_led_strip_mode == 3) { // run rgb led strip all evening + night
    if(rtc->todays_minutes >= kEveningTimeMinutes || rtc->todays_minutes < kDayTimeMinutes)
      TurnOnRgbStrip();
    else
      TurnOffRgbStrip();
  }
  else if(autorun_rgb_led_strip_mode == 2) {  // // run rgb led strip all evening only
    if(rtc->todays_minutes >= kEveningTimeMinutes && rtc->todays_minutes < night_time_minutes)
      TurnOnRgbStrip();
    else
      TurnOffRgbStrip();
  }
  else if(autorun_rgb_led_strip_mode == 1)
    TurnOnRgbStrip();
  else
    TurnOffRgbStrip();
}

const char* RgbLedSettingString() {
  switch (autorun_rgb_led_strip_mode) {
    case 0: return kManualOffStr;
    case 1: return kManualOnStr;
    case 2: return kEveningStr;
    case 3: return kSunDownStr;
    default: return kManualOffStr;
  }
}

// GLOBAL VARIABLES AND FUNCTIONS

bool use_photoresistor = false;

int current_rgb_led_strip_index = 0;

/* 0 = manual OFF
 1 = manual ON
 2 = autorun at evening
 3 = autorun at sun-down */
uint8_t autorun_rgb_led_strip_mode = 2;

// minute of day at which to dim display to night time brightness if not using a LDR
uint16_t night_time_minutes = 1320;

// debug mode turned On by pulling debug pin Low
bool debug_mode = false;

// firmware updated flag user information
bool firmware_updated_flag_user_information = false;

// all display buttons vector
std::vector<std::vector<DisplayButton*>> display_pages_vec(kNoPageSelected);

// CPU Speed for ESP32 CPU
uint32_t cpu_speed_mhz = 80;

// counter to note user inactivity seconds
elapsedMillis inactivity_millis = 0;

// Display Visible Data Structure variables
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// current cursor highlight location on page
Cursor current_cursor = kCursorNoSelection;

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
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // Code for version 3.x
      // esp32 watchdog example https://iotassistant.io/esp32/fixing-error-hardware-wdt-arduino-esp32/
      esp_task_wdt_config_t twdt_config = {
          .timeout_ms = ms,
          .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
          .trigger_panic = true,
      };
      esp_task_wdt_deinit(); //wdt is enabled by default, so we need to deinit it first
      esp_task_wdt_init(&twdt_config); //enable panic so ESP32 restarts
      esp_task_wdt_add(NULL); //add current thread to WDT watch
    #else
    // Code for version 2.x
      // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
      // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-reference/system/wdts.html
      // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
      esp_task_wdt_init(ms / 1000, true); //enable panic so ESP32 restarts
      esp_task_wdt_add(NULL); //add current thread to WDT watch
    #endif
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

  if(millis() < 2000) {
    Serial.println("Serial Input Ignored..");
    return;
  }
  // process user input
  switch (input) {
    case 'a':   // toggle alarm On Off
      Serial.println(F("**** Toggle Alarm ****"));
      alarm_clock->alarm_ON_ = !alarm_clock->alarm_ON_;
      Serial.print(F("alarmOn = ")); Serial.println(alarm_clock->alarm_ON_);
      break;
    case 'b':   // RGB LED brightness
      {
        Serial.println(F("**** Set RGB Brightness [0-255] ****"));
        SerialInputWait();
        int brightnessVal = Serial.parseInt();
        SerialInputFlush();
        nvs_preferences->SaveRgbStripLedBrightness(brightnessVal);
        InitializeRgbLed();
        RunRgbLedAccordingToSettings();
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
    case 'h':   // enable / disable TOUCHSCREEN
      if(ts != NULL) {
        delete ts;
        ts = NULL;
        nvs_preferences->SaveIsTouchscreen(false);
      }
      else {
        nvs_preferences->SaveIsTouchscreen(true);
        ts = new Touchscreen();
      }
      PrintLn("RetrieveIsTouchscreen() = ", nvs_preferences->RetrieveIsTouchscreen());
      break;
    case 'i':   // set WiFi details
      {
        // increase watchdog timeout to 90s
        if(!debug_mode) SetWatchdogTime(kWatchdogTimeoutOtaUpdateMs);

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

        // set back watchdog timeout
        if(!debug_mode) SetWatchdogTime(kWatchdogTimeoutMs);
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
    case 'l':   // nvs_preferences->RetrieveScreenOrientation()
      nvs_preferences->RetrieveScreenOrientation();
      break;
    case 'm':   // RotateScreen();
      display->RotateScreen();
      if(ts != NULL)
          ts->SetTouchscreenOrientation();
      display->redraw_display_ = true;
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
        std::string label = "WiFi Password";
        char returnText[kWifiSsidPasswordLengthMax + 1] = "";
        // get user input from screen
        display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ false);
        Serial.print("User Input :"); Serial.println(returnText);
        SetPage(kSettingsPage);
      }
      break;
    case 'p':   // turn ON RGB LED Strip
      TurnOnRgbStrip();
      break;
    case 'q':   // turn OFF RGB LED Strip
      TurnOffRgbStrip();
      break;
    case 'r':   // Set RGB LED Count
      {
        Serial.println(F("**** Set RGB LED Count [0-255] ****"));
        SerialInputWait();
        int rgb_strip_led_count_user = Serial.parseInt();
        SerialInputFlush();
        nvs_preferences->SaveRgbStripLedCount(rgb_strip_led_count_user);
        InitializeRgbLed();
        RunRgbLedAccordingToSettings();
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
    case 'x':   // toggle RGB LED Strip Mode
      if(autorun_rgb_led_strip_mode < 3)
        autorun_rgb_led_strip_mode++;
      else
        autorun_rgb_led_strip_mode = 0;
      nvs_preferences->SaveAutorunRgbLedStripMode(autorun_rgb_led_strip_mode);
      PrintLn("RGB LED Strip Mode = ", RgbLedSettingString());
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
    nvs_preferences->SaveCpuSpeed();
    Serial.printf("Updated CPU Speed to %u MHz\n", cpu_speed_mhz);
  #endif
}

void SetRgbStripColor(uint16_t rgb565_color, bool set_color_sequentially) {
  if(!rgb_led_strip_on)
    return;

  // RGB565 to RGB888
  byte r = byte(((rgb565_color & 0xF800) >> 11) << 3);
  byte g = byte(((rgb565_color & 0x7E0) >> 5) << 2);
  byte b = byte(((rgb565_color & 0x1F)) << 3);
  uint32_t rgb888 = (uint32_t(r) << 16) + (uint32_t(g) << 8) + uint32_t(b);
  // Serial.printf("rgb565_color = 0x%X   r=%d  g=%d  b=%d    rgb888 = 0x%X\n", rgb565_color, r, g, b, rgb888);
  // color rgb_led_strip
  if(set_color_sequentially) {
    rgb_led_strip->setPixelColor(current_rgb_led_strip_index, rgb888);
    current_rgb_led_strip_index++;
    if(current_rgb_led_strip_index == rgb_strip_led_count) {
      current_rgb_led_strip_index = 0;
      // stop further color changes until screensaver color change
      current_led_strip_color = display->kColorPickerWheel[display->current_random_color_index_];
    }
  }
  else {
    rgb_led_strip->fill(rgb888, 0, 0);
  }
  rgb_led_strip->show();
}

void TurnOnRgbStrip() {
  rgb_led_strip->setBrightness(rgb_strip_led_brightness);
  rgb_led_strip->fill(kDefaultLedStripColor, 0, 0);
  rgb_led_strip->show();
  rgb_led_strip_on = true;
  PrintLn("TurnOnRgbStrip()");
}

void TurnOffRgbStrip() {
  // rgb_led_strip->fill(0x000000, 0, 0);
  rgb_led_strip->setBrightness(0);
  rgb_led_strip->show();
  rgb_led_strip_on = false;
  PrintLn("TurnOffRgbStrip()");
}

bool AnyButtonPressed() {
  if(push_button->buttonActiveDebounced() || inc_button->buttonActiveDebounced() || dec_button->buttonActiveDebounced())
    return true;
  else
    return false;
}

void SetPage(ScreenPage set_this_page) {
  SetPage(set_this_page, /* bool move_cursor_to_first_button = */ true, /* bool increment_page = */ false);
}

void SetPage(ScreenPage set_this_page, bool move_cursor_to_first_button) {
  SetPage(set_this_page, move_cursor_to_first_button, /* bool increment_page = */ false);
}

void SetPage(ScreenPage set_this_page, bool move_cursor_to_first_button, bool increment_page) {
  switch(set_this_page) {
    case kMainPage:
      // if screensaver is active then clear screensaver canvas to free memory
      if(current_page == kScreensaverPage)
        display->ScreensaverControl(false);
      current_page = set_this_page;         // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kCursorNoSelection;
      display->redraw_display_ = true;
      display->DisplayTimeUpdate();
      // useful flag to show on UI the latest firmware in Settings Page
      wifi_stuff->firmware_update_available_str_ = "";
      break;
    case kScreensaverPage:
      current_page = set_this_page;      // new page needs to be set before any action
      display->ScreensaverControl(true);
      if(move_cursor_to_first_button) current_cursor = kCursorNoSelection;
      break;
    case kFirmwareUpdatePage:
      current_page = set_this_page;
      display->FirmwareUpdatePage();
      break;
    case kAlarmSetPage:
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kAlarmSetPageHour;
      // set variables for alarm set screen
      alarm_clock->var_1_ = alarm_clock->alarm_hr_;
      alarm_clock->var_2_ = alarm_clock->alarm_min_;
      alarm_clock->var_3_is_AM_ = alarm_clock->alarm_is_AM_;
      alarm_clock->var_4_ON_ = alarm_clock->alarm_ON_;
      display->SetAlarmScreen(/* process_user_input */ false, /* inc_button_pressed */ false, /* dec_button_pressed */ false, /* push_button_pressed */ false);
      break;
    case kAlarmTriggeredPage:
      current_page = set_this_page;     // new page needs to be set before any action
      display->AlarmTriggeredScreen(true, alarm_clock->alarm_long_press_seconds_);
      break;
    case kWiFiSettingsPage:
      // try to connect to WiFi
      AddSecondCoreTaskIfNotThere(kConnectWiFi);
      WaitForExecutionOfSecondCoreTask();
      // show page
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kWiFiSettingsPageScanNetworks;
      display->DisplayCurrentPage();
      break;
    case kSettingsPage:
    case kLocationAndWeatherSettingsPage:
    case kScreensaverSettingsPage:
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = display_pages_vec[current_page][0]->btn_cursor_id;
      display->DisplayCurrentPage();
      break;
    case kWiFiScanNetworksPage:
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kWiFiScanNetworksPageRescan;
      display->WiFiScanNetworksPage(increment_page);
      break;
    case kSoftApInputsPage:
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kPageSaveButton;
      display->SoftApInputsPage();
      break;
    case kLocationInputsPage:
      current_page = set_this_page;     // new page needs to be set before any action
      if(move_cursor_to_first_button) current_cursor = kPageSaveButton;
      display->LocationInputsLocalServerPage();
      break;
    case kEnterWeatherLocationZipPage:
      current_page = kLocationAndWeatherSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen ZIP/PIN Code Text Input ****");
        // user input string
        std::string label = "ZIP/PIN Code";
        char returnText[8] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ true);
        PrintLn(label, returnText);
        if(ret) {
          // set Location ZIP code:
          wifi_stuff->location_zip_code_ = atoi(returnText);
          PrintLn("Location ZIP code: ", wifi_stuff->location_zip_code_);
          wifi_stuff->SaveWeatherLocationDetails();
        }
        SetPage(kLocationAndWeatherSettingsPage);
      }
      break;
    case kEnterWeatherLocationCountryCodePage:
      current_page = kLocationAndWeatherSettingsPage;     // new page needs to be set before any action
      {
        PrintLn("**** On Screen Country Code Text Input ****");
        // user input string
        std::string label = "Two Letter Country Code";
        char returnText[3] = "";
        // get user input from screen
        bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ false);
        PrintLn(label, returnText);
        if(ret) {
          // set country code:
          wifi_stuff->location_country_code_.assign(returnText);
          PrintLn("country code: ", wifi_stuff->location_country_code_);
          wifi_stuff->SaveWeatherLocationDetails();
        }
        SetPage(kLocationAndWeatherSettingsPage);
      }
      break;
    default:
      Serial.print("Unprogrammed Page "); Serial.print(set_this_page); Serial.println('!');
  }
  delay(kUserInputDelayMs);
  display->DisplayCursorHighlight(/*highlight_On = */ true);
}

void MoveCursor(bool increment) {
  // Alarm Set Page (kAlarmSetPage) increment/decrement operations do not come here
  // they are handled as a special case

  display->DisplayCursorHighlight(/*highlight_On = */ false);

  // find first and last Click button in the page
  int first_click_button_index = 0, last_click_button_index = display_pages_vec[current_page].size() - 1;
  while(display_pages_vec[current_page][first_click_button_index]->btn_type == kLabelOnlyNoClickButton)
    first_click_button_index++;
  while(display_pages_vec[current_page][last_click_button_index]->btn_type == kLabelOnlyNoClickButton)
    last_click_button_index--;
  PrintLn("first_click_button_index = ", first_click_button_index);
  PrintLn("last_click_button_index = ", last_click_button_index);
  PrintLn("increment = ", increment);
  // special case of WiFi Scan Networks Page:
  bool find_next_button = false;
  if((current_page == kWiFiScanNetworksPage) && (current_cursor == kWiFiScanNetworksPageList)) {
    // move cursor inside the WiFi Networks Selection List
    if(increment) {
      if(display->current_wifi_networks_scan_page_cursor < display->kWifiScanNetworksPageItems - 1) {
        display->current_wifi_networks_scan_page_cursor++;
        PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
      }
      else {
        display->current_wifi_networks_scan_page_cursor = -1;
        find_next_button = true;
        PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
      }
    }
    else {
      if(display->current_wifi_networks_scan_page_cursor > 0) {
        display->current_wifi_networks_scan_page_cursor--;
        PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
      }
      else {
        display->current_wifi_networks_scan_page_cursor = -1;
        find_next_button = true;
        PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
      }
    }
    // special case of kWiFiScanNetworksPage continued inside if(find_next_button)
  }
  else {
    find_next_button = true;
  }
  // find next button
  if(find_next_button) {
    if(increment) {
      if(current_cursor != display_pages_vec[current_page][last_click_button_index]->btn_cursor_id) {
        int new_button_index = DisplayPagesVecCurrentButtonIndex() + 1;
        while(display_pages_vec[current_page][new_button_index]->btn_type == kLabelOnlyNoClickButton)
          new_button_index++;
        current_cursor = display_pages_vec[current_page][new_button_index]->btn_cursor_id;
      }
      else {
        current_cursor = display_pages_vec[current_page][first_click_button_index]->btn_cursor_id;
        // special case of kWiFiScanNetworksPage continued from before if(find_next_button)
        if((current_page == kWiFiScanNetworksPage) && (current_cursor == kWiFiScanNetworksPageList)) {
          // came here from cancel button
          display->current_wifi_networks_scan_page_cursor = 0;
          PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
        }
      }
    }
    else {
      if(current_cursor != display_pages_vec[current_page][first_click_button_index]->btn_cursor_id) {
        int new_button_index = DisplayPagesVecCurrentButtonIndex() - 1;
        while(display_pages_vec[current_page][new_button_index]->btn_type == kLabelOnlyNoClickButton)
          new_button_index--;
        current_cursor = display_pages_vec[current_page][new_button_index]->btn_cursor_id;
        // special case of kWiFiScanNetworksPage continued from before if(find_next_button)
        if((current_page == kWiFiScanNetworksPage) && (current_cursor == kWiFiScanNetworksPageList)) {
          // came here from Rescan button
          display->current_wifi_networks_scan_page_cursor = display->kWifiScanNetworksPageItems - 1;
          PrintLn("display->current_wifi_networks_scan_page_cursor = ", display->current_wifi_networks_scan_page_cursor);
        }
      }
      else {
        current_cursor = display_pages_vec[current_page][last_click_button_index]->btn_cursor_id;
      }
    }
  }
  PrintLn("current_cursor = ", current_cursor);

  display->DisplayCursorHighlight(/*highlight_On = */ true);
  // wait a little
  delay(2*kUserInputDelayMs);
}

// populate all pages in display_pages_vec
void PopulateDisplayPages() {
  //// struct decleration in common.h
  //struct DisplayButton {
  //  const Cursor btn_cursor_id;
  //  const ButtonType btn_type;
  //  const std::string row_label;
  //  const bool fixed_location;
  //  int16_t btn_x;
  //  int16_t btn_y;
  //  uint16_t btn_w;
  //  uint16_t btn_h;
  //  std::string btn_value;
  //};

  DisplayButton* page_save_button = new DisplayButton{ /* Save Button */ kPageSaveButton, kClickButtonWithLabel, "", true, kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, kSaveStr };
  DisplayButton* page_cancel_button = new DisplayButton{ /* Cancel Button */ kPageCancelButton, kClickButtonWithLabel, "", true, kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, kCancelStr };

  // MAIN PAGE
  display_pages_vec[kMainPage] = std::vector<DisplayButton*> {
    new DisplayButton{ /* No Selection */ kCursorNoSelection, kClickButtonWithIcon, "", true, 0, 0, 0, 0, "" },
    new DisplayButton{ /* Settings Wheel */ kMainPageSettingsWheel, kClickButtonWithIcon, "", true, kSettingsGearX1, kSettingsGearY1, kSettingsGearWidth, kSettingsGearHeight, "" },
    new DisplayButton{ /* Alarms Row     */ kMainPageSetAlarm, kClickButtonWithIcon, "", true, 1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, "" },
  };

  // ALARM SET PAGE
  // this page is handled as a special case

  // SETTINGS PAGE
  display_pages_vec[kSettingsPage] = std::vector<DisplayButton*> {
    new DisplayButton{ kSettingsPageWiFi, kClickButtonWithLabel, "WiFi Settings:", false, 0,0,0,0, "WIFI" },
    new DisplayButton{ kSettingsPageLocationAndWeather, kClickButtonWithLabel, "Location Settings:", false, 0,0,0,0, "LOCATION" },
    new DisplayButton{ kSettingsPageAlarmLongPressTime, kClickButtonWithLabel, "Long Press / Alarm Snooze Hold Time:", false, 0,0,0,0, (std::to_string(alarm_clock->alarm_long_press_seconds_) + "sec") },
    new DisplayButton{ kSettingsPageScreensaver, kClickButtonWithLabel, "Set RGB LEDs &:", false, 0,0,0,0, "SCREENSAVER" },
    new DisplayButton{ kSettingsPageRotateScreen, kClickButtonWithLabel, "Rotate Screen:", false, 0,0,0,0, "ROTATE" },
    new DisplayButton{ kSettingsPageUpdate, kClickButtonWithLabel, "Firmware Update:", false, 0,0,0,0, "UPDATE" },
    page_cancel_button,
  };

  // WIFI SETTINGS PAGE
  display_pages_vec[kWiFiSettingsPage] = std::vector<DisplayButton*> {
    new DisplayButton{ kWiFiSettingsPageShowSsidRow, kLabelOnlyNoClickButton, "Saved WiFi:", false, 0,0,0,0, wifi_stuff->WiFiDetailsShortString() },
    //new DisplayButton{ kWiFiSettingsPageClearSsidAndPasswd, kClickButtonWithLabel, "Clear WiFi Details:", false, 0,0,0,0, "CLEAR" },
    new DisplayButton{ kWiFiSettingsPageScanNetworks, kClickButtonWithLabel, "Scan Networks:", false, 0,0,0,0, "SCAN WIFI" },
    new DisplayButton{ kWiFiSettingsPageChangePasswd, kClickButtonWithLabel, "Change Password:", false, 0,0,0,0, "WIFI PASSWD" },
    new DisplayButton{ kWiFiSettingsPageConnect, kClickButtonWithLabel, "", false, 0,0,0,0, "CONNECT WIFI" },
    new DisplayButton{ kWiFiSettingsPageDisconnect, kClickButtonWithLabel, "", false, 0,0,0,0, "DISCONNECT" },
    page_cancel_button,
  };

  // WIFI SCAN NETWORKS PAGE
  display_pages_vec[kWiFiScanNetworksPage] = std::vector<DisplayButton*> {
    new DisplayButton{ kWiFiScanNetworksPageList, kClickButtonWithIcon, "", true, 0, 0, 0, 0, "" },
    new DisplayButton{ kWiFiScanNetworksPageRescan, kClickButtonWithLabel, "", true, kRescanButtonX1, kRescanButtonY1, kRescanButtonW, kRescanButtonH, kRescanStr },
    new DisplayButton{ kWiFiScanNetworksPageNext, kClickButtonWithLabel, "", true, kNextButtonX1, kNextButtonY1, kNextButtonW, kNextButtonH, kNextStr },
    page_cancel_button,
  };

  // WIFI DETAILS SOFT AP PAGE
  display_pages_vec[kSoftApInputsPage] = std::vector<DisplayButton*> {
    page_save_button,
    page_cancel_button,
  };

  // LOCATION AND WEATHER SETTINGS PAGE
  display_pages_vec[kLocationAndWeatherSettingsPage] = std::vector<DisplayButton*> {
    new DisplayButton{ kLocationAndWeatherSettingsPageSetLocation, kClickButtonWithLabel, "City:", false, 0,0,0,0, (std::to_string(wifi_stuff->location_zip_code_) + " " + wifi_stuff->location_country_code_) },
    new DisplayButton{ kLocationAndWeatherSettingsPageUnits, kClickButtonWithLabel, "Set Units:", false, 0,0,0,0, (wifi_stuff->weather_units_metric_not_imperial_ ? kMetricUnitStr : kImperialUnitStr) },
    new DisplayButton{ kLocationAndWeatherSettingsPageFetch, kClickButtonWithLabel, "Fetch Weather:", false, 0,0,0,0, "FETCH" },
    new DisplayButton{ kLocationAndWeatherSettingsPageUpdateTime, kClickButtonWithLabel, "Time-Zone:", false, 0,0,0,0, "UPDATE TIME" },
    page_cancel_button,
  };

  // LOCATION INPUT DETAILS PAGE
  display_pages_vec[kLocationInputsPage] = std::vector<DisplayButton*> {
    page_save_button,
    page_cancel_button,
  };

  // SCREENSAVER SETTINGS PAGE
  display_pages_vec[kScreensaverSettingsPage] = std::vector<DisplayButton*> {
    new DisplayButton{ kScreensaverSettingsPageMotion, kClickButtonWithLabel, "Screensaver Motion:", false, 0,0,0,0, (display->screensaver_bounce_not_fly_horizontally_ ? kBounceScreensaverStr : kFlyOutScreensaverStr) },
    new DisplayButton{ kScreensaverSettingsPageSpeed, kClickButtonWithLabel, "Screensaver Speed:", false, 0,0,0,0, (cpu_speed_mhz == 80 ? kSlowStr : (cpu_speed_mhz == 160 ? kMediumStr : kFastStr)) },
    new DisplayButton{ kScreensaverSettingsPageRun, kClickButtonWithLabel, "Run Screensaver:", false, 0,0,0,0, "RUN" },
    new DisplayButton{ kScreensaverSettingsPageRgbLedStripMode, kClickButtonWithLabel, "RGB LEDs Mode:", false, 0,0,0,0, RgbLedSettingString() },
    new DisplayButton{ kScreensaverSettingsPageNightTmDimHr, kClickButtonWithLabel, ("Evening time is " + std::to_string(kEveningTimeMinutes / 60 - 12) + "PM to:"), false, 0,0,0,0, (std::to_string(nvs_preferences->RetrieveNightTimeDimHour()) + "PM") },
    new DisplayButton{ kScreensaverSettingsPageRgbLedBrightness, kClickButtonWithLabel, "RGB LEDs Brightness:", false, 0,0,0,0, (std::to_string(int(static_cast<float>(rgb_strip_led_brightness) / 255 * 100)) + "%") },
    page_cancel_button,
  };

}

int DisplayPagesVecCurrentButtonIndex() {
  for (int i = 0; i < display_pages_vec[current_page].size(); i++) {
    if(display_pages_vec[current_page][i]->btn_cursor_id == current_cursor)
      return i;
  }
  return -1;
}

int DisplayPagesVecButtonIndex(ScreenPage button_page, Cursor button_cursor) {
  for (int i = 0; i < display_pages_vec[button_page].size(); i++) {
    if(display_pages_vec[button_page][i]->btn_cursor_id == button_cursor)
      return i;
  }
  return -1;
}

/*  1: turn On Button & wait,
    2: turn On Button,
    3: turn Off Button,
    default: turn On Button, wait & turn Off button
*/
void LedButtonClickUiResponse(int response_type = 0) {
  switch (response_type) {
    case 1:   // turn On Button, wait
      display->DisplayCurrentPageButtonRow(/*is_on = */ true);
      delay(kUserInputDelayMs);
      break;
    case 2:   // turn On Button
      display->DisplayCurrentPageButtonRow(/*is_on = */ true);
      break;
    case 3:   // turn Off Button
      display->DisplayCurrentPageButtonRow(/*is_on = */ false);
      break;
    default:     // turn On Button, wait, turn Off button
      display->DisplayCurrentPageButtonRow(/*is_on = */ true);
      delay(kUserInputDelayMs);
      display->DisplayCurrentPageButtonRow(/*is_on = */ false);
  }
}

void LedButtonClickAction() {
  if(current_page == kAlarmSetPage)
    display->SetAlarmScreen(/* process_user_input */ true, /* inc_button_pressed */ false, /* dec_button_pressed */ false, /* push_button_pressed */ true);
  else {
    if(current_page == kMainPage) {                 // MAIN PAGE
      if(current_cursor == kMainPageSettingsWheel)
        SetPage(kSettingsPage);
      else if(current_cursor == kMainPageSetAlarm)
        SetPage(kAlarmSetPage);
    }
    else if(current_page == kSettingsPage) {        // SETTINGS PAGE
      if(current_cursor == kSettingsPageWiFi) {
        LedButtonClickUiResponse(1);
        SetPage(kWiFiSettingsPage);
      }
      else if(current_cursor == kSettingsPageLocationAndWeather) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kLocationAndWeatherSettingsPage);
      }
      else if(current_cursor == kSettingsPageAlarmLongPressTime) {
        // change seconds
        if(alarm_clock->alarm_long_press_seconds_ < 25)
          alarm_clock->alarm_long_press_seconds_ += 10;
        else
          alarm_clock->alarm_long_press_seconds_ = 5;
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = std::to_string(alarm_clock->alarm_long_press_seconds_) + "sec";
        nvs_preferences->SaveLongPressSeconds(alarm_clock->alarm_long_press_seconds_);
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kSettingsPageScreensaver) {
        LedButtonClickUiResponse(1);
        SetPage(kScreensaverSettingsPage);
      }
      else if(current_cursor == kSettingsPageRotateScreen) {
        // rotate screen 180 degrees
        display->RotateScreen();
        if(ts != NULL)
          ts->SetTouchscreenOrientation();
        SetPage(kSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      else if(current_cursor == kSettingsPageUpdate) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kFirmwareVersionCheck);
        WaitForExecutionOfSecondCoreTask();
        if(wifi_stuff->firmware_update_available_str_.size() > 0)
          display->DisplayFirmwareVersionAndDate();
        LedButtonClickUiResponse(3);
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        SetPage(kMainPage);
      }
    }
    else if(current_page == kWiFiSettingsPage) {          // WIFI SETTINGS PAGE
      if(current_cursor == kWiFiSettingsPageScanNetworks) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kScanNetworks);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kWiFiScanNetworksPage);
      }
      else if(current_cursor == kWiFiSettingsPageChangePasswd) {
        LedButtonClickUiResponse(2);
        if(ts != NULL) {
          // use touchscreen
          // user input string
          std::string label = "Pwd for " + wifi_stuff->wifi_ssid_.substr(0,16) + ":";
          char returnText[kWifiSsidPasswordLengthMax + 1] = "";
          // get user input from screen
          bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ false);
          Serial.print("User Input :"); Serial.println(returnText);
          LedButtonClickUiResponse(2);
          if(ret) {
            wifi_stuff->wifi_password_ = returnText;
            wifi_stuff->SaveWiFiDetails();
            int index_of_ssid_button = DisplayPagesVecButtonIndex(kWiFiSettingsPage, kWiFiSettingsPageShowSsidRow);
            display_pages_vec[kWiFiSettingsPage][index_of_ssid_button]->btn_value = wifi_stuff->WiFiDetailsShortString();
          }
          SetPage(kWiFiSettingsPage);
        }
        else {
          // start a SoftAP and take user input
          AddSecondCoreTaskIfNotThere(kStartSetWiFiSoftAP);
          WaitForExecutionOfSecondCoreTask();
          SetPage(kSoftApInputsPage);
        }
      }
      else if(current_cursor == kWiFiSettingsPageClearSsidAndPasswd) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kDisconnectWiFi);
        WaitForExecutionOfSecondCoreTask();
        wifi_stuff->wifi_ssid_ = "Enter SSID";
        wifi_stuff->wifi_password_ = "Enter Passwd";
        wifi_stuff->SaveWiFiDetails();
        int display_pages_vec_wifi_ssid_passwd_button_index = DisplayPagesVecButtonIndex(kWiFiSettingsPage, kWiFiSettingsPageChangePasswd);
        display_pages_vec[kWiFiSettingsPage][display_pages_vec_wifi_ssid_passwd_button_index]->btn_value = wifi_stuff->WiFiDetailsShortString();
        SetPage(kWiFiSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      else if(current_cursor == kWiFiSettingsPageConnect) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kConnectWiFi);
        WaitForExecutionOfSecondCoreTask();
        LedButtonClickUiResponse(3);
        display->DisplayWiFiConnectionStatus();
      }
      else if(current_cursor == kWiFiSettingsPageDisconnect) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kDisconnectWiFi);
        WaitForExecutionOfSecondCoreTask();
        LedButtonClickUiResponse(3);
        display->DisplayWiFiConnectionStatus();
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        SetPage(kSettingsPage);
      }
    }
    else if(current_page == kWiFiScanNetworksPage) {          // WIFI NETWORKS SCAN PAGE
      if(current_cursor == kWiFiScanNetworksPageList) {
        int index_of_selected_ssid = display->current_wifi_networks_scan_page_cursor + display->current_wifi_networks_scan_page_no * display->kWifiScanNetworksPageItems;
        PrintLn("index_of_selected_ssid = ", index_of_selected_ssid);
        if(index_of_selected_ssid > wifi_stuff->WiFiScanNetworksCount() - 1)
          return;
        LedButtonClickUiResponse(2);
        std::string wifi_ssid = wifi_stuff->WiFiScanNetworkSsid(index_of_selected_ssid);
        // if Touchscreen, then ask for WiFi Password
        if(ts != NULL) {
          // use touchscreen
          // user input string
          std::string label = "Pwd for " + wifi_ssid.substr(0,16) + ":";
          char returnText[kWifiSsidPasswordLengthMax + 1] = "";
          // get user input from screen
          bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ false);
          Serial.print("User Input :"); Serial.println(returnText);
          LedButtonClickUiResponse(2);
          if(ret) {
            wifi_stuff->wifi_ssid_ = wifi_ssid;
            wifi_stuff->wifi_password_ = returnText;
            wifi_stuff->SaveWiFiDetails();
            int index_of_ssid_button = DisplayPagesVecButtonIndex(kWiFiSettingsPage, kWiFiSettingsPageShowSsidRow);
            display_pages_vec[kWiFiSettingsPage][index_of_ssid_button]->btn_value = wifi_stuff->WiFiDetailsShortString();
          }
        }
        else {
          wifi_stuff->wifi_ssid_ = wifi_ssid;
          wifi_stuff->SaveWiFiDetails();
          int index_of_ssid_button = DisplayPagesVecButtonIndex(kWiFiSettingsPage, kWiFiSettingsPageShowSsidRow);
          display_pages_vec[kWiFiSettingsPage][index_of_ssid_button]->btn_value = wifi_stuff->WiFiDetailsShortString();
        }
        wifi_stuff->WiFiScanNetworksFreeMemory();
        SetPage(kWiFiSettingsPage);
      }
      if(current_cursor == kWiFiScanNetworksPageRescan) {
        LedButtonClickUiResponse(2);
        AddSecondCoreTaskIfNotThere(kScanNetworks);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kWiFiScanNetworksPage);
      }
      else if(current_cursor == kWiFiScanNetworksPageNext) {
        LedButtonClickUiResponse();
        SetPage(kWiFiScanNetworksPage, false, true);
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        wifi_stuff->WiFiScanNetworksFreeMemory();
        SetPage(kWiFiSettingsPage);
      }
    }
    else if(current_page == kSoftApInputsPage) {          // SOFT AP SET WIFI SSID PASSWD PAGE
      if(current_cursor == kPageSaveButton) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
        WaitForExecutionOfSecondCoreTask();
        wifi_stuff->SaveWiFiDetails();
        // populate info of WiFi on page
        int index_of_ssid_button = DisplayPagesVecButtonIndex(kWiFiSettingsPage, kWiFiSettingsPageShowSsidRow);
        display_pages_vec[kWiFiSettingsPage][index_of_ssid_button]->btn_value = wifi_stuff->WiFiDetailsShortString();
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kStopSetWiFiSoftAP);
        WaitForExecutionOfSecondCoreTask();
      }
      SetPage(kWiFiSettingsPage);
    }
    else if(current_page == kLocationAndWeatherSettingsPage) {       // LOCATION AND WEATHER SETTINGS PAGE
      if(current_cursor == kLocationAndWeatherSettingsPageSetLocation) {
        LedButtonClickUiResponse(2);
        if(ts != NULL) {
          // use touchscreen
          // user input string
          std::string label = "Your City ZIP(5)/PIN(6) Code:";
          std::string location_zip_code = std::to_string(wifi_stuff->location_zip_code_);
          char returnText[kWifiSsidPasswordLengthMax + 1] = "";
          for(int i = 0; i< location_zip_code.size(); i++) {
            returnText[i] = location_zip_code[i];
          }
          // get user input from screen
          bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ true);
          Serial.print("User Input :"); Serial.println(returnText);
          LedButtonClickUiResponse(2);
          if(ret) {
            std::string new_location_zip_str = returnText;
            //Serial.print("new_location_zip_str :"); Serial.println(new_location_zip_str.c_str());
            uint32_t new_location_zip = std::stoi(new_location_zip_str);
            Serial.print("new_location_zip :"); Serial.println(new_location_zip);

            // get Country Code

            label = "Your Two Letter Country Code:";
            strcpy(returnText, "");
            strcpy(returnText, wifi_stuff->location_country_code_.c_str());
            // get user input from screen
            bool ret = display->GetUserOnScreenTextInput(label, returnText, /* bool number_input = */ false);
            Serial.print("User Input :"); Serial.println(returnText);
            LedButtonClickUiResponse(2);
            if(ret) {
              wifi_stuff->location_zip_code_ = new_location_zip;
              wifi_stuff->location_country_code_ = returnText;
              wifi_stuff->SaveWeatherLocationDetails();
              // update new location Zip/Pin code on button
              int display_pages_vec_location_and_weather_button_index = DisplayPagesVecButtonIndex(kLocationAndWeatherSettingsPage, kLocationAndWeatherSettingsPageSetLocation);
              std::string location_str = (std::to_string(wifi_stuff->location_zip_code_) + " " + wifi_stuff->location_country_code_);
              display_pages_vec[kLocationAndWeatherSettingsPage][display_pages_vec_location_and_weather_button_index]->btn_value = location_str;
              // get new location, update time and weather info
              AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
              WaitForExecutionOfSecondCoreTask();
            }
            SetPage(kLocationAndWeatherSettingsPage);
          }
          SetPage(kLocationAndWeatherSettingsPage);
        }
        else {
          AddSecondCoreTaskIfNotThere(kStartLocationInputsLocalServer);
          WaitForExecutionOfSecondCoreTask();
          SetPage(kLocationInputsPage);
        }
      }
      else if(current_cursor == kLocationAndWeatherSettingsPageUnits) {
        wifi_stuff->weather_units_metric_not_imperial_ = !wifi_stuff->weather_units_metric_not_imperial_;
        wifi_stuff->SaveWeatherUnits();
        wifi_stuff->got_weather_info_ = false;
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = (wifi_stuff->weather_units_metric_not_imperial_ ? kMetricUnitStr : kImperialUnitStr);
        LedButtonClickUiResponse(1);
        // fetch weather info in new units
        AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kLocationAndWeatherSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      else if(current_cursor == kLocationAndWeatherSettingsPageFetch) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kGetWeatherInfo);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kLocationAndWeatherSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      else if(current_cursor == kLocationAndWeatherSettingsPageUpdateTime) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
        WaitForExecutionOfSecondCoreTask();
        if(wifi_stuff->manual_time_update_successful_)
          SetPage(kMainPage);
        else
          SetPage(kLocationAndWeatherSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        current_cursor = kSettingsPageLocationAndWeather;
        SetPage(kSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
    }
    else if(current_page == kLocationInputsPage) {          // LOCATION INPUTS PAGE
      if(current_cursor == kPageSaveButton) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
        WaitForExecutionOfSecondCoreTask();
        wifi_stuff->SaveWeatherLocationDetails();
        wifi_stuff->got_weather_info_ = false;
        // update new location Zip/Pin code on button
        int display_pages_vec_location_and_weather_button_index = DisplayPagesVecButtonIndex(kLocationAndWeatherSettingsPage, kLocationAndWeatherSettingsPageSetLocation);
        std::string location_str = (std::to_string(wifi_stuff->location_zip_code_) + " " + wifi_stuff->location_country_code_);
        display_pages_vec[kLocationAndWeatherSettingsPage][display_pages_vec_location_and_weather_button_index]->btn_value = location_str;
        // get new location, update time and weather info
        AddSecondCoreTaskIfNotThere(kUpdateTimeFromNtpServer);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kLocationAndWeatherSettingsPage);
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        AddSecondCoreTaskIfNotThere(kStopLocationInputsLocalServer);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kLocationAndWeatherSettingsPage);
      }
    }
    else if(current_page == kScreensaverSettingsPage) {        // SCREENSAVER SETTINGS PAGE
      if(current_cursor == kScreensaverSettingsPageMotion) {
        display->screensaver_bounce_not_fly_horizontally_ = !display->screensaver_bounce_not_fly_horizontally_;
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = (display->screensaver_bounce_not_fly_horizontally_ ? kBounceScreensaverStr : kFlyOutScreensaverStr);
        nvs_preferences->SaveScreensaverBounceNotFlyHorizontally(display->screensaver_bounce_not_fly_horizontally_);
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kScreensaverSettingsPageSpeed) {
        CycleCpuFrequency();
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = (cpu_speed_mhz == 80 ? kSlowStr : (cpu_speed_mhz == 160 ? kMediumStr : kFastStr));
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kScreensaverSettingsPageRun) {
        LedButtonClickUiResponse(1);
        SetPage(kScreensaverPage);
      }
      else if(current_cursor == kScreensaverSettingsPageNightTmDimHr) {
        // change hours
        uint8_t night_time_dim_hour = nvs_preferences->RetrieveNightTimeDimHour();
        if(night_time_dim_hour < 11)
          night_time_dim_hour++;
        else
          night_time_dim_hour = 8;
        nvs_preferences->SaveNightTimeDimHour(night_time_dim_hour);
        night_time_minutes = night_time_dim_hour * 60 + 720;
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = (std::to_string(night_time_dim_hour) + "PM");
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kScreensaverSettingsPageRgbLedStripMode) {
        if(autorun_rgb_led_strip_mode < 3)
          autorun_rgb_led_strip_mode++;
        else
          autorun_rgb_led_strip_mode = 0;
        nvs_preferences->SaveAutorunRgbLedStripMode(autorun_rgb_led_strip_mode);
        RunRgbLedAccordingToSettings();
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = RgbLedSettingString();
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kScreensaverSettingsPageRgbLedBrightness) {
        // change brightness
        if(rgb_strip_led_brightness < 255)
          rgb_strip_led_brightness += 51;
        else
          rgb_strip_led_brightness = 51;
        display_pages_vec[current_page][DisplayPagesVecCurrentButtonIndex()]->btn_value = (std::to_string(int(static_cast<float>(rgb_strip_led_brightness) / 255 * 100)) + "%");
        nvs_preferences->SaveRgbStripLedBrightness(rgb_strip_led_brightness);
        InitializeRgbLed();
        RunRgbLedAccordingToSettings();
        LedButtonClickUiResponse();
      }
      else if(current_cursor == kPageCancelButton) {
        LedButtonClickUiResponse(1);
        current_cursor = kSettingsPageScreensaver;
        SetPage(kSettingsPage, /* bool move_cursor_to_first_button = */ false);
      }
      
    }
  }
}

