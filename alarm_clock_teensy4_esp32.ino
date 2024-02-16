/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/
#include "alarm_clock.h"
#include "rgb_display.h"
#if defined(MCU_IS_ESP32) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include "wifi_stuff.h"
#endif
#include "eeprom.h"
#include <PushButtonTaps.h>
#if defined(TOUCHSCREEN_IS_XPT2046)
  #include "touchscreen.h"
#endif

RGBDisplay* display = NULL;   // display class object
AlarmClock* alarmClock = NULL;  // object that controls RTC and Alarm functions
WiFiStuff* wifiStuff = NULL;  // WiFi and Weather fetch functions
EEPROM* eeprom = NULL;    // External EEPROM
PushButtonTaps pushBtn;   // Push Button
#if defined(TOUCHSCREEN_IS_XPT2046)
  Touchscreen ts;         // touchscreen
#endif

extern "C" char* sbrk(int incr);

// Serial.print(F("- SRAM left: ")); Serial.println(freeRam());
int freeRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

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
    wifiStuff->setup(eeprom);
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
