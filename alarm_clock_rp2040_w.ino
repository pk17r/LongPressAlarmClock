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

// modules - hardware or software
PushButtonTaps* push_button = NULL;   // Push Button object
EEPROM* eeprom = NULL;    // ptr to External EEPROM HW class object
WiFiStuff* wifi_stuff = NULL;  // ptr to wifi stuff class object that contains WiFi and Weather Fetch functions
RTC* rtc = NULL;  // ptr to class object containing RTC HW
AlarmClock* alarm_clock = NULL;  // ptr to alarm clock class object that controls Alarm functions
RGBDisplay* display = NULL;   // ptr to display class object that manages the display
Touchscreen* ts = NULL;         // Touchscreen class object


// setup core0
void setup() {

  // watchdog to reboot system if it gets stuck for whatever reason for over 8.3 seconds
  // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#void-rp2040-wdt-begin-uint32-t-delay-ms
  rp2040.wdt_begin(8300);

  Serial.begin(9600);
  delay(100);
  // while(!Serial) { delay(20); };
  PrintLn("Serial OK");
  // Serial.println();

  // make all SPI CS pins high
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TS_CS_PIN, OUTPUT);
  digitalWrite(TS_CS_PIN, HIGH);

  // LED Pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // initialize push button
  push_button = new PushButtonTaps(BUTTON_PIN);

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
  ts = new Touchscreen();

}

// arduino loop function on core0 - High Priority one with time update tasks
void loop() {
  // check if button pressed or touchscreen touched
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

  // if user presses button, show instant response by turning On LED
  if(push_button->buttonActiveDebounced())
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // new second! Update Time!
  if (rtc->rtc_hw_sec_update_) {
    rtc->rtc_hw_sec_update_ = false;

    SerialTimeStampPrefix();

    // if time is lost because of power failure
    if(rtc->year() < 2024 && second_core_task == kNoTask) {
      Serial.println(F("**** Update RTC HW Time from NTP Server ****"));
      // update time from NTP server
      second_core_task = kUpdateTimeFromNtpServer;
    }

    // new minute!
    if (rtc->rtc_hw_min_update_) {
      rtc->rtc_hw_min_update_ = false;
      Serial.println("New Minute!");

      // Activate Buzzer if Alarm Time has arrived
      if(rtc->year() >= 2024 && alarm_clock->MinutesToAlarm() == 0) {
        // go to buzz alarm function and show alarm triggered screen!
        alarm_clock->BuzzAlarmFn();
        // returned from Alarm Triggered Screen and Good Morning Screen
        // set main page
        SetPage(kMainPage);
        inactivity_seconds = 0;
      }

      // if screensaver is On, then update time on it
      if(current_page == kScreensaverPage)
        display->refresh_screensaver_canvas_ = true;

      #if defined(WIFI_IS_USED)
        // try to get weather info 5 mins before alarm time and every 60 minutes
        if((second_core_task == kNoTask) && (wifi_stuff->got_weather_info_time_ms == 0 || millis() - wifi_stuff->got_weather_info_time_ms > 60*60*1000 || alarm_clock->MinutesToAlarm() == 5)) {
            // get updated weather info every 60 minutes and as well as 5 minutes before alarm time
            second_core_task = kGetWeatherInfo;
            Serial.println("Get Weather Info!");
        }

        // auto update time at 2AM every morning
        if(second_core_task == kNoTask && rtc->hourModeAndAmPm() == 1 && rtc->hour() == 2 && rtc->minute() == 0) {
          Serial.println(F("Auto Update RTC HW Time from NTP Server"));
          // update time from NTP server
          second_core_task = kUpdateTimeFromNtpServer;
          Serial.println("Get Time Update!");
        }
      #endif

    }

    // prepare date and time arrays
    PrepareTimeDayDateArrays();

    // update time on main page
    if(current_page == kMainPage)
      display->DisplayTimeUpdate();

    // serial print RTC Date Time
    SerialPrintRtcDateTime();
    Serial.println();

    // check for inactivity
    if(inactivity_seconds < kInactivitySecondsLimit) {
      inactivity_seconds++;
      if(inactivity_seconds == kInactivitySecondsLimit) {
        // set display brightness based on time
        display->CheckTimeAndSetBrightness();
        // turn screen saver On
        if(current_page != kScreensaverPage)
          SetPage(kScreensaverPage);
      }
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

}

// setup core1
void setup1() {
  delay(2000);
}

// arduino loop function on core1 - low priority one with wifi weather update task
void loop1() {
  // run the core only to do specific not time important operations
  if (second_core_task != kNoTask) {

    if(second_core_task == kGetWeatherInfo) {
      // get today's weather info
      wifi_stuff->GetTodaysWeatherInfo();

      // try once more if did not get info
      if(!wifi_stuff->got_weather_info_)
        wifi_stuff->GetTodaysWeatherInfo();
    }
    else if(second_core_task == kUpdateTimeFromNtpServer) {
      // get time from NTP server
      if(!(wifi_stuff->GetTimeFromNtpServer())) {
        delay(1000);
        // try once more if did not get info
        wifi_stuff->GetTimeFromNtpServer();
      }
    }
    else if(second_core_task == kConnectWiFi) {
      wifi_stuff->TurnWiFiOn();
    }
    else if(second_core_task == kDisconnectWiFi) {
      wifi_stuff->TurnWiFiOff();
    }

    // done processing the task
    second_core_task = kNoTask;
  }

  // a delay to slow things down and not crash
  delay(1000);
}


// GLOBAL VARIABLES AND FUNCTIONS

// counter to note user inactivity seconds
uint8_t inactivity_seconds = 0;

// Display Visible Data Structure variables
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// second core current task
volatile SecondCoreTask second_core_task = kNoTask;

int AvailableRam() {
  // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#int-rp2040-getfreeheap
  return rp2040.getFreeHeap();
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
  Serial.print('(');
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
  Serial.print(" :i");
  if(inactivity_seconds < 100) Serial.print(kCharZero);
  if(inactivity_seconds < 10) Serial.print(kCharZero);
  Serial.print(inactivity_seconds);
  Serial.print(": RAM "); Serial.print(AvailableRam());
  Serial.print(')');
  Serial.print(kCharSpace);
  Serial.flush();
}

void PrintLn(const char* charText) {
  SerialTimeStampPrefix();
  Serial.println(charText);
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
  Serial.print(new_display_data_.alarm_str);
  Serial.flush();
}

// reset watchdog within time so it does not reboot system
void ResetWatchdog() {
  // https://arduino-pico.readthedocs.io/en/latest/rp2040.html#hardware-watchdog
  rp2040.wdt_reset();
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
        inactivity_seconds = 0;
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
        inactivity_seconds = 0;
      }
      break;
    case 'w':   // get today's weather info
      {
        Serial.println(F("**** Get Weather Info ****"));
        // get today's weather info
        second_core_task = kGetWeatherInfo;
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
    case 'n':   // get time from NTP server and set on RTC HW
      {
        Serial.println(F("**** Update RTC HW Time from NTP Server ****"));
        // update time from NTP server
        second_core_task = kUpdateTimeFromNtpServer;
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
        second_core_task = kConnectWiFi;
      }
      break;
    case 'd' :  // disconnect WiFi
      {
        Serial.println(F("**** Disconnect WiFi ****"));
        second_core_task = kDisconnectWiFi;
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
      display->SettingsPage();
      display->SetMaxBrightness();
      break;
    default:
      Serial.print("Unprogrammed Page "); Serial.print(page); Serial.println('!');
  }
}
