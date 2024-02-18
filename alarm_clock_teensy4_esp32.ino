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
#if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include "wifi_stuff.h"
#endif
#include "rtc.h"
#include "alarm_clock.h"
#include "rgb_display.h"
#if defined(TOUCHSCREEN_IS_XPT2046)
  #include "touchscreen.h"
#endif

// modules - hardware or software
PushButtonTaps* push_button = NULL;   // Push Button object
EEPROM* eeprom = NULL;    // ptr to External EEPROM HW class object
WiFiStuff* wifi_stuff = NULL;  // ptr to wifi stuff class object that contains WiFi and Weather Fetch functions
RTC* rtc = NULL;  // ptr to class object containing RTC HW
AlarmClock* alarm_clock = NULL;  // ptr to alarm clock class object that controls Alarm functions
RGBDisplay* display = NULL;   // ptr to display class object that manages the display
#if defined(TOUCHSCREEN_IS_XPT2046)
  Touchscreen* ts = NULL;         // Touchscreen class object
#endif

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
  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
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

void loop() {
  // put your main code here, to run repeatedly:
  alarm_clock->UpdateTimePriorityLoop();
}

void setup1() {
  delay(2000);
}

void loop1() {
  // put your main code here, to run repeatedly:
  alarm_clock->NonPriorityTasksLoop();
}


// GLOBAL VARIABLES AND FUNCTIONS

// counter to note user inactivity seconds
uint8_t inactivity_seconds = 0;

// Display Visible Data Structure variables
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// seconds flag triggered by interrupt
// static volatile bool rtcHwSecUpdate = false;

// secondCoreControlFlag controls idling and restarting core1 from core0
//    0 = core is idling
//    1 = resume the other core from core0
//    2 = core is running some operation
//    3 = core is done processing and can be idled
volatile byte second_core_control_flag = 0;

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
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        PrepareTimeDayDateArrays();
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
        // refresh time
        rtc->Refresh();
        // prepare date and time arrays
        PrepareTimeDayDateArrays();
        // set main page back
        SetPage(kMainPage);
        inactivity_seconds = 0;
      }
      break;
    case 'w':   // get today's weather info
      {
        Serial.println(F("**** Get Weather Info ****"));
        // get today's weather info
        second_core_control_flag = 1;
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
