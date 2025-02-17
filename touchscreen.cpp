#include "touchscreen.h"
#include <SPI.h>
#include "rgb_display.h"
#include "nvs_preferences.h"

Touchscreen::Touchscreen() {
  touchscreen_type = nvs_preferences->RetrieveTouchscreenType();
  // PrintLn("touchscreen_type = ", touchscreen_type);

  if(touchscreen_type == 1) {   // XPT2046
    touchscreen_ptr_ = new XPT2046_Touchscreen(TS_CS_PIN, TS_IRQ_PIN);
    touchscreen_ptr_->begin(*spi_obj);
    touchscreen_calibration_ = TouchCalibration{220, 3800, 280, 3830, kTftWidth, kTftHeight};
  }
  else if(touchscreen_type == 2) {       // MCU ADC
    touchscreen_r_ptr_ = new TouchscreenResistive(TOUCHSCREEN_XP, TOUCHSCREEN_XM, TOUCHSCREEN_YP, TOUCHSCREEN_YM, 310);
    analogReadResolution(kAdcResolutionBits);
    touchscreen_r_ptr_->setAdcResolutionAndThreshold(kAdcResolutionBits);
    touchscreen_calibration_ = TouchCalibration{150, 930, 150, 870, kTftWidth, kTftHeight};
  }

  SetTouchscreenOrientation();
  touchscreen_flip = nvs_preferences->RetrieveTouchscreenFlip();

  PrintLn("Touchscreen Initialized!");
}

Touchscreen::~Touchscreen() {
  if(touchscreen_ptr_ != NULL) {
    delete touchscreen_ptr_;
    // PrintLn("deleted touchscreen_ptr_");
  }
  if(touchscreen_r_ptr_ != NULL) {
    delete touchscreen_r_ptr_;
    // PrintLn("deleted touchscreen_r_ptr_");
  }
}

void Touchscreen::SetTouchscreenOrientation() {
  if(touchscreen_type == 1) {
    if(display->screen_orientation_ == 1)
      touchscreen_ptr_->setRotation(3);
    else
      touchscreen_ptr_->setRotation(1);
  }
  else if(touchscreen_type == 2) {
    if(display->screen_orientation_ == 1)
      touchscreen_r_ptr_->setRotation(3);
    else
      touchscreen_r_ptr_->setRotation(1);
  }
}

bool Touchscreen::IsTouched() {
  if(touchscreen_type == 1) {   // XPT2046
    // irq touch is super fast
    if(!touchscreen_ptr_->tirqTouched())
      return false;
  }

  if(inactivity_millis < kUserInputDelayMs) {
    // always read touch point if user is already engaged
    GetTouchedPixel();
    return last_touch_Pixel_.is_touched;
  }
  else if((inactivity_millis >= kUserInputDelayMs) && (millis() - last_polled_millis_ <= kPollingGapMs)) {
    // low power mode: if user is not engaged
    return last_touch_Pixel_.is_touched;
  }
  else {
    // read touch, if touched then read touch point
    if(touchscreen_type == 2) {       // MCU ADC
      last_touch_Pixel_.is_touched = touchscreen_r_ptr_->touched();
      if(last_touch_Pixel_.is_touched)
        GetTouchedPixel();
    }
    else {    // if(touchscreen_type == 1)  // XPT2046
      // when irq touch is triggered, it takes a few hundred milliseconds to turn off
      // during this time we will poll touch screen pressure using SPI to know if touchscreen is pressed or not
      GetTouchedPixel();
    }
    return last_touch_Pixel_.is_touched;
  }
}

TouchPixel* Touchscreen::GetTouchedPixel() {
  if(millis() - last_polled_millis_ <= kPollingGapMs) {
    // return last touch point
  }
  else {
    // note polling time
    last_polled_millis_ = millis();

    int16_t x = -1, y = -1, z = -1;
    if(touchscreen_type == 2) {
      // get touch point using MCU ADC
      TsPoint touch = touchscreen_r_ptr_->getPoint();
      x = touch.x;
      y = touch.y;
      z = touch.z;
    }
    else {    //if(touchscreen_type == 1)
      // get touch point from XPT2046
      TS_Point touch = touchscreen_ptr_->getPoint();
      x = touch.x;
      y = touch.y;
      z = touch.z;
    }

    if(z < (touchscreen_type == 1 ? 100 : 1)) {
      last_touch_Pixel_ = TouchPixel{-1, -1, false};
      return &last_touch_Pixel_;
    }

    // map
    x = max(min((int16_t)map(x, touchscreen_calibration_.xMin, touchscreen_calibration_.xMax, 0, touchscreen_calibration_.screen_width - 1), (int16_t)(touchscreen_calibration_.screen_width - 1)), (int16_t)0);
    y = max(min((int16_t)map(y, touchscreen_calibration_.yMin, touchscreen_calibration_.yMax, 0, touchscreen_calibration_.screen_height), (int16_t)(touchscreen_calibration_.screen_height - 1)), (int16_t)0);

    if(touchscreen_flip)
      y = touchscreen_calibration_.screen_height - y;

    last_touch_Pixel_ = TouchPixel{x, y, true};
  }
  return &last_touch_Pixel_;
}
