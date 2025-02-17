#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H
#include "common.h"
#define XPT2046_OPTION
#ifdef XPT2046_OPTION
#include <XPT2046_Touchscreen.h>
#endif
#include <TouchscreenResistive.h>

// struct declerations
struct TouchPixel {
  int16_t x;
  int16_t y;
  bool is_touched;
};

struct TouchCalibration {
  int16_t xMin;
  int16_t xMax;
  int16_t yMin;
  int16_t yMax;
  uint16_t screen_width;
  uint16_t screen_height;
};

class Touchscreen {

private:

// OBJECTS

  // store last touchpoint
  TouchPixel last_touch_Pixel_;

  // store last time touchscreen was polled
  unsigned long last_polled_millis_ = 0;

  // minimum time gap in milliseconds before polling touchscreen
  const unsigned short kPollingGapMs = 100;

  // store touchscreen calibration
  TouchCalibration touchscreen_calibration_;

  uint8_t touchscreen_type = 0;   // 0 = no touch, 1 = Resistive read using XPT2046, 2 = Resistive read using MCU ADC

#ifdef XPT2046_OPTION
  // Param 2 - Touch IRQ Pin - interrupt enabled polling
  // XPT2046_Touchscreen touchscreen_object_{ TS_CS_PIN, TS_IRQ_PIN };
  XPT2046_Touchscreen* touchscreen_ptr_ = NULL;
#endif
  // Direct touchscreen measurements using MCU ADC
  TouchscreenResistive* touchscreen_r_ptr_ = NULL;

// FUNCTIONS

public:

  // constructor
  Touchscreen();
  ~Touchscreen();

  // to know if touchscreen is touched
  bool IsTouched();

  // function to get x, y and isTouched flag
  TouchPixel* GetTouchedPixel();

  // flip touchscreen along horizontal axis -> top becomes bottom
  bool touchscreen_flip = false;

  void SetTouchscreenOrientation();
};

#endif  // TOUCHSCREEN_H