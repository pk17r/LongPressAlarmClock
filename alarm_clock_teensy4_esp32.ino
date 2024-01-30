/**************************************************************************
  Arduino Alarm Clock using 
    1.8" ST7735S display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

  Adafruit License:

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 **************************************************************************/
#include <PushButtonTaps.h>
// #include <displayData>

// ST7735S Global Variable Space

// SELECT MCU
// #define MCU_IS_ESP32
#define MCU_IS_TEENSY

#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <Fonts/FreeSansBold24pt7b.h>
#if !defined(MCU_IS_ARDUINO)
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#endif
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
#define TFT_CLK 13
#define TFT_CS 10
#define TFT_RST 9  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 8
#define TFT_BL 7  //  controls TFT Display backlight as output of PWM pin
#endif
// For 1.8" TFT with ST7735 using Hardware VSPI Pins COPI and SCK:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
const int nightTimeBrightness = 145;
const int dayTimeBrightness = 125;
const int maxBrightness = 0;

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
const int BUTTON_PIN = 4;
const int LED_PIN = 5;
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
bool refreshTime = false;
volatile bool secondsIncremented = false;
bool alarmOn = true;

// function declerations
void firstTimeDs3231SetFn();
void prepareTimeDayDateArrays();
void serialPrintRtcDateTime();
void displayUpdateFast();
void processSerialInput();
void checkTimeAndSetBrightness();
void serial_input_flush();
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

const unsigned char bell_bitmap[] PROGMEM = {
	// 'bell_40x30, 40x30px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0x00, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x0f, 0xff, 0xe0, 0x00, 0x00, 0x1f, 
	0xff, 0xe0, 0x00, 0x00, 0x1f, 0xff, 0xf0, 0x18, 0x38, 0x1f, 0xff, 0xf8, 0x3c, 0x7e, 0x3f, 0xff, 
	0xf8, 0xfc, 0x3f, 0x3f, 0xff, 0xf9, 0xf0, 0x1f, 0xbf, 0xff, 0xfb, 0xe0, 0x03, 0xbf, 0xff, 0xfd, 
	0x80, 0x00, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x00, 0x70, 0x3f, 0xff, 0xfc, 0x1c, 
	0xff, 0x3f, 0xff, 0xfc, 0xfe, 0xff, 0x7f, 0xff, 0xfd, 0xfe, 0x0f, 0x7f, 0xff, 0xfd, 0xe0, 0x00, 
	0x7f, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x0d, 0xff, 0xff, 0xff, 0x60, 0x3f, 0xff, 
	0xff, 0xff, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0xfe, 0xf9, 0xff, 0xff, 0xff, 0x3e, 0x60, 0x00, 0x00, 
	0x00, 0x0e, 0x00, 0x03, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 
	0x00, 0x00, 0x00, 0x7c, 0x00, 0x00
};
const uint8_t bell_w = 40, bell_h = 30;

const unsigned char bell_fallen_bitmap [] PROGMEM = {
	// 'bell_off, 31x28px
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x01, 0xfc, 0x00, 
	0x00, 0x03, 0xfc, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x03, 0xff, 0xfd, 0xc0, 
	0x07, 0xff, 0xfd, 0xe0, 0x0f, 0xff, 0xfd, 0xf0, 0x1f, 0xff, 0xfd, 0xf8, 0x3f, 0xff, 0xfd, 0xf8, 
	0x3f, 0xff, 0xfc, 0xfc, 0x7f, 0xff, 0xfe, 0xfc, 0x7f, 0xff, 0xfe, 0xfc, 0x7f, 0xff, 0xfe, 0x78, 
	0x7f, 0xff, 0xff, 0x78, 0x7f, 0xff, 0xff, 0x38, 0x7f, 0xff, 0xff, 0x90, 0x7f, 0xff, 0xff, 0xc0, 
	0x3f, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xff, 0xe0, 0x3f, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xe0, 
	0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xe0, 0x01, 0xff, 0x83, 0xc0, 0x00, 0x00, 0x00, 0x00
};
const uint8_t bell_fallen_w = 31, bell_fallen_h = 28;

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
  //pinMode(TFT_BL, OUTPUT);
  checkTimeAndSetBrightness();

  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);  // Init ST7735S chip, black tab

  // set col and row offset of display
  // tft.setColRowStart(2, 1);

  // make display landscape orientation
  tft.setRotation(tft.getRotation() + 1);
  // clear screen
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  Serial.println(F("ST7735 Display Initialized"));

  // seconds blink LED
  pinMode(LED_PIN, OUTPUT);
  
  // initialize push button
  pushBtn.setButtonPin(BUTTON_PIN);
  pushBtn.setButtonActiveLow(false);

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
  displayUpdateFast();

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

      // set display brightness based on time
      checkTimeAndSetBrightness();

      serialTimeStampPrefix();
    }

    // prepare date and time arrays
    prepareTimeDayDateArrays();

    // update TFT display for changes
    displayUpdateFast();

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
    else
      setBrightness(dayTimeBrightness);
  } else if (rtc.hourModeAndAmPm() == 0) {  // 24hr
    if (rtc.hour() > 21 || rtc.hour() < 7)
      setBrightness(nightTimeBrightness);
    else
      setBrightness(dayTimeBrightness);
  }
}

// USER DEFINED LOCATIONS OF VARIOUS DISPLAY TEXT STRINGS
const int16_t TIME_ROW_X0 = 0, TIME_ROW_Y0 = 40;
const int16_t DATE_ROW_Y0 = 75;
const int16_t ALARM_ROW_Y0 = 110;
const int16_t DISPLAY_TEXT_GAP = 5;

// location of various display text strings
int16_t tft_HHMM_x1, tft_HHMM_y1;
uint16_t tft_HHMM_w;
int16_t tft_AmPm_x0, tft_AmPm_y0;
int16_t tft_SS_x0;
int16_t date_row_x0 = 0;
int16_t alarm_row_x0 = 0;
int16_t alarm_icon_x0 = 0, alarm_icon_y0 = 0;

void displayUpdateFast() {
  // HH:MM string and AM/PM string
  if (strcmp(newDisplayData.timeHHMM, displayedData.timeHHMM) != 0) {

    // HH:MM

    // set font
    tft.setFont(&FreeSansBold24pt7b);

    // home the cursor to currently displayed text location
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
    tft.print(displayedData.timeHHMM);

    // home the cursor
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
    // Serial.print("TIME_ROW_X0 "); Serial.print(TIME_ROW_X0); Serial.print(" y0 "); Serial.print(TIME_ROW_Y0); Serial.print(" tft.getCursorX() "); Serial.print(tft.getCursorX()); Serial.print(" tft.getCursorY() "); Serial.println(tft.getCursorY()); 

    // record location of new HH:MM string on tft display (with background color as this causes a blink)
    uint16_t tft_HHMM_h;
    tft.getTextBounds(newDisplayData.timeHHMM, tft.getCursorX(), tft.getCursorY(), &tft_HHMM_x1, &tft_HHMM_y1, &tft_HHMM_w, &tft_HHMM_h);
    // Serial.print("HHMM_x1 "); Serial.print(tft_HHMM_x1); Serial.print(" y1 "); Serial.print(tft_HHMM_y1); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 

    // home the cursor
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Time_Color);

    // draw the new time value
    tft.print(newDisplayData.timeHHMM);

    // and remember the new value
    strcpy(displayedData.timeHHMM, newDisplayData.timeHHMM);

    // AM/PM

    // set font
    tft.setFont(&FreeSans9pt7b);

    // clear old AM/PM
    if(displayedData._12hourMode) {
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
      tft_AmPm_x0 = tft_HHMM_x1 + tft_HHMM_w + 2 * DISPLAY_TEXT_GAP;
      tft_AmPm_y0 = (TIME_ROW_Y0 + tft_HHMM_y1) / 2;

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
      tft_AmPm_y0 -= tft_AmPm_y1 - tft_HHMM_y1;
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
  if (strcmp(newDisplayData.timeSS, displayedData.timeSS) != 0) {
    // set font
    tft.setFont(&FreeSans12pt7b);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // home the cursor
    tft.setCursor(tft_SS_x0, TIME_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
    tft.print(displayedData.timeSS);

    // fill new home values
    tft_SS_x0 = tft_HHMM_x1 + tft_HHMM_w + DISPLAY_TEXT_GAP;

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
  if (strcmp(newDisplayData.dateStr, displayedData.dateStr) != 0) {
    // set font
    tft.setFont(&FreeSansBold12pt7b);

    // yes! home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
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
  if (strcmp(newDisplayData.alarmStr, displayedData.alarmStr) != 0 || newDisplayData._alarmOn != displayedData._alarmOn) {
    // set font
    tft.setFont(&FreeSansBold12pt7b);

    // yes! home the cursor
    tft.setCursor(alarm_row_x0, ALARM_ROW_Y0);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // redraw the old value to erase
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
}

void drawrects() {
  // clear area
  int16_t y_Rect = TIME_ROW_Y0 + 25;
  int16_t h_Rect = tft.height() - TIME_ROW_Y0;
  tft.fillRect(0, y_Rect, tft.width(), h_Rect, ST77XX_BLACK);
  int16_t y_Rect_mid = y_Rect + h_Rect / 2;
  //for (int16_t x = 0; x < tft.width(); x += 6) {
  for (int16_t x = tft.width() - 1; x > 6; x -= 6) {
    tft.fillRect(tft.width() / 2 - x / 2, max(y_Rect_mid - x / 2, y_Rect), x, min(x, h_Rect), ST77XX_BLUE);
    tft.drawRect(tft.width() / 2 - x / 2, max(y_Rect_mid - x / 2, y_Rect), x, min(x, h_Rect), ST77XX_RED);
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













void loopTFT() {
  // // large block of text
  // tft.fillScreen(ST77XX_BLACK);
  // testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
  // Serial.print("testdrawtext ");
  // takeUserInput();
  // delay(500);

  // // a single pixel
  // tft.drawPixel(tft.width()/2, tft.height()/2, ST77XX_GREEN);
  // Serial.print("drawPixel ");
  // takeUserInput();
  // delay(500);

  // // line draw test
  // testlines(ST77XX_YELLOW);
  // Serial.print("testlines ");
  // takeUserInput();
  // delay(500);

  // // optimized lines
  // testfastlines(ST77XX_RED, ST77XX_BLUE);
  // Serial.print("testfastlines ");
  // takeUserInput();
  // delay(500);

  // testdrawrects(ST77XX_GREEN);
  // Serial.print("testdrawrects ");
  // takeUserInput();
  // delay(500);

  // testfillrects(ST77XX_YELLOW, ST77XX_MAGENTA);
  // Serial.print("testfillrects ");
  // takeUserInput();
  // delay(500);

  // tft.fillScreen(ST77XX_BLACK);
  // testfillcircles(10, ST77XX_BLUE);
  // testdrawcircles(10, ST77XX_WHITE);
  // takeUserInput();
  // delay(500);

  // testroundrects();
  // takeUserInput();
  // delay(500);

  // testtriangles();
  // takeUserInput();
  // delay(500);

  // tft.invertDisplay(true);
  // delay(500);
  // tft.invertDisplay(false);
  // delay(500);
}

void testlines(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, 0, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, 0, 0, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, tft.height() - 1, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y = 0; y < tft.height(); y += 5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x = 0; x < tft.width(); x += 5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = tft.width() - 1; x > 6; x -= 6) {
    tft.fillRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x, color1);
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = radius; x < tft.width(); x += radius * 2) {
    for (int16_t y = radius; y < tft.height(); y += radius * 2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = 0; x < tft.width() + radius; x += radius * 2) {
    for (int16_t y = 0; y < tft.height() + radius; y += radius * 2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = tft.width() / 2;
  int x = tft.height() - 1;
  int y = 0;
  int z = tft.width();
  for (t = 0; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x -= 4;
    y += 4;
    z -= 4;
    color += 100;
  }
}

void testroundrects() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for (t = 0; t <= 4; t += 1) {
    int x = 0;
    int y = 0;
    int w = tft.width() - 2;
    int h = tft.height() - 2;
    for (i = 0; i <= 16; i += 1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x += 2;
      y += 3;
      w -= 4;
      h -= 6;
      color += 1100;
    }
    color += 100;
  }
}
