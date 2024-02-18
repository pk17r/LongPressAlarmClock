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

// Display Visible Data Struct
DisplayData new_display_data_ { "", "", "", "", true, false, true }, displayed_data_ { "", "", "", "", true, false, true };

// current page on display
ScreenPage current_page = kMainPage;

// seconds flag triggered by interrupt
// static volatile bool rtcHwSecUpdate = false;

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
  if(alarm_clock->inactivity_seconds_ < 100) Serial.print(kCharZero);
  if(alarm_clock->inactivity_seconds_ < 10) Serial.print(kCharZero);
  Serial.print(alarm_clock->inactivity_seconds_);
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
