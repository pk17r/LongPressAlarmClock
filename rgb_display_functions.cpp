#include "pin_defs.h"
#include "rgb_display_class.h"
#include "alarm_clock.h"
#include "uRTCLib.h"
#include <Arduino.h>

// constructor
rgb_display_class::rgb_display_class() {
  newDisplayData.timeHHMM = new char[HHMM_ARR_SIZE];
  newDisplayData.timeSS = new char[SS_ARR_SIZE];
  newDisplayData.dateStr = new char[DATE_ARR_SIZE];
  newDisplayData.alarmStr = new char[ALARM_ARR_SIZE];
  displayedData.timeHHMM = new char[HHMM_ARR_SIZE];
  displayedData.timeSS = new char[SS_ARR_SIZE];
  displayedData.dateStr = new char[DATE_ARR_SIZE];
  displayedData.alarmStr = new char[ALARM_ARR_SIZE];
  newDisplayData.timeHHMM[0] = '\0';
  newDisplayData.timeSS[0] = '\0';
  newDisplayData.dateStr[0] = '\0';
  newDisplayData.alarmStr[0] = '\0';
  displayedData.timeHHMM[0] = '\0';
  displayedData.timeSS[0] = '\0';
  displayedData.dateStr[0] = '\0';
  displayedData.alarmStr[0] = '\0';
};

// destructor
rgb_display_class::~rgb_display_class() {
  delete newDisplayData.timeHHMM;
  delete newDisplayData.timeSS;
  delete newDisplayData.dateStr;
  delete newDisplayData.alarmStr;
  delete displayedData.timeHHMM;
  delete displayedData.timeSS;
  delete displayedData.dateStr;
  delete displayedData.alarmStr;
}

void rgb_display_class::setup(AlarmClock* main_ptr) {
  // friend class pointer
  this->main = main_ptr;

  /* INITIALIZE DISPLAYS */

  // tft display backlight control PWM output pin
  pinMode(TFT_BL, OUTPUT);

#if defined(DISPLAY_IS_ST7789V)

  // OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
  // tft.init(TFT_HEIGHT, TFT_WIDTH);           // Init ST7789 320x240
  uint32_t SPI_Speed = 80000000;
  tft.init(TFT_HEIGHT, TFT_WIDTH);           // Init ST7789 320x240
  tft.setSPISpeed(SPI_Speed);
  tft.invertDisplay(false);
  // make display landscape orientation
  tft.setRotation(3);

#elif defined(DISPLAY_IS_ST7735)

  // Use this initializer if using a 1.8" ST7735 TFT screen:
  tft.initR(INITR_BLACKTAB);  // Init ST7735 chip, black tab
  // set col and row offset of display for ST7735S
  tft.setColRowStart(2, 1);

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

  // SPI speed defaults to SPI_DEFAULT_FREQ defined in the library, you can override it here
  // Note that speed allowable depends on chip and quality of wiring, if you go too fast, you
  // may end up with a black screen some times, or all the time.
  // tft.setSPISpeed(80000000);

  // clear screen
  tft.fillScreen(Display_Color_Black);
  tft.setTextWrap(false);

  unsigned long seed = (((((main->rtc.year() + 2000) * 12 + main->rtc.month()) * 30 + main->rtc.day()) * 24 + main->rtc.hour()) * 60 + main->rtc.minute()) * 60 + main->rtc.second();
  randomSeed(seed);

  Serial.println(F("Display Initialized"));
}

// set display brightness function
void rgb_display_class::setBrightness(int brightness) {
  analogWrite(TFT_BL, brightness);
  Serial.print(F("Display Brightness set to "));
  Serial.println(brightness);
  current_brightness = brightness;
}

void rgb_display_class::setMaxBrightness() {
  if(current_brightness != MAX_BRIGHTNESS)
    setBrightness(MAX_BRIGHTNESS);
}

void rgb_display_class::checkTimeAndSetBrightness() {
  if (main->rtc.hourModeAndAmPm() == 1) {  // 12hr AM
    if (main->rtc.hour() < 7 || main->rtc.hour() == 12)
      setBrightness(NIGHT_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  } else if (main->rtc.hourModeAndAmPm() == 2) {  // 12hr PM
    if (main->rtc.hour() > 10 && main->rtc.hour() != 12)
      setBrightness(NIGHT_BRIGHTNESS);
    // else if(rtc.hour() > 6)
    //   setBrightness(EVENING_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  } else if (main->rtc.hourModeAndAmPm() == 0) {  // 24hr
    if (main->rtc.hour() > 21 || main->rtc.hour() < 7)
      setBrightness(NIGHT_BRIGHTNESS);
    // else if(rtc.hour() > 18)
    //   setBrightness(EVENING_BRIGHTNESS);
    else
      setBrightness(DAY_BRIGHTNESS);
  }
}

void rgb_display_class::updateSecondsOnTimeStrArr(uint8_t &second) {
  snprintf(newDisplayData.timeSS, SS_ARR_SIZE, ":%02d", second);
}

void rgb_display_class::screensaverControl(bool turnOn) {
  if(!turnOn && myCanvas != NULL) {
    // delete screensaverCanvas;
    delete myCanvas;
    myCanvas = NULL;
  }
  else
    refreshScreensaverCanvas = true;
  // clear screen
  tft.fillScreen(Display_Color_Black);
  tft_HHMM_x0 = 20;
  tft_HHMM_y0 = 20;
  redrawDisplay = true;
  prepareTimeDayDateArrays();
}

void rgb_display_class::prepareTimeDayDateArrays() {
  // HH:MM
  snprintf(newDisplayData.timeHHMM, HHMM_ARR_SIZE, "%d:%02d", main->rtc.hour(), main->rtc.minute());
  // :SS
  snprintf(newDisplayData.timeSS, SS_ARR_SIZE, ":%02d", main->second);
  if(main->rtc.hourModeAndAmPm() == 0)
    newDisplayData._12hourMode = false;
  else if(main->rtc.hourModeAndAmPm() == 1) {
    newDisplayData._12hourMode = true;
    newDisplayData._pmNotAm = false;
  }
  else {
    newDisplayData._12hourMode = true;
    newDisplayData._pmNotAm = true;
  }
  // Mon dd Day
  snprintf(newDisplayData.dateStr, DATE_ARR_SIZE, "%s  %d  %s", days_table[main->rtc.dayOfWeek() - 1], main->rtc.day(), months_table[main->rtc.month() - 1]);
  if(main->alarmOn)
    snprintf(newDisplayData.alarmStr, ALARM_ARR_SIZE, "%d:%02d %s", main->alarmHr, main->alarmMin, (main->alarmIsAm ? amLabel : pmLabel));
  else
    snprintf(newDisplayData.alarmStr, ALARM_ARR_SIZE, "%s %s", alarmLabel, offLabel);
  newDisplayData._alarmOn = main->alarmOn;
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
  Serial.print(main->rtc.year() + 2000);
  Serial.print(charSpace);
  Serial.print(newDisplayData.alarmStr);
  Serial.flush();
}
