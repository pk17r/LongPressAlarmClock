/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/
#include "common.h"
#include "rtc.h"
#include "rgb_display.h"
#include "alarm_clock.h"
#if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include "wifi_stuff.h"
#endif
#include "eeprom.h"
#include <PushButtonTaps.h>
#if defined(TOUCHSCREEN_IS_XPT2046)
  #include "touchscreen.h"
#endif

// modules - hardware or software
RTC* rtc = NULL;  // ptr to class object containing RTC HW ** Initialization Order 1
RGBDisplay* display = NULL;   // ptr to display class object that manages the display ** Initialization Order 2
AlarmClock* alarmClock = NULL;  // ptr to alarm clock class object that controls Alarm functions ** Initialization Order 3
EEPROM* eeprom = NULL;    // ptr to External EEPROM HW class object ** Initialization Order 4
WiFiStuff* wifiStuff = NULL;  // ptr to wifi stuff class object that contains WiFi and Weather Fetch functions ** Initialization Order 5
PushButtonTaps pushBtn;   // Push Button object ** Initialization Order 6
#if defined(TOUCHSCREEN_IS_XPT2046)
  Touchscreen ts;         // Touchscreen class object ** Initialization Order 7
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

  // initialize modules
  rtc = new RTC();
  alarmClock = new AlarmClock();
  display = new RGBDisplay();
  eeprom = new EEPROM();
  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
    wifiStuff = new WiFiStuff();
  #endif

  // initialize push button
  pushBtn.setButtonPin(BUTTON_PIN);
  // pushBtn.setButtonActiveLow(false);

  #if defined(TOUCHSCREEN_IS_XPT2046)
    // touchscreen setup and calibration
    ts.setupAndCalibrate(220, 3800, 280, 3830, 320, 240);
  #endif

  // retrieve wifi details
  #if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
    wifiStuff->retrieveWiFiDetails();
    wifiStuff->turn_WiFi_Off();
  #endif

  // setup display
  display->setup();

  // setup alarm clock
  alarmClock->setup();

  // restart the other core
  // rp2040.restartCore1();
}

void loop() {
  // put your main code here, to run repeatedly:
  alarmClock->updateTimePriorityLoop();
}

void setup1() {
  delay(2000);
}

void loop1() {
  // put your main code here, to run repeatedly:
  alarmClock->nonPriorityTasksLoop();
}


// GLOBAL FUNCTIONS

extern "C" char* sbrk(int incr);

// Serial.print(F("- SRAM left: ")); Serial.println(freeRam());
int availableRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

void serialInputFlush() {
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

void serialTimeStampPrefix() {
  Serial.print(F("(s"));
  if(rtc->second() < 10) Serial.print('0');
  Serial.print(rtc->second());
  Serial.print(F(":i"));
  if(alarmClock->inactivitySeconds < 100) Serial.print('0');
  if(alarmClock->inactivitySeconds < 10) Serial.print('0');
  Serial.print(alarmClock->inactivitySeconds);
  Serial.print(F(": SRAM left: ")); Serial.print(availableRam());
  Serial.print(F(") - "));
  Serial.flush();
}
