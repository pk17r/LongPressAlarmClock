#include "rgb_display.h"
#include "alarm_clock.h"
#include "rtc.h"

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
  tft.setRotation(1);

#elif defined(DISPLAY_IS_ST7735)

  // Use this initializer if using a 1.8" ST7735 TFT screen:
  tft.initR(INITR_BLACKTAB);  // Init ST7735 chip, black tab
  // set col and row offset of display for ST7735S
  tft.setColRowStart(2, 1);
  // make display landscape orientation
  tft.setRotation(1);

#elif defined(DISPLAY_IS_ILI9341)

  // Use this initializer if using a 1.8" ILI9341 TFT screen:
  tft.begin();
  // make display landscape orientation
  tft.setRotation(1);

#elif defined(DISPLAY_IS_ILI9488)

  tft.begin();
  tft.setRotation(tft.getRotation() + 2);
  int16_t x, y;
  tft.getOrigin(&x, &y);
  x = 0, y = 0;
  tft.setOrigin(x, y);
  tft.setClipRect();
  tft.setFontAdafruit();
  tft.invertDisplay(true);

#endif

  // SPI speed defaults to SPI_DEFAULT_FREQ defined in the library, you can override it here
  // Note that speed allowable depends on chip and quality of wiring, if you go too fast, you
  // may end up with a black screen some times, or all the time.
  // tft.setSPISpeed(80000000);

  // clear screen
  tft.fillScreen(kDisplayColorBlack);
  tft.setTextWrap(false);

  // update TFT display
  DisplayTimeUpdate();

  // set display brightness based on time of day
  CheckTimeAndSetBrightness();

  PrintLn("Display Initialized!");
}

// set display brightness function
void RGBDisplay::SetBrightness(int brightness) {
  if(current_brightness_ != brightness) {
    analogWrite(TFT_BL, brightness);
    PrintLn("Display Brightness set to ", brightness);
  }
  current_brightness_ = brightness;
  show_colored_edge_screensaver_ = (brightness >= kEveningBrightness);
}

void RGBDisplay::SetMaxBrightness() {
  if(current_brightness_ != kMaxBrightness)
    SetBrightness(kMaxBrightness);
}

void RGBDisplay::CheckTimeAndSetBrightness() {
  // check if RTC is good
  if(rtc->year() < 2024) {
    // RTC Time is not set!
    // Keep screen on day brightness
    SetBrightness(kDayBrightness);
  }
  else {
    // check time of the day and set brightness
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
