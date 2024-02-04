/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/
#include <PushButtonTaps.h>
// #include <displayData>

// Display Global Variable Space

// SELECT MCU
// #define MCU_IS_ESP32
#define MCU_IS_TEENSY

// SELECT DISPLAY
#define DISPLAY_IS_ST7789V
// #define DISPLAY_IS_ST7735
// #define DISPLAY_IS_ILI9341

#include <Adafruit_GFX.h>     // Core graphics library
// #include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
// #include "Adafruit_ILI9341.h"
#include <Fonts/ComingSoon_Regular70pt7b.h>
#include <Fonts/FreeSansBold48pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <SPI.h>

#if defined(MCU_IS_ESP32)
#define TFT_COPI 23
#define TFT_CLK 18
#define TFT_CS 5
#define TFT_RST 27  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 26
#define TFT_BL 25  //  controls TFT Display backlight as output of PWM pin
#elif defined(MCU_IS_TEENSY)
#define TFT_COPI 11
#define TFT_CIPO 12
#define TFT_CLK 13
#define TFT_CS 10
#define TFT_RST 9  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 8
#define TFT_BL 7  //  controls TFT Display backlight as output of PWM pin
#endif

#if defined(DISPLAY_IS_ST7735)
// For 1.8" TFT with ST7735 using Hardware VSPI Pins COPI and SCK:
// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
#elif defined(DISPLAY_IS_ST7789V)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_COPI, TFT_CLK, TFT_RST);
#elif defined(DISPLAY_IS_ILI9341)
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_COPI, TFT_CLK, TFT_RST, TFT_CIPO);
#endif
const int nightTimeBrightness = 1;
// const int eveningTimeBrightness = 10;
const int dayTimeBrightness = 150;
const int maxBrightness = 255;

// TM1637Display Global Variable Space

#include "uRTCLib.h"
// #include <TM1637Display.h>

#if defined(MCU_IS_ESP32)
#include "WiFi.h"
#endif

// Module connection pins (Digital Pins)
// #define TM1637_CLK 16
// #define TM1637_DIO 17

// TM1637Display tm1637Display(TM1637_CLK, TM1637_DIO);

#if defined(MCU_IS_ESP32)
// Sqw Alarm Interrupt Pin
const byte interruptSqwPin = 4;
const int BUTTON_PIN = 35;
#define BUTTON_PIN_BITMASK 0x800000000  // 2^35 in hex
const int LED_PIN = 32;
#elif defined(MCU_IS_TEENSY)
// Sqw Alarm Interrupt Pin
const byte interruptSqwPin = 17;
const int BUTTON_PIN = 2;
const int LED_PIN = 14;
#endif

// Push Button
PushButtonTaps pushBtn;

// seconds blinker
bool blink = false;

// RTC Lib for DC3231
uRTCLib rtc;

// just a counter to track RTC HW seconds
// we'll refresh RTC time everytime second reaches 60
// All other parameters of RTC will not change at any other time
// at 60 seconds, we'll update the time row
byte second = 0;
bool refreshTime = false, redrawDisplay = false, screensaver = false;
volatile bool secondsIncremented = false;
bool alarmOn = true;
uint8_t inactivitySeconds = 0;
const uint8_t INACTIVITY_SECONDS_LIM = 120;

// function declerations
void firstTimeDs3231SetFn();
void prepareTimeDayDateArrays();
void serialPrintRtcDateTime();
void displayHHMM(bool moveAround);
void displayUpdate();
void processSerialInput();
void checkTimeAndSetBrightness();
void serial_input_flush();
void screensaverControl(bool turnOn);
#if defined(MCU_IS_ESP32)
void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason);
void putEsp32ToLightSleep();
#endif

void sqwPinInterruptFn() {
  secondsIncremented = true;
}

void setBrightness(int brightness) {
#if defined(MCU_IS_ESP32)
  dacWrite(TFT_BL, brightness);
#else
  analogWrite(TFT_BL, brightness);
#endif
  Serial.print(F("Display Brightness set to "));
  Serial.println(brightness);
}

/*
 *  Display Data Storage File
 */

const int timeHHMMArraySize = 6, timeSSArraySize = 4, dateArraySize = 11, alarmArraySize = 10;

struct displayData {
  char timeHHMM[timeHHMMArraySize] = "";
  char timeSS[timeSSArraySize] = "";
  bool _12hourMode;
  bool _pmNotAm;
  char dateStr[dateArraySize] = "";
  char alarmStr[alarmArraySize] = "";
  bool _alarmOn;
} newDisplayData, displayedData;

#if !defined(__AVR__) && !defined(PROGMEM)
#define PROGMEM
#endif

// day arrays
const char day_Sun[] PROGMEM = "Sun";
const char day_Mon[] PROGMEM = "Mon";
const char day_Tue[] PROGMEM = "Tue";
const char day_Wed[] PROGMEM = "Wed";
const char day_Thu[] PROGMEM = "Thu";
const char day_Fri[] PROGMEM = "Fri";
const char day_Sat[] PROGMEM = "Sat";

// Then set up a table to refer to your strings.
const char *const days_table[] PROGMEM = { day_Sun, day_Mon, day_Tue, day_Wed, day_Thu, day_Fri, day_Sat };

// day arrays
const char day_Jan[] PROGMEM = "Jan";
const char day_Feb[] PROGMEM = "Feb";
const char day_Mar[] PROGMEM = "Mar";
const char day_Apr[] PROGMEM = "Apr";
const char day_May[] PROGMEM = "May";
const char day_Jun[] PROGMEM = "Jun";
const char day_Jul[] PROGMEM = "Jul";
const char day_Aug[] PROGMEM = "Aug";
const char day_Sep[] PROGMEM = "Sep";
const char day_Oct[] PROGMEM = "Oct";
const char day_Nov[] PROGMEM = "Nov";
const char day_Dec[] PROGMEM = "Dec";

// Then set up a table to refer to your strings.
const char *const months_table[] PROGMEM = { day_Jan, day_Feb, day_Mar, day_Apr, day_May, day_Jun, day_Jul, day_Aug, day_Sep, day_Oct, day_Nov, day_Dec };

const char amLabel[] = "AM", pmLabel[] = "PM", offLabel[] = "Off", alarmLabel[] = "Alarm";
const char charSpace = ' ', charZero = '0';

// color definitions
const uint16_t  Display_Color_Black        = 0x0000;
const uint16_t  Display_Color_Blue         = 0x001F;
const uint16_t  Display_Color_Red          = 0xF800;
const uint16_t  Display_Color_Green        = 0x07E0;
const uint16_t  Display_Color_Cyan         = 0x07FF;
const uint16_t  Display_Color_Magenta      = 0xF81F;
const uint16_t  Display_Color_Yellow       = 0xFFE0;
const uint16_t  Display_Color_White        = 0xFFFF;

// The colors we actually want to use
uint16_t        Display_Time_Color         = Display_Color_Yellow;
uint16_t        Display_Date_Color         = Display_Color_Green;
uint16_t        Display_Alarm_Color        = Display_Color_Cyan;
uint16_t        Display_Backround_Color    = Display_Color_Black;

// convert image into binary monochrome using https://javl.github.io/image2cpp/
const unsigned char bell_bitmap[] PROGMEM = {
	// 'bell_114x75, 114x75px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x07, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x1f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 
	0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 
	0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 
	0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x03, 0x80, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x07, 
	0xe0, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x07, 0xf8, 
	0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x07, 0xfe, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x03, 0xff, 0xc0, 0x00, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0xff, 0xe0, 0x00, 0x00, 0xff, 0xf0, 0x00, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x07, 0xff, 0x80, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x80, 0x1f, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x80, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xff, 0x81, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xc0, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x81, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xc0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 
	0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x03, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xe0, 0x1f, 0xf0, 0x00, 0x00, 0x1f, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0x3f, 0xff, 0xfc, 0x00, 0x3f, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xf0, 0x3f, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 
	0x3f, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 
	0xff, 0xfe, 0x00, 0x1f, 0xf8, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x0f, 
	0xfc, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x0e, 
	0x00, 0x00, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 
	0x00, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x1f, 0xf8, 0x00, 
	0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 
	0x03, 0xff, 0xf8, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x0f, 0xff, 0xe0, 0x00, 0x1f, 
	0xff, 0xc0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x3f, 0xfe, 
	0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x3f, 0xf0, 0x00, 
	0x00, 0x00, 0x03, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x3f, 0x80, 0x00, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 
	0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00
};
const uint8_t bell_w = 114, bell_h = 75;

const unsigned char bell_fallen_bitmap [] PROGMEM = {
	// 'bell_fallen_75x71, 75x71px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 
	0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x1f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
	0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7c, 
	0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x80, 0x00, 0x00, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xe0, 0x00, 
	0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xf0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xfc, 0x00, 0x00, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xfc, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xef, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 
	0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf7, 0xff, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x0f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xff, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 
	0xfe, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfd, 0xfe, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xfc, 0x00, 
	0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfe, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xf0, 0x00, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x40, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 
	0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x0f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xc0, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x07, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
	0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xf0, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x7f, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xf0, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x0f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 
	0x00, 0x03, 0xff, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xe0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x80, 
	0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x03, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const uint8_t bell_fallen_w = 75, bell_fallen_h = 71;

void setup() {
#if defined(MCU_IS_ESP32)
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  setCpuFrequencyMhz(80);
#endif
  Serial.begin(9600);
  delay(100);
  // while(!Serial) {};
  Serial.println(F("\nSerial OK"));

  /* INITIALIZE DISPLAYS */

  // tft display backlight control PWM output pin
  pinMode(TFT_BL, OUTPUT);

  #if defined(DISPLAY_IS_ST7789V)
  // OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
  tft.init(240, 320);           // Init ST7789 320x240
  tft.invertDisplay(false);
  #elif defined(DISPLAY_IS_ST7735)
  // Use this initializer if using a 1.8" ST7735 TFT screen:
  // tft.initR(INITR_BLACKTAB);  // Init ST7735 chip, black tab
  #elif defined(DISPLAY_IS_ILI9341)
  // Use this initializer if using a 1.8" ILI9341 TFT screen:
  // tft.begin();
  #endif

  // set col and row offset of display for ST7735S
  // tft.setColRowStart(2, 1);

  // make display landscape orientation
  tft.setRotation(tft.getRotation() + 3);
  // clear screen
  tft.fillScreen(Display_Color_Black);
  tft.setTextWrap(false);

  Serial.println(F("Display Initialized"));

  // seconds blink LED
  pinMode(LED_PIN, OUTPUT);
  
  // initialize push button
  pushBtn.setButtonPin(BUTTON_PIN);
  // pushBtn.setButtonActiveLow(false);

  // // TM1637 Display set brightness 0 to 7
  // tm1637Display.setBrightness(3);

  // Serial.println(F("TM1637 Display Initialized"));

  // seconds interrupt pin
  pinMode(interruptSqwPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptSqwPin), sqwPinInterruptFn, RISING);

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.begin();

  // set rtc model
  rtc.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231
  rtc.refresh();

  // to setup DS3231 - run this only for the first time during setup, thereafter set back to false and flash the code again so this is never run again
  if (0) {
    // Set Oscillator to use VBAT when VCC turns off
    if(rtc.enableBattery())
      Serial.println(F("Enable Battery Success"));
    else
      Serial.println(F("Enable Battery UNSUCCESSFUL!"));

    // disable SQ wave out
    rtc.disable32KOut();
    Serial.println(F("disable32KOut() done"));

    // stop sq wave on sqw pin
    rtc.sqwgSetMode(URTCLIB_SQWG_OFF_1);
    Serial.println(F("stop sq wave on sqw pin. Mode set: URTCLIB_SQWG_OFF_1"));

    // clear alarms flags
    rtc.alarmClearFlag(URTCLIB_ALARM_1);
    rtc.alarmClearFlag(URTCLIB_ALARM_2);
    Serial.println(F("alarmClearFlag() done"));

    // disable alarms
    rtc.alarmDisable(URTCLIB_ALARM_1);
    rtc.alarmDisable(URTCLIB_ALARM_2);
    Serial.println(F("alarmDisable() done"));

    // Set current time and date
    Serial.println();
    Serial.println(F("Waiting for input from user to set time."));
    Serial.println(F("Provide a keyboard input when set time is equal to real world time..."));
    while (Serial.available() == 0) {};
    // Only used once, then disabled
    rtc.set(0, 30, 2, 6, 26, 1, 24);
    //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
    Serial.println(F("Time set"));
    serial_input_flush();
  }

  // Check if time is up to date
  Serial.print(F("Lost power status: "));
  if (rtc.lostPower()) {
    Serial.println(F("POWER FAILED. Clearing flag..."));
    rtc.lostPowerClear();
  }
  else
    Serial.println(F("POWER OK"));

  // Check whether OSC is set to use VBAT or not
  if (rtc.getEOSCFlag())
    Serial.println(F("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!"));
  else
    Serial.println(F("Oscillator will use VBAT if VCC cuts off."));

  // we won't use RTC for alarm
  rtc.alarmDisable(URTCLIB_ALARM_1);
  rtc.alarmDisable(URTCLIB_ALARM_2);

  // make second equal to rtc second (-1 just to make first rtc refresh come later than when it should, as there is no time sync initially)
  second = rtc.second() - 1;

  // prepare date and time arrays and serial print RTC Date Time
  prepareTimeDayDateArrays();

  // update TFT display
  displayUpdate();
  checkTimeAndSetBrightness();

  // set sqw pin to trigger every second
  rtc.sqwgSetMode(URTCLIB_SQWG_1H);
}

void serialTimeStampPrefix() {
  Serial.print(F("("));
  Serial.print(millis());
  Serial.print(newDisplayData.timeSS);
  Serial.print(F(") - "));
  Serial.flush();
}

void loop() {
  if(pushBtn.checkButtonStatus() != 0) {
    setBrightness(maxBrightness);
    screensaverControl(false);
  }

  // process time actions every second
  if (secondsIncremented) {
    secondsIncremented = false;

    // update seconds
    ++second;
    if (second >= 60)
      refreshTime = true;

    // prepare updated seconds array
    snprintf(newDisplayData.timeSS, timeSSArraySize, ":%02d", second);

    serialTimeStampPrefix();

    // blink LED every second
    blink = !blink;
    digitalWrite(LED_PIN, blink);

    // get time update
    if (refreshTime) {
      Serial.println(F("__RTC Refresh__ "));
      rtc.refresh();
      refreshTime = false;

      // make second equal to rtc seconds -> should be 0
      second = rtc.second();

      serialTimeStampPrefix();

      // Check if time is up to date
      if (rtc.lostPower()) {
        Serial.println(F("POWER FAILED. Time is not up to date!"));
        Serial.println(F("Stopping!"));
        exit(1);
      }

      // Check whether OSC is set to use VBAT or not
      if (rtc.getEOSCFlag()) {
        Serial.println(F("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!"));
      }

      serialTimeStampPrefix();
    }

    // prepare date and time arrays
    prepareTimeDayDateArrays();

    // check for inactivity
    if(inactivitySeconds <= INACTIVITY_SECONDS_LIM) {
      inactivitySeconds++;
      if(inactivitySeconds >= INACTIVITY_SECONDS_LIM) {
        // set display brightness based on time
        checkTimeAndSetBrightness();
        // turn screen saver On
        if(!screensaver)
          screensaverControl(true);
      }
    }

    // update TFT display for changes
    if(!screensaver)
      displayUpdate();
    else
      displayHHMM(true);

    // serial print RTC Date Time
    serialPrintRtcDateTime();

    // animation
    // drawrects();

    // display time
    // int displayNumber = rtc.hour()*100 + rtc.minute();
    // tm1637Display.showNumberDecEx(displayNumber, blink * 0xff, false, 4);

    Serial.println();

#if defined(MCU_IS_ESP32)
    // if button is inactive, then go to sleep
    if(!pushBtn.buttonActiveDebounced())
      putEsp32ToLightSleep();
#endif
  }

  // accept user inputs
  if (Serial.available() != 0)
    processSerialInput();
}

void checkTimeAndSetBrightness() {
  if (rtc.hourModeAndAmPm() == 1) {  // 12hr AM
    if (rtc.hour() < 7 || rtc.hour() == 12)
      setBrightness(nightTimeBrightness);
    else
      setBrightness(dayTimeBrightness);
  } else if (rtc.hourModeAndAmPm() == 2) {  // 12hr PM
    if (rtc.hour() > 10 && rtc.hour() != 12)
      setBrightness(nightTimeBrightness);
    // else if(rtc.hour() > 6)
    //   setBrightness(eveningTimeBrightness);
    else
      setBrightness(dayTimeBrightness);
  } else if (rtc.hourModeAndAmPm() == 0) {  // 24hr
    if (rtc.hour() > 21 || rtc.hour() < 7)
      setBrightness(nightTimeBrightness);
    // else if(rtc.hour() > 18)
    //   setBrightness(eveningTimeBrightness);
    else
      setBrightness(dayTimeBrightness);
  }
}

// USER DEFINED LOCATIONS OF VARIOUS DISPLAY TEXT STRINGS
const int16_t TIME_ROW_X0 = 0, TIME_ROW_Y0 = 80;
const int16_t DATE_ROW_Y0 = 135;
const int16_t ALARM_ROW_Y0 = 210;
const int16_t DISPLAY_TEXT_GAP = 10;

// location of various display text strings
int16_t gap_right_x, gap_up_y;
int16_t tft_HHMM_x0 = TIME_ROW_X0, tft_HHMM_y0 = 2 * TIME_ROW_Y0;   // default starting location of screensaver
uint16_t tft_HHMM_w, tft_HHMM_h;
int16_t tft_AmPm_x0, tft_AmPm_y0;
int16_t tft_SS_x0;
int16_t date_row_x0 = 0;
int16_t alarm_row_x0 = 0;
int16_t alarm_icon_x0 = 0, alarm_icon_y0 = 0;

bool goDown = true, goRight = true;
void screensaverControl(bool turnOn) {
  // clear screen
  tft.fillScreen(Display_Color_Black);
  screensaver = turnOn;
  redrawDisplay = true;
  inactivitySeconds = 0;
  Serial.print("screensaver "); Serial.println(screensaver);
}

void displayHHMM(bool moveAround) {
  bool isThisTheFirstTime = strcmp(displayedData.timeSS, "") == 0;

  // HH:MM

  // set font
  // tft.setTextSize(1);
  if(screensaver)
    tft.setFont(&ComingSoon_Regular70pt7b);
  else
    tft.setFont(&FreeSansBold48pt7b);

  // change the text color to the background color
  tft.setTextColor(Display_Backround_Color);

  // clear old time if it was there
  if(!isThisTheFirstTime) {
    // home the cursor to currently displayed text location
    if(!moveAround || isThisTheFirstTime)
      tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
    else
      tft.setCursor(tft_HHMM_x0, tft_HHMM_y0);

    // redraw the old value to erase
    tft.print(displayedData.timeHHMM);
  }

  // record location of new HH:MM string on tft display (with background color as this causes a blink)
  tft.getTextBounds(newDisplayData.timeHHMM, 0, 0, &gap_right_x, &gap_up_y, &tft_HHMM_w, &tft_HHMM_h);
  // Serial.print("gap_right_x "); Serial.print(gap_right_x); Serial.print(" gap_up_y "); Serial.print(gap_up_y); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 

  if(moveAround) {
    const int16_t adder = 2;
    if(tft_HHMM_x0 <= adder)  goRight = true;
    else if(tft_HHMM_x0 + gap_right_x + tft_HHMM_w >= tft.width() - adder)  goRight = false;
    if(tft_HHMM_y0 + gap_up_y <= adder)  goDown = true;
    else if(tft_HHMM_y0 + gap_up_y + tft_HHMM_h >= tft.height() - adder)  goDown = false;
    // Serial.print("Bx0-adder "); Serial.println(tft_HHMM_x0 - adder);
    // Serial.print("tft_HHMM_x0 + gap_right_x + tft_HHMM_w - tft.width() + adder "); Serial.println(tft_HHMM_x0 + gap_right_x + tft_HHMM_w - tft.width() + adder);
    // Serial.print("tft_HHMM_y0 + gap_up_y - adder "); Serial.println(tft_HHMM_y0 + gap_up_y - adder);
    // Serial.print("tft_HHMM_y0 + gap_up_y + tft_HHMM_h - tft.height() + adder "); Serial.println(tft_HHMM_y0 + gap_up_y + tft_HHMM_h - tft.height() + adder);
    // Serial.print(" goRight "); Serial.print(goRight); Serial.print(" goDown "); Serial.println(goDown);
    tft_HHMM_x0 += (goRight ? adder : -adder);
    tft_HHMM_y0 += (goDown ? adder : -adder);
    // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
    // delay(2000);
  }

  // home the cursor
  if(!moveAround || isThisTheFirstTime)
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
  else
    tft.setCursor(tft_HHMM_x0, tft_HHMM_y0);

  // change the text color to foreground color
  tft.setTextColor(Display_Time_Color);

  // draw the new time value
  tft.print(newDisplayData.timeHHMM);
  // tft.setTextSize(1);

  // and remember the new value
  strcpy(displayedData.timeHHMM, newDisplayData.timeHHMM);
}

void displayUpdate() {
  bool isThisTheFirstTime = strcmp(displayedData.timeSS, "") == 0;

  // HH:MM string and AM/PM string
  if (strcmp(newDisplayData.timeHHMM, displayedData.timeHHMM) != 0 || redrawDisplay) {

    // HH:MM
    displayHHMM(false);

    // AM/PM

    // set font
    tft.setFont(&FreeSans18pt7b);

    // clear old AM/PM
    if(displayedData._12hourMode && !isThisTheFirstTime) {
      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);

      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // redraw the old value to erase
      if(displayedData._pmNotAm)
        tft.print(pmLabel);
      else
        tft.print(amLabel);
    }

    // draw new AM/PM
    if(newDisplayData._12hourMode) {
      // set test location of Am/Pm
      tft_AmPm_x0 = TIME_ROW_X0 + gap_right_x + tft_HHMM_w + 2 * DISPLAY_TEXT_GAP;
      tft_AmPm_y0 = TIME_ROW_Y0 + gap_up_y / 2;

      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);
      // Serial.print("tft_AmPm_x0 "); Serial.print(tft_AmPm_x0); Serial.print(" y0 "); Serial.print(tft_AmPm_y0); Serial.print(" tft.getCursorX() "); Serial.print(tft.getCursorX()); Serial.print(" tft.getCursorY() "); Serial.println(tft.getCursorY()); 

      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // record location of new AM/PM string on tft display (with background color as this causes a blink)
      int16_t tft_AmPm_x1, tft_AmPm_y1;
      uint16_t tft_AmPm_w, tft_AmPm_h;
      tft.getTextBounds((newDisplayData._pmNotAm ? pmLabel : amLabel), tft.getCursorX(), tft.getCursorY(), &tft_AmPm_x1, &tft_AmPm_y1, &tft_AmPm_w, &tft_AmPm_h);
      // Serial.print("AmPm_x1 "); Serial.print(tft_AmPm_x1); Serial.print(" y1 "); Serial.print(tft_AmPm_y1); Serial.print(" w "); Serial.print(tft_AmPm_w); Serial.print(" h "); Serial.println(tft_AmPm_h); 

      // calculate tft_AmPm_y0 to align top with HH:MM
      tft_AmPm_y0 -= tft_AmPm_y1 - TIME_ROW_Y0 - gap_up_y;
      // Serial.print("tft_AmPm_y0 "); Serial.println(tft_AmPm_y0);

      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);

      // change the text color to foreground color
      tft.setTextColor(Display_Time_Color);

      // draw the new time value
      if(newDisplayData._pmNotAm)
        tft.print(pmLabel);
      else
        tft.print(amLabel);
    }

    // and remember the new value
    displayedData._12hourMode = newDisplayData._12hourMode;
    displayedData._pmNotAm = newDisplayData._pmNotAm;
  }

  // :SS string
  if (strcmp(newDisplayData.timeSS, displayedData.timeSS) != 0 || redrawDisplay) {
    // set font
    tft.setFont(&FreeSans24pt7b);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // home the cursor
    tft.setCursor(tft_SS_x0, TIME_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
    if(!isThisTheFirstTime)
      tft.print(displayedData.timeSS);

    // fill new home values
    tft_SS_x0 = TIME_ROW_X0 + gap_right_x + tft_HHMM_w + DISPLAY_TEXT_GAP;

    // home the cursor
    tft.setCursor(tft_SS_x0, TIME_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Time_Color);

    // draw the new time value
    tft.print(newDisplayData.timeSS);

    // and remember the new value
    strcpy(displayedData.timeSS, newDisplayData.timeSS);
  }
  
  // date string center aligned
  if (strcmp(newDisplayData.dateStr, displayedData.dateStr) != 0 || redrawDisplay) {
    // set font
    tft.setFont(&FreeSansBold24pt7b);

    // yes! home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
    if(!isThisTheFirstTime)
      tft.print(displayedData.dateStr);

    // home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // record date_row_w to calculate center aligned date_row_x0 value
    int16_t date_row_y1;
    uint16_t date_row_w, date_row_h;
    // record location of new dateStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.dateStr, tft.getCursorX(), tft.getCursorY(), &date_row_x0, &date_row_y1, &date_row_w, &date_row_h);
    date_row_x0 = (tft.width() - date_row_w) / 2;

    // home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Date_Color);

    // draw the new dateStr value
    tft.print(newDisplayData.dateStr);

    // and remember the new value
    strcpy(displayedData.dateStr, newDisplayData.dateStr);
  }

  // alarm string center aligned
  if (strcmp(newDisplayData.alarmStr, displayedData.alarmStr) != 0 || newDisplayData._alarmOn != displayedData._alarmOn || redrawDisplay) {
    // set font
    tft.setFont(&FreeSansBold24pt7b);

    // yes! home the cursor
    tft.setCursor(alarm_row_x0, ALARM_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
    if(!isThisTheFirstTime)
      tft.print(displayedData.alarmStr);

    int16_t alarm_icon_w, alarm_icon_h;
    if(displayedData._alarmOn) {
      alarm_icon_w = bell_w;
      alarm_icon_h = bell_h;
    }
    else {
      alarm_icon_w = bell_fallen_w;
      alarm_icon_h = bell_fallen_h;
    }

    // erase bell
    if(!isThisTheFirstTime)
      tft.drawBitmap(alarm_icon_x0, alarm_icon_y0, (displayedData._alarmOn ? bell_bitmap : bell_fallen_bitmap), alarm_icon_w, alarm_icon_h, Display_Backround_Color);

    //  Redraw new alarm data

    if(newDisplayData._alarmOn) {
      alarm_icon_w = bell_w;
      alarm_icon_h = bell_h;
    }
    else {
      alarm_icon_w = bell_fallen_w;
      alarm_icon_h = bell_fallen_h;
    }

    // home the cursor
    tft.setCursor(alarm_icon_x0, ALARM_ROW_Y0);

    // record alarm_row_w to calculate center aligned alarm_row_x0 value
    int16_t alarm_row_y1;
    uint16_t alarm_row_w, alarm_row_h;
    // record location of new alarmStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.alarmStr, tft.getCursorX(), tft.getCursorY(), &alarm_row_x0, &alarm_row_y1, &alarm_row_w, &alarm_row_h);
    uint16_t graphic_width = alarm_icon_w + alarm_row_w;
    // three equal length gaps on left center and right of graphic
    uint16_t equal_gaps = (tft.width() - graphic_width) / 3;
    alarm_row_x0 = equal_gaps + alarm_icon_w + equal_gaps;
    alarm_icon_x0 = equal_gaps;
    // align bell center with text center
    alarm_icon_y0 = ALARM_ROW_Y0 - alarm_row_h / 2 - alarm_icon_h / 2;

    // home the cursor
    tft.setCursor(alarm_row_x0, ALARM_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Alarm_Color);

    // draw the new alarmStr value
    tft.print(newDisplayData.alarmStr);

    // draw bell
    tft.drawBitmap(alarm_icon_x0, alarm_icon_y0, (newDisplayData._alarmOn ? bell_bitmap : bell_fallen_bitmap), alarm_icon_w, alarm_icon_h, Display_Alarm_Color);

    // and remember the new value
    strcpy(displayedData.alarmStr, newDisplayData.alarmStr);
    displayedData._alarmOn = newDisplayData._alarmOn;
  }

  redrawDisplay = false;
}

void drawrects() {
  // clear area
  int16_t y_Rect = TIME_ROW_Y0 + 25;
  int16_t h_Rect = tft.height() - TIME_ROW_Y0;
  tft.fillRect(0, y_Rect, tft.width(), h_Rect, Display_Color_Black);
  int16_t y_Rect_mid = y_Rect + h_Rect / 2;
  //for (int16_t x = 0; x < tft.width(); x += 6) {
  for (int16_t x = tft.width() - 1; x > 6; x -= 6) {
    tft.fillRect(tft.width() / 2 - x / 2, max(y_Rect_mid - x / 2, y_Rect), x, min(x, h_Rect), Display_Color_Blue);
    tft.drawRect(tft.width() / 2 - x / 2, max(y_Rect_mid - x / 2, y_Rect), x, min(x, h_Rect), Display_Color_Red);
    delay(20);
  }
}

void serial_input_flush() {
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

void processSerialInput() {
  // take user input
  char input = Serial.read();
  serial_input_flush();
  // acceptable user input
  Serial.print(F("User input: "));
  Serial.println(input);
  // process user input
  switch (input) {
    case 'a':
      Serial.println(F("**** Toggle Alarm ****"));
      alarmOn = !alarmOn;
      Serial.print(F("alarmOn = ")); Serial.println(alarmOn);
      break;
    case 'b':
      {
        Serial.println(F("**** Set Brightness [0-255] ****"));
        while (Serial.available() == 0) {};
        int brightnessVal = Serial.parseInt();
        serial_input_flush();
        setBrightness(brightnessVal);
      }
      break;
    case 'd':  //disable battery
      {
        Serial.println(F("Disable Battery"));
        bool ret = rtc.disableBattery();
        if (ret) Serial.println(F("Disable Battery Success"));
        else Serial.println(F("Could not Disable Battery!"));
      }
      break;
    case 'e':  //enable battery
      {
        Serial.println(F("Enable Battery"));
        bool ret = rtc.enableBattery();
        if (ret) Serial.println(F("Enable Battery Success"));
        else Serial.println(F("Could not Enable Battery!"));
      }
      break;
    case 'g': // good morning
      {
        tft.fillScreen(Display_Color_Black);
        // set font
        tft.setFont(&FreeSansBold24pt7b);

        // change the text color to the background color
        tft.setTextColor(Display_Color_Green);

        // yes! home the cursor
        tft.setCursor(80, 40);
        // redraw the old value to erase
        tft.print(F("GOOD"));
        // yes! home the cursor
        tft.setCursor(20, 80);
        // redraw the old value to erase
        tft.print(F("MORNING!!"));

        int16_t x0 = 80, y0 = 80;
        uint16_t edge = 160;
        
        unsigned int startTime = millis();
        while(millis() - startTime < 5000)
          drawSun(x0, y0, edge);

        tft.fillScreen(Display_Color_Black);
        refreshTime = true;
        redrawDisplay = true;
      }
      break;
    case 'h':
      {
        Serial.println(F("**** Set clock 12/24 hr mode ****"));
        Serial.println(F("Enter 'twelveHrMode' = 0 or 1"));
        while (Serial.available() == 0) {};
        unsigned int clockModeInp = Serial.parseInt();
        Serial.println(clockModeInp);
        serial_input_flush();
        rtc.set_12hour_mode((bool)clockModeInp);
        // print RTC Date Time and Alarm
        refreshTime = true;
      }
      break;
    case 's':
      {
        Serial.println(F("**** Screensaver ****"));
        screensaverControl(!screensaver);
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}

void prepareTimeDayDateArrays() {
  // HH:MM:
  if(rtc.hour() < 10)
    snprintf(newDisplayData.timeHHMM, timeHHMMArraySize, " %d:%02d", rtc.hour(), rtc.minute());
  else
    snprintf(newDisplayData.timeHHMM, timeHHMMArraySize, "%d:%02d", rtc.hour(), rtc.minute());
  // SS
  snprintf(newDisplayData.timeSS, timeSSArraySize, ":%02d", second);
  if(rtc.hourModeAndAmPm() == 0)
    newDisplayData._12hourMode = false;
  else if(rtc.hourModeAndAmPm() == 1) {
    newDisplayData._12hourMode = true;
    newDisplayData._pmNotAm = false;
  }
  else {
    newDisplayData._12hourMode = true;
    newDisplayData._pmNotAm = true;
  }
  char dayArray[4] = "";
  char monthArray[4] = "";
  // Month
  strcpy_P(monthArray, (char *)pgm_read_ptr(&(months_table[rtc.month() - 1])));  // Necessary casts and dereferencing, just copy.
  // DoWX
  strcpy_P(dayArray, (char *)pgm_read_ptr(&(days_table[rtc.dayOfWeek() - 1])));  // Necessary casts and dereferencing, just copy.
  // Mon dd Day
  snprintf(newDisplayData.dateStr, dateArraySize, "%s %d %s", monthArray, rtc.day(), dayArray);
  if(alarmOn)
    snprintf(newDisplayData.alarmStr, alarmArraySize, " 7:00 AM");
  else
    snprintf(newDisplayData.alarmStr, alarmArraySize, "%s %s", alarmLabel, offLabel);
  newDisplayData._alarmOn = alarmOn;
}

void serialPrintRtcDateTime() {
  // full serial print time date day array
  Serial.print(newDisplayData.timeHHMM);
  // snprintf(timeArraySec, timeSSArraySize, ":%02d", second);
  Serial.print(newDisplayData.timeSS);
  if (newDisplayData._12hourMode) {
    Serial.print(charSpace);
    if (newDisplayData._pmNotAm)
      Serial.print(pmLabel);
    else
      Serial.print(amLabel);
  }
  Serial.print(charSpace);
  Serial.print(newDisplayData.dateStr);
  Serial.print(charSpace);
  Serial.print(newDisplayData.alarmStr);
  Serial.flush();
}

#if defined(MCU_IS_ESP32)
/*
  Esp32 light sleep function
  https://lastminuteengineers.com/esp32-deep-sleep-wakeup-sources/
*/
void putEsp32ToLightSleep() {
  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */
  // add a timer to wake up ESP32
  esp_sleep_enable_timer_wakeup(500000); //0.5 seconds
  // ext1 button press as wake up source
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  //Go to sleep now
  serialTimeStampPrefix();
  Serial.println("Go To Light Sleep for 0.5 sec or button press");
  Serial.flush();
  // go to light sleep
  esp_light_sleep_start();
  // On WAKEUP disable timer as wake up source
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  //Print the wakeup reason for ESP32
  serialTimeStampPrefix();
  print_wakeup_reason(wakeup_reason);

  // if wakeup reason was timer then add seconds ticker signal to wake up source and go back to sleep
  if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // add ext0 RTC seconds ticker as wake up source
    esp_sleep_enable_ext0_wakeup((gpio_num_t)interruptSqwPin,1); //Wake up at: 1 = High, 0 = Low
    //Go to sleep now
    serialTimeStampPrefix();
    Serial.println("Go To Light Sleep until seconds tick or button press");
    //esp_deep_sleep_start();
    Serial.flush();
    // go to light sleep
    esp_light_sleep_start();

    // On WAKEUP disable EXT0 as wake up source
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);

    wakeup_reason = esp_sleep_get_wakeup_cause();
    // if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    //   turnBacklightOn();

    //Print the wakeup reason for ESP32
    serialTimeStampPrefix();
    print_wakeup_reason(wakeup_reason);
  }

  // update time
  // secondsIncremented = true;
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason){
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println(F("Wakeup by ext signal RTC_IO - SECONDS TICK")); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println(F("Wakeup by ext signal RTC_CNTL - BUTTON PRESS")); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println(F("Wakeup caused by TIMER")); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println(F("Wakeup caused by touchpad")); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println(F("Wakeup caused by ULP program")); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
#endif


/* draw Sun
 * 
 * params: top left corner 'x0' and 'y0', square edge length of graphic 'edge'
 */ 
void drawSun(int16_t x0, int16_t y0, uint16_t edge) {

  // set dimensions of sun and rays

  // sun center
  int16_t cx = x0 + edge / 2, cy = y0 + edge / 2;
  // sun radius
  int16_t sr = edge * 0.23;
  // length of rays
  int16_t rl = edge * 0.09;
  // rays inner radius
  int16_t rr = sr + edge * 0.08;
  // width of rays
  int16_t rw = 5;
  // number of rays
  uint8_t rn = 12;

  // color
  uint16_t color = Display_Color_Yellow;
  uint16_t background = Display_Color_Black;

  int16_t variation_prev = 0;

  // sun
  tft.fillCircle(cx, cy, sr, color);

  // eyes
  int16_t eye_offset_x = sr / 2, eye_offset_y = sr / 3, eye_r = max(sr / 8, 3);
  tft.fillCircle(cx - eye_offset_x, cy - eye_offset_y, eye_r, background);
  tft.fillCircle(cx + eye_offset_x, cy - eye_offset_y, eye_r, background);

  // smile
  int16_t smile_angle_deg = 37;
  int16_t smile_cy = cy - sr / 2;
  int16_t smile_r = sr * 1.1, smile_w = max(sr / 15, 3);
  for(uint8_t i = 0; i <= smile_angle_deg; i=i+2) {
    float smile_angle_rad = DEG_TO_RAD * i;
    int16_t smile_tapered_w = max(smile_w - i / 13, 1);
    // Serial.print(i); Serial.print(" "); Serial.print(smile_w); Serial.print(" "); Serial.println(smile_tapered_w);
    int16_t smile_offset_x = smile_r * sin(smile_angle_rad), smile_offset_y = smile_r * cos(smile_angle_rad);
    tft.fillCircle(cx - smile_offset_x, smile_cy + smile_offset_y, smile_tapered_w, background);
    tft.fillCircle(cx + smile_offset_x, smile_cy + smile_offset_y, smile_tapered_w, background);
  }

  // rays
  for(int16_t i = 0; i < 120; i++) {
    // variation goes from 0 to 5 to 0
    int16_t i_base10_fwd = i % 10;
    int16_t i_base10_bwd = ((i / 10) + 1) * 10 - i;
    int16_t variation = min(i_base10_fwd, i_base10_bwd);
    // Serial.println(variation);
    int16_t r_variable = rr + variation;
    // draw rays
    drawRays(cx, cy, r_variable, rl, rw, rn, i, color);
    // increase sun size
    // tft.drawCircle(cx, cy, sr + variation, color);
    drawDenseCircle(cx, cy, sr + variation, color);
    // show for sometime
    delay(30);

    // undraw rays
    drawRays(cx, cy, r_variable, rl, rw, rn, i, background);
    // reduce sun size
    if(variation < variation_prev){
      // tft.drawCircle(cx, cy, sr + variation_prev, background);
      drawDenseCircle(cx, cy, sr + variation_prev + 1, background);
    }
    // delay(1000);
    variation_prev = variation;
  }
}

/* draw rays
 * 
 * params: center cx, cy; inner radius of rays rr, length of rays rl, width of rays rw, number of rays rn, start angle degStart, color
 */ 
void drawRays(int16_t &cx, int16_t &cy, int16_t &rr, int16_t &rl, int16_t &rw, uint8_t &rn, int16_t &degStart, uint16_t &color) {
  // rays
  for(uint8_t i = 0; i < rn; i++) {
    // find coordinates of two triangles for each ray and use fillTriangle function to draw rays
    float theta = 2 * PI * i / rn + degStart * DEG_TO_RAD;
    double rcos = rr * cos(theta), rlcos = (rr + rl) * cos(theta), rsin = rr * sin(theta), rlsin = (rr + rl) * sin(theta);
    double w2sin = rw / 2 * sin(theta), w2cos = rw / 2 * cos(theta);
    int16_t x1 = cx + rcos - w2sin;
    int16_t x2 = cx + rcos + w2sin;
    int16_t x3 = cx + rlcos + w2sin;
    int16_t x4 = cx + rlcos - w2sin;
    int16_t y1 = cy + rsin + w2cos;
    int16_t y2 = cy + rsin - w2cos;
    int16_t y3 = cy + rlsin - w2cos;
    int16_t y4 = cy + rlsin + w2cos;
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    tft.fillTriangle(x1, y1, x3, y3, x4, y4, color);
  }
}

/* drawDenseCircle
 * densely pack a circle's circumference
 */
void drawDenseCircle(int16_t &cx, int16_t &cy, int16_t r, uint16_t &color) {
  // calculate angular resolution required
  // r*dTheta = 0.5
  double dTheta = 0.5 / static_cast<double>(r);
  // number of runs to cover quarter circle
  uint32_t n = PI / 2 / dTheta;
  // Serial.print("dTheta "); Serial.print(dTheta, 5); Serial.print(" n "); Serial.println(n);

  for(uint32_t i = 0; i < n; i++) {
    float theta = i * dTheta; // Serial.print(" theta "); Serial.println(theta);
    int16_t rcos = r * cos(theta);
    int16_t rsin = r * sin(theta);
    tft.drawPixel(cx + rcos, cy + rsin, color);
    tft.drawPixel(cx - rcos, cy + rsin, color);
    tft.drawPixel(cx + rcos, cy - rsin, color);
    tft.drawPixel(cx - rcos, cy - rsin, color);
  }
}








