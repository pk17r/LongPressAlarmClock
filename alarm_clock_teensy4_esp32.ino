/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

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

// LOCAL PROGRAM VARIABLES

// counter to note when time is not being updated, thereafter reboot system
unsigned long last_seconds_update_millis = 0;

// setup core0
void setup() {

  // idle the other core
  // rp2040.idleOtherCore();

  #if defined(MCU_IS_ESP32)
    setCpuFrequencyMhz(160);
  #endif

  Serial.begin(9600);
  delay(100);
  // while(!Serial) { delay(20); };
  Serial.println(F("\nSerial OK"));

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

  // restart the other core
  // rp2040.restartCore1();
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

      // 5 mins before alarm time, try to get weather info
      #if defined(WIFI_IS_USED)
        if((second_core_task == kNoTask) && (wifi_stuff->got_weather_info_time_ms == 0 || millis() - wifi_stuff->got_weather_info_time_ms > 60*60*1000 || alarm_clock->MinutesToAlarm() == 5)) {
            // get updated weather info every 60 minutes and as well as 5 minutes before alarm time
            second_core_task = kGetWeatherInfo;
        }
      #endif

      // auto update time at 2AM every morning
      if(rtc->hour() == 2 && rtc->minute() == 0 && second_core_task == kNoTask) {
        Serial.println(F("Auto Update RTC HW Time from NTP Server"));
        // update time from NTP server
        second_core_task = kUpdateTimeFromNtpServer;
      }

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

    // second core control operations
    if(second_core_task == kTaskCompleted) {
      second_core_task = kNoTask;
    }
    // switch(second_core_control_flag) {
    //   case 1: // resume the other core
    //     // rp2040.restartCore1();
    //     // rp2040.resumeOtherCore();
    //     second_core_control_flag = 2;  // core started
    //     // Serial.println("Resumed core1");
    //     break;
    //   case 3: // core1 is done processing and can be idled
    //     // rp2040.idleOtherCore();
    //     second_core_control_flag = 0;  // core idled
    //     // Serial.println("Idled core1");
    //     break;
    // }

    // counter to note when time is not being updated, thereafter reboot system. Needs to be at end of seconds update control
    last_seconds_update_millis = millis();
  }

  // make screensaver motion fast
  if(current_page == kScreensaverPage)
    display->Screensaver();

  // accept user serial inputs
  if (Serial.available() != 0)
    ProcessSerialInput();

  // check if time is not being updated, then reset system
  if(millis() - last_seconds_update_millis > 3000) {
    Serial.println(); Serial.println("**** Time Stuck. Rebooting! ****"); Serial.println(); Serial.flush();
    rp2040.reboot();
  }

  // #if defined(MCU_IS_ESP32)
  //     // if button is inactive, then go to sleep
  //     if(!pushBtn->buttonActiveDebounced())
  //       putEsp32ToLightSleep();
  // #endif
}

// setup core1
void setup1() {
  delay(2000);
}

// arduino loop function on core1 - low priority one with wifi weather update task
void loop1() {

  // run the core only to do specific not time important operations
  if (second_core_task != kNoTask && second_core_task != kTaskCompleted) {

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
    // set the core up to be idled from core0
    // second_core_control_flag = 3;
    second_core_task = kTaskCompleted;
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

extern "C" char* sbrk(int incr);
// Serial.print(F("- SRAM left: ")); Serial.println(freeRam());
int AvailableRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
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
  Serial.print(F("(s"));
  if(rtc->second() < 10) Serial.print(kCharZero);
  Serial.print(rtc->second());
  Serial.print(F(":i"));
  if(inactivity_seconds < 100) Serial.print(kCharZero);
  if(inactivity_seconds < 10) Serial.print(kCharZero);
  Serial.print(inactivity_seconds);
  Serial.print(F(": SRAM left: ")); Serial.print(AvailableRam());
  Serial.print(F(") - "));
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
