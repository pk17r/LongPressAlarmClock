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
  if(rtc->second() < 10) Serial.print('0');
  Serial.print(rtc->second());
  Serial.print(F(":i"));
  if(alarm_clock->inactivity_seconds_ < 100) Serial.print('0');
  if(alarm_clock->inactivity_seconds_ < 10) Serial.print('0');
  Serial.print(alarm_clock->inactivity_seconds_);
  Serial.print(F(": SRAM left: ")); Serial.print(AvailableRam());
  Serial.print(F(") - "));
  Serial.flush();
}
