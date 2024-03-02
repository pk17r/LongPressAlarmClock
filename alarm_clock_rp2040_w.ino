/**************************************************************************

  # Touchscreen Long Press Alarm Clock


- Hardware:
  - MCU: Raspberry Pi Pico W
  - Display: 2.8" ST7789V touchscreen display, other selectable options: ST7735, ILI9341 and ILI9488
  - DS3231 RTC Clock
  - A push button with LED
  - A quite powerful 85dB passive buzzer for alarm


- Software:
  - A fast low RAM usage FastDrawBitmap function is implement that converts monochrome image into RGB565 with 2 colors and sends image to display via SPI row by row
  - Adafruit Library used for GFX functions
  - uRTCLib Library for DS3231 updated with AM/PM mode and class size reduced by 3 bytes while adding additional functionality


- Salient Features
  - Program requires user to press and hold a button for 25 seconds continously to turn off alarm and buzzer
  - C++ OOP Based Project
  - All modules have their own independent definition headers
  - A common header containing pointers to objects of every module and gloal functions
  - Time update via NTP server using WiFi once every day to keep accuracy
  - DS3231 RTC itself is high accuracy clock having deviation of +/-2 minutes per year
  - Get Weather info using WiFi and display today's weather after alarm
  - Get user input of WiFi details via an on-screen keyboard
  - Colorful smooth Screensaver with a big clock
  - Touchscreen based alarm set page
  - Settings saved in EEPROM so not lost on power loss
  - RP2040 watchdog keeps check on program not getting stuck, reboots if stuck
  - Screen brightness changes according to time of the day, with lowest brightness setting at night time
  - Time critical tasks happen on core0 - time update, screensaver fast motion, alarm time trigger
  - Non Time critical tasks happen on core1 - update weather info using WiFi, update time using NTP server, connect/disconnect WiFi


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

#if defined(MCU_IS_ESP32_WROOM_DA_MODULE)
  TaskHandle_t Task1;
#endif

SPIClass* spi_obj = NULL;

// watchdog timeout time
#if defined(MCU_IS_RP2040)
const unsigned long kWatchdogTimeoutMs = 8300;
#elif defined(MCU_IS_ESP32)
const unsigned long kWatchdogTimeoutMs = 10000;
#endif

// setup core0
void setup() {

  #if defined(MCU_IS_RP2040)
    // watchdog to reboot system if it gets stuck for whatever reason for over 8.3 seconds
    // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#void-rp2040-wdt-begin-uint32-t-delay-ms
    rp2040.wdt_begin(kWatchdogTimeoutMs);
  #elif defined(MCU_IS_ESP32)
    // slow the ESP32 CPU to reduce power consumption
    setCpuFrequencyMhz(80);
    // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-reference/system/wdts.html
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
    esp_task_wdt_init(kWatchdogTimeoutMs / 1000, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch
  #endif

  Serial.begin(115200);
  delay(200);
  // while(!Serial) { delay(20); };
  Serial.println(F("\nSerial OK"));
  Serial.flush();

  // make all spi CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  #if defined(TOUCHSCREEN_IS_XPT2046)
    pinMode(TS_CS_PIN, OUTPUT);
    digitalWrite(TS_CS_PIN, HIGH);
  #endif

  // initialize spi
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

  // LED Pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // BUILTIN LED - we use for WiFi Connect Notification
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // initialize push button
  push_button = new PushButtonTaps(BUTTON_PIN);
  inc_button = new PushButtonTaps(INC_BUTTON_PIN);
  dec_button = new PushButtonTaps(DEC_BUTTON_PIN);

  // initialize modules
  eeprom = new EEPROM();
  #if defined(WIFI_IS_USED)
    wifi_stuff = new WiFiStuff();
  #endif
  rtc = new RTC();
  alarm_clock = new AlarmClock();
  // setup alarm clock
  alarm_clock->Setup();
  // prepare date and time arrays and serial print RTC Date Time
  PrepareTimeDayDateArrays();
  // serial print RTC Date Time
  SerialPrintRtcDateTime();
  display = new RGBDisplay();
  // setup display
  display->Setup();
  #if defined(TOUCHSCREEN_IS_XPT2046)
    ts = new Touchscreen();
  #endif

  // if time is lost because of power failure
  if(rtc->year() < 2024) {
    PrintLn("**** Update RTC HW Time from NTP Server ****");
    // update time from NTP server
    second_core_tasks_queue.push(kUpdateTimeFromNtpServer);
  }

  #if defined(MCU_IS_ESP32_WROOM_DA_MODULE)
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

// arduino loop function on core0 - High Priority one with time update tasks
void loop() {
  // check if button pressed or touchscreen touched
  if((inactivity_millis >= kUserInputDelayMs) && (push_button->buttonActiveDebounced() || inc_button->buttonActiveDebounced() || dec_button->buttonActiveDebounced() || (ts != NULL && ts->IsTouched()))) {
    // show instant response by turing up brightness
    display->SetMaxBrightness();

    inactivity_millis = 0;

    // instant page change action
    if(current_page == kScreensaverPage)
    { // turn off screensaver if on
      SetPage(kMainPage);
    }
    else if(current_page == kAlarmSetPage)
    { // if on alarm page, then take alarm set page user inputs
      display->SetAlarmScreen(true);
    }
    else if(ts != NULL && current_page != kAlarmSetPage && inactivity_millis >= kUserInputDelayMs)
    { // if not on alarm page and user clicked somewhere, get touch input
      ScreenPage userTouchRegion = display->ClassifyUserScreenTouchInput();
      if(userTouchRegion != kNoPageSelected)
        SetPage(userTouchRegion);
    }

    // button click action
    if(push_button->buttonActiveDebounced()) {
      PrintLn("push_button");
      if(highlight == kMainPageSettingsWheel)
        SetPage(kSettingsPage);
      else if(highlight == kMainPageSetAlarm)
        SetPage(kAlarmSetPage);
      else if(highlight == kSettingsPageWiFi)
        SetPage(kWiFiSettingsPage);
      else if(highlight == kSettingsPageLocation)
        SetPage(kWeatherSettingsPage);
      else if(highlight == kSettingsPageScreensaver)
        SetPage(kScreensaverPage);
      else if(highlight == kSettingsPageCancel)
        SetPage(kMainPage);
      else if(highlight == kWiFiSettingsPageConnect) {
        display->InstantHighlightResponse(kWiFiSettingsPageConnect);
        second_core_tasks_queue.push(kConnectWiFi);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kWiFiSettingsPage);
      }
      else if(highlight == kWiFiSettingsPageDisconnect) {
        display->InstantHighlightResponse(kWiFiSettingsPageDisconnect);
        second_core_tasks_queue.push(kDisconnectWiFi);
        WaitForExecutionOfSecondCoreTask();
        SetPage(kWiFiSettingsPage);
      }
      else if(highlight == kWiFiSettingsPageCancel)
        SetPage(kSettingsPage);

      if(ts == NULL)
        display->InstantHighlightResponse(kCursorNoSelection);
    }
    else if(inc_button->buttonActiveDebounced()) {
      MoveCursor(true);
      PrintLn("inc_button");
    }
    else if(dec_button->buttonActiveDebounced()) {
      MoveCursor(false);
      PrintLn("dec_button");
    }
  }

  // if user presses button, show instant response by turning On LED
  if(push_button->buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // new second! Update Time!
  if (rtc->rtc_hw_sec_update_) {
    rtc->rtc_hw_sec_update_ = false;

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
        // try to get weather info 5 mins before alarm time and every 60 minutes
        if((wifi_stuff->got_weather_info_time_ms == 0 || millis() - wifi_stuff->got_weather_info_time_ms > 60*60*1000 || alarm_clock->MinutesToAlarm() == 10)) {
          // get updated weather info every 60 minutes and as well as 5 minutes before alarm time
          second_core_tasks_queue.push(kGetWeatherInfo);
          PrintLn("Get Weather Info!");
        }

        // auto update time at 2AM every morning
        if(rtc->hourModeAndAmPm() == 1 && rtc->hour() == 2 && rtc->minute() == 0) {
          // update time from NTP server
          second_core_tasks_queue.push(kUpdateTimeFromNtpServer);
          PrintLn("Get Time Update from NTP Server");
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
    if(inactivity_millis > kInactivityMillisLimit) {
      // set display brightness based on time
      display->CheckTimeAndSetBrightness();
      // turn screen saver On
      if(current_page != kScreensaverPage)
        SetPage(kScreensaverPage);
    }

    // watchdog to reboot system if it gets stuck for whatever reason
    ResetWatchdog();
  }

  // make screensaver motion fast
  if(current_page == kScreensaverPage)
    display->Screensaver();

  // accept user serial inputs
  if (Serial.available() != 0)
    ProcessSerialInput();

  #if defined(MCU_IS_ESP32_S2_MINI)
    // ESP32_S2_MINI is single core MCU
    loop1();
  #endif
}

#if defined(MCU_IS_RP2040)
// setup core1
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

    if(current_task == kGetWeatherInfo && (wifi_stuff->got_weather_info_time_ms == 0 || millis() - wifi_stuff->got_weather_info_time_ms > 10*1000)) {
      // get today's weather info
      wifi_stuff->GetTodaysWeatherInfo();

      // try once more if did not get info
      if(!wifi_stuff->got_weather_info_)
        wifi_stuff->GetTodaysWeatherInfo();
    }
    else if(current_task == kUpdateTimeFromNtpServer && (wifi_stuff->last_ntp_server_time_update_time_ms == 0 || millis() - wifi_stuff->last_ntp_server_time_update_time_ms > 10*1000)) {
      // get time from NTP server
      if(!(wifi_stuff->GetTimeFromNtpServer())) {
        delay(1000);
        // try once more if did not get info
        wifi_stuff->GetTimeFromNtpServer();
      }
    }
    else if(current_task == kConnectWiFi) {
      wifi_stuff->TurnWiFiOn();
    }
    else if(current_task == kDisconnectWiFi) {
      wifi_stuff->TurnWiFiOff();
    }

    // done processing the task
    second_core_tasks_queue.pop();
  }
  // turn off WiFi if there are no more requests and User is not using device
  if(wifi_stuff->wifi_connected_ && inactivity_millis >= kInactivityMillisLimit)
    wifi_stuff->TurnWiFiOff();
}

#if defined(MCU_IS_ESP32_WROOM_DA_MODULE)
void Task1code( void * parameter) {
  for(;;) 
    loop1();
}
#endif

void WaitForExecutionOfSecondCoreTask() {
  #if defined(MCU_IS_ESP32_S2_MINI)
    // ESP32_S2_MINI is single core MCU
    loop1();
  #elif defined(MCU_IS_RP2040) || defined(MCU_IS_ESP32)
    unsigned long time_start = millis();
    while (!second_core_tasks_queue.empty() && millis() - time_start <  kWatchdogTimeoutMs - 3000) {
      delay(10);
    }
  #endif
}

// GLOBAL VARIABLES AND FUNCTIONS

// counter to note user inactivity seconds
elapsedMillis inactivity_millis = 0;

// Display Visible Data Structure variables
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// current cursor highlight location on page
Cursor highlight = kCursorNoSelection;

// second core current task
// volatile SecondCoreTask second_core_task = kNoTask;
std::queue<SecondCoreTask> second_core_tasks_queue;

int AvailableRam() {
  #if defined(MCU_IS_RP2040)
    // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#int-rp2040-getfreeheap
    return rp2040.getFreeHeap();
  #elif defined(MCU_IS_ESP32)
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html
    return esp_get_free_heap_size();
  #endif
}

void SerialInputFlush() {
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

// reset watchdog within time so it does not reboot system
void ResetWatchdog() {
  #if defined(MCU_IS_RP2040)
    // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#hardware-watchdog
    rp2040.wdt_reset();
  #elif defined(MCU_IS_ESP32)
    // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-reference/system/wdts.html
    esp_task_wdt_reset();
  #endif
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
        alarm_clock->BuzzAlarmFn();
        // set main page back
        SetPage(kMainPage);
        inactivity_millis = 0;
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
        // set main page back
        SetPage(kMainPage);
        inactivity_millis = 0;
      }
      break;
    case 'w':   // get today's weather info
      {
        Serial.println(F("**** Get Weather Info ****"));
        // get today's weather info
        second_core_tasks_queue.push(kGetWeatherInfo);
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
        wifi_stuff->wifi_ssid_ = "";
        for (int i = 0; i < min(inputStr.length(), kWifiSsidPasswordLengthMax); i++)
          if(inputStr[i] != '\0' || inputStr[i] != '\n')
            wifi_stuff->wifi_ssid_ = wifi_stuff->wifi_ssid_ + inputStr[i];
        Serial.println(wifi_stuff->wifi_ssid_.c_str());
        Serial.print("PASSWORD: ");
        while(Serial.available() == 0) {
          delay(20);
        }
        delay(20);
        inputStr = Serial.readString();
        wifi_stuff->wifi_password_ = "";
        for (int i = 0; i < min(inputStr.length(), kWifiSsidPasswordLengthMax); i++)
          if(inputStr[i] != '\0' || inputStr[i] != '\n')
            wifi_stuff->wifi_password_ = wifi_stuff->wifi_password_ + inputStr[i];
        Serial.println(wifi_stuff->wifi_password_.c_str());
        wifi_stuff->SaveWiFiDetails();
      }
      break;
    case 'n':   // get time from NTP server and set on RTC HW
      {
        Serial.println(F("**** Update RTC HW Time from NTP Server ****"));
        // update time from NTP server
        second_core_tasks_queue.push(kUpdateTimeFromNtpServer);
      }
      break;
    case 'o' :  // On Screen User Text Input
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
    case 'c' :  // connect to WiFi
      {
        Serial.println(F("**** Connect to WiFi ****"));
        second_core_tasks_queue.push(kConnectWiFi);
      }
      break;
    case 'd' :  // disconnect WiFi
      {
        Serial.println(F("**** Disconnect WiFi ****"));
        second_core_tasks_queue.push(kDisconnectWiFi);
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
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
    case kAlarmSetPage:
      current_page = kAlarmSetPage;     // new page needs to be set before any action
      if(ts == NULL)
        highlight = kAlarmSetPageHour;
      // set variables for alarm set screen
      alarm_clock->var_1_ = alarm_clock->alarm_hr_;
      alarm_clock->var_2_ = alarm_clock->alarm_min_;
      alarm_clock->var_3_AM_PM_ = alarm_clock->alarm_is_AM_;
      alarm_clock->var_4_ON_OFF_ = alarm_clock->alarm_ON_;
      display->SetAlarmScreen(false);
      break;
    case kAlarmTriggeredPage:
      current_page = kAlarmTriggeredPage;     // new page needs to be set before any action
      display->AlarmTriggeredScreen(true, alarm_clock->kAlarmEndButtonPressAndHoldSeconds);
      display->SetMaxBrightness();
      break;
    case kSettingsPage:
      current_page = kSettingsPage;     // new page needs to be set before any action
      if(ts == NULL)
        highlight = kSettingsPageWiFi;
      display->SettingsPage();
      display->SetMaxBrightness();
      break;
    case kWiFiSettingsPage:
      current_page = kWiFiSettingsPage;     // new page needs to be set before any action
      display->WiFiSettingsPage();
      display->SetMaxBrightness();
      break;
    case kEnterWiFiSsidPage:
      current_page = kWiFiSettingsPage;     // new page needs to be set before any action
      display->SetMaxBrightness();
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
      display->SetMaxBrightness();
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
      display->WeatherSettingsPage();
      display->SetMaxBrightness();
      break;
    case kEnterWeatherLocationZipPage:
      current_page = kWeatherSettingsPage;     // new page needs to be set before any action
      display->SetMaxBrightness();
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
      display->SetMaxBrightness();
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
}

void MoveCursor(bool increment) {
  Serial.print("MoveCursor top highlight "); Serial.println(highlight);
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
      else if(highlight == kAlarmSetPageX)
        highlight = kCursorNoSelection;
      else
        highlight++;
    }
    else {
      if(highlight == kCursorNoSelection)
        highlight = kAlarmSetPageX;
      else if(highlight == kAlarmSetPageHour)
        highlight = kCursorNoSelection;
      else
        highlight--;
    }
  }
  else if(current_page == kWiFiSettingsPage) {
    if(increment) {
      if(highlight == kWiFiSettingsPageCancel)
        highlight = kWiFiSettingsPageConnect;
      else
        highlight++;
    }
    else {
      if(highlight == kWiFiSettingsPageConnect)
        highlight = kWiFiSettingsPageCancel;
      else
        highlight--;
    }
  }
  Serial.print("MoveCursor bottom highlight "); Serial.println(highlight);
  display->InstantHighlightResponse(kCursorNoSelection);
}