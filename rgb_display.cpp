#include "rgb_display.h"
#include "alarm_clock.h"
#include "rtc.h"

// constructor
RGBDisplay::RGBDisplay() {
  new_display_data_.time_HHMM = new char[kHHMM_ArraySize];
  new_display_data_.time_SS = new char[kSS_ArraySize];
  new_display_data_.date_str = new char[kDateArraySize];
  new_display_data_.alarm_str = new char[kAlarmArraySize];
  displayed_data_.time_HHMM = new char[kHHMM_ArraySize];
  displayed_data_.time_SS = new char[kSS_ArraySize];
  displayed_data_.date_str = new char[kDateArraySize];
  displayed_data_.alarm_str = new char[kAlarmArraySize];
  new_display_data_.time_HHMM[0] = '\0';
  new_display_data_.time_SS[0] = '\0';
  new_display_data_.date_str[0] = '\0';
  new_display_data_.alarm_str[0] = '\0';
  displayed_data_.time_HHMM[0] = '\0';
  displayed_data_.time_SS[0] = '\0';
  displayed_data_.date_str[0] = '\0';
  displayed_data_.alarm_str[0] = '\0';
};

// destructor
RGBDisplay::~RGBDisplay() {
  delete new_display_data_.time_HHMM;
  delete new_display_data_.time_SS;
  delete new_display_data_.date_str;
  delete new_display_data_.alarm_str;
  delete displayed_data_.time_HHMM;
  delete displayed_data_.time_SS;
  delete displayed_data_.date_str;
  delete displayed_data_.alarm_str;
}

void RGBDisplay::Setup() {

  /* INITIALIZE DISPLAYS */

  // tft display backlight control PWM output pin
  pinMode(TFT_BL, OUTPUT);

#if defined(DISPLAY_IS_ST7789V)

  // OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
  // tft.init(TFT_HEIGHT, TFT_WIDTH);           // Init ST7789 320x240
  uint32_t SPI_Speed = 80000000;
  tft.init(kTftHeight, kTftWidth);           // Init ST7789 320x240
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
  tft.fillScreen(kDisplayColorBlack);
  tft.setTextWrap(false);

  unsigned long seed = (((((rtc->year()) * 12 + rtc->month()) * 30 + rtc->day()) * 24 + rtc->hour()) * 60 + rtc->minute()) * 60 + rtc->second();
  randomSeed(seed);

  // prepare date and time arrays and serial print RTC Date Time
  display->PrepareTimeDayDateArrays();

  // serial print RTC Date Time
  display->SerialPrintRtcDateTime();

  // update TFT display
  display->DisplayTimeUpdate();

  // set display brightness based on time of day
  display->CheckTimeAndSetBrightness();

  Serial.println(F("Display Initialized"));
}

// set display brightness function
void RGBDisplay::SetBrightness(int brightness) {
  analogWrite(TFT_BL, brightness);
  Serial.print(F("Display Brightness set to ")); Serial.print(brightness); Serial.print(kCharSpace);
  current_brightness_ = brightness;
  show_colored_edge_screensaver_ = (brightness >= kEveningBrightness);
}

void RGBDisplay::SetMaxBrightness() {
  if(current_brightness_ != kMaxBrightness)
    SetBrightness(kMaxBrightness);
}

void RGBDisplay::CheckTimeAndSetBrightness() {
  if (rtc->hourModeAndAmPm() == 1) {  // 12hr AM
    if (rtc->hour() < 6 || rtc->hour() == 12)
      SetBrightness(kNightBrightness);
    else
      SetBrightness(kDayBrightness);
  } else if (rtc->hourModeAndAmPm() == 2) {  // 12hr PM
    if (rtc->hour() > 10 && rtc->hour() != 12)
      SetBrightness(kNightBrightness);
    else if(rtc->hour() >= 6)
      SetBrightness(kEveningBrightness);
    else
      SetBrightness(kDayBrightness);
  } else if (rtc->hourModeAndAmPm() == 0) {  // 24hr
    if (rtc->hour() > 21 || rtc->hour() < 7)
      SetBrightness(kNightBrightness);
    else if(rtc->hour() > 18)
      SetBrightness(kEveningBrightness);
    else
      SetBrightness(kDayBrightness);
  }
}

void RGBDisplay::UpdateSecondsOnTimeStrArr(uint8_t second) {
  snprintf(new_display_data_.time_SS, kSS_ArraySize, ":%02d", second);
}

void RGBDisplay::ScreensaverControl(bool turnOn) {
  if(!turnOn && my_canvas_ != NULL) {
    // delete screensaverCanvas;
    delete my_canvas_;
    my_canvas_ = NULL;
  }
  else
    refresh_screensaver_canvas_ = true;
  // clear screen
  tft.fillScreen(kDisplayColorBlack);
  screensaver_x1_ = 0;
  screensaver_y1_ = 20;
  redraw_display_ = true;
  PrepareTimeDayDateArrays();
}

void RGBDisplay::PrepareTimeDayDateArrays() {
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
  snprintf(new_display_data_.date_str, kDateArraySize, "%s  %d  %s", days_table_[rtc->dayOfWeek() - 1], rtc->day(), months_table[rtc->month() - 1]);
  if(alarm_clock->alarm_ON_)
    snprintf(new_display_data_.alarm_str, kAlarmArraySize, "%d:%02d %s", alarm_clock->alarm_hr_, alarm_clock->alarm_min_, (alarm_clock->alarm_is_AM_ ? kAmLabel : kPmLabel));
  else
    snprintf(new_display_data_.alarm_str, kAlarmArraySize, "%s %s", kAlarmLabel, kOffLabel);
  new_display_data_.alarm_ON = alarm_clock->alarm_ON_;
}

void RGBDisplay::SerialPrintRtcDateTime() {
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
