#include <string>
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
  #if defined(TOUCHSCREEN_IS_XPT2046)   // touchscreen version display has things rotated 180 deg
    tft.setRotation(3);
  #else
    tft.setRotation(1);
  #endif

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

  if(use_photodiode) {
    // configure Photoresistor pin
    pinMode(PHOTORESISTOR_PIN, INPUT);
    analogReadResolution(kAdcResolutionBits);

    // set display brightness
    CheckPhotoresistorAndSetBrightness();
  }
  else
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
  // if(debug_mode)
  //   RealTimeOnScreenOutput(std::to_string(brightness), 50);
  current_brightness_ = brightness;
  if(use_photodiode) {
    // hysteresis in background color on / off
    if(!show_colored_edge_screensaver_ && brightness >= kBrightnessBackgroundColorThreshold + 5)
      show_colored_edge_screensaver_ = true;
    else if(show_colored_edge_screensaver_ && brightness <= kBrightnessBackgroundColorThreshold - 5)
      show_colored_edge_screensaver_ = false;
  }
  else
    show_colored_edge_screensaver_ = (brightness >= kEveningBrightness);
}

void RGBDisplay::SetMaxBrightness() {
  if(current_brightness_ != kMaxBrightness)
    SetBrightness(kMaxBrightness);
}

void RGBDisplay::CheckPhotoresistorAndSetBrightness() {
  int photodiode_light_raw = analogRead(PHOTORESISTOR_PIN);
  // int lcd_brightness_val = max(photodiode_light_raw * kBrightnessInactiveMax / kPhotodiodeLightRawMax, 1);
  int lcd_brightness_val2 = max((int)map(photodiode_light_raw, 0.2 / 3.3 * kPhotodiodeLightRawMax, kPhotodiodeLightRawMax, 1, kBrightnessInactiveMax), 1);
  if(debug_mode)
    Serial.printf("photodiode_light_raw = %d, lcd_brightness_val2 = %d\n", photodiode_light_raw, lcd_brightness_val2);
  SetBrightness(lcd_brightness_val2);
}

void RGBDisplay::CheckTimeAndSetBrightness() {
  // check if RTC is good
  if(rtc->year() < 2024) {
    // RTC Time is not set!
    // Keep screen on day brightness
    SetBrightness(kDayBrightness);
  }
  else {
    if(rtc->todays_minutes > kNightTimeMinutes)
      SetBrightness(kNightBrightness);
    else if(rtc->todays_minutes > kEveningTimeMinutes)
      SetBrightness(kEveningBrightness);
    else if(rtc->todays_minutes > kDayTimeMinutes)
      SetBrightness(kDayBrightness);
    else
      SetBrightness(kNightBrightness);
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
