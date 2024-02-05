#include "pin_defs.h"
#include "rgb_display_class.h"
#include "uRTCLib.h"
#include <Arduino.h>

void rgb_display_class::setup() {
  
  /* INITIALIZE DISPLAYS */

  // tft display backlight control PWM output pin
  pinMode(TFT_BL, OUTPUT);

#if defined(DISPLAY_IS_ST7789V)

  // OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
  tft.init(240, 320);           // Init ST7789 320x240
  tft.invertDisplay(false);
  // make display landscape orientation
  tft.setRotation(3);

#elif defined(DISPLAY_IS_ST7735)

  // Use this initializer if using a 1.8" ST7735 TFT screen:
  tft.initR(INITR_BLACKTAB);  // Init ST7735 chip, black tab

#elif defined(DISPLAY_IS_ILI9341)

  // Use this initializer if using a 1.8" ILI9341 TFT screen:
  tft.begin();

#elif defined(DISPLAY_IS_ILI9488)

  tft.begin();
  tft.setRotation(tft.getRotation() + 2);
  int16_t x, y;
  tft.getOrigin(&x, &y);
  Serial.print("x="); Serial.print(x); Serial.print(" y="); Serial.println(y);
  x = 0, y = 0;
  tft.setOrigin(x, y);
  tft.setClipRect();
  tft.setFontAdafruit();
  tft.invertDisplay(true);
  // make display landscape orientation
  Serial.print("h="); Serial.print(tft.height()); Serial.print(" w="); Serial.println(tft.width());

#endif

  // set col and row offset of display for ST7735S
  // tft.setColRowStart(2, 1);

  // SPI speed defaults to SPI_DEFAULT_FREQ defined in the library, you can override it here
  // Note that speed allowable depends on chip and quality of wiring, if you go too fast, you
  // may end up with a black screen some times, or all the time.
  // tft.setSPISpeed(8000000);

  // clear screen
  tft.fillScreen(Display_Color_Black);
  tft.setTextWrap(false);

  Serial.println(F("Display Initialized"));

}

// set display brightness function
void rgb_display_class::setBrightness(int brightness) {
#if defined(MCU_IS_ESP32)
  dacWrite(TFT_BL, brightness);
#else
  analogWrite(TFT_BL, brightness);
#endif
  Serial.print(F("Display Brightness set to "));
  Serial.println(brightness);
}

void rgb_display_class::checkTimeAndSetBrightness() {
  extern uRTCLib rtc;
  if (rtc.hourModeAndAmPm() == 1) {  // 12hr AM
    if (rtc.hour() < 7 || rtc.hour() == 12)
      setBrightness(NIGHT_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  } else if (rtc.hourModeAndAmPm() == 2) {  // 12hr PM
    if (rtc.hour() > 10 && rtc.hour() != 12)
      setBrightness(NIGHT_BRIGHTNESS);
    // else if(rtc.hour() > 6)
    //   setBrightness(EVENING_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  } else if (rtc.hourModeAndAmPm() == 0) {  // 24hr
    if (rtc.hour() > 21 || rtc.hour() < 7)
      setBrightness(NIGHT_BRIGHTNESS);
    // else if(rtc.hour() > 18)
    //   setBrightness(EVENING_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  }
}

void rgb_display_class::screensaverControl(bool turnOn) {
  // clear screen
  tft.fillScreen(Display_Color_Black);
  screensaver = turnOn;
  redrawDisplay = true;
  extern bool inactivitySeconds;
  inactivitySeconds = 0;
  Serial.print("screensaver "); Serial.println(screensaver);
}

void rgb_display_class::prepareTimeDayDateArrays() {
  // HH:MM
  extern uRTCLib rtc;
  if(!screensaver && rtc.hour() < 10)
    snprintf(newDisplayData.timeHHMM, HHMM_ARR_SIZE, " %d:%02d", rtc.hour(), rtc.minute());
  else
    snprintf(newDisplayData.timeHHMM, HHMM_ARR_SIZE, "%d:%02d", rtc.hour(), rtc.minute());
  // :SS
  extern byte second;
  snprintf(newDisplayData.timeSS, SS_ARR_SIZE, ":%02d", second);
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
  // Mon dd Day
  snprintf(newDisplayData.dateStr, DATE_ARR_SIZE, "%s %d %s", months_table[rtc.month() - 1], rtc.day(), days_table[rtc.dayOfWeek() - 1]);
  extern bool alarmOn;
  if(alarmOn)
    snprintf(newDisplayData.alarmStr, ALARM_ARR_SIZE, " 7:00 AM");
  else
    snprintf(newDisplayData.alarmStr, ALARM_ARR_SIZE, "%s %s", alarmLabel, offLabel);
  newDisplayData._alarmOn = alarmOn;
}

void rgb_display_class::serialPrintRtcDateTime() {
  // full serial print time date day array
  Serial.print(newDisplayData.timeHHMM);
  // snprintf(timeArraySec, SS_ARR_SIZE, ":%02d", second);
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
