#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H
#include "common.h"
#include <XPT2046_Touchscreen.h>

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

  // Param 2 - Touch IRQ Pin - interrupt enabled polling
  // XPT2046_Touchscreen touchscreen_object_{ TS_CS_PIN, TS_IRQ_PIN };
  XPT2046_Touchscreen* touchscreen_ptr_ = NULL;

// FUNCTIONS

public:

  // constructor
  Touchscreen();
  // to know if touchscreen is touched
  bool IsTouched();
  // function to get x, y and isTouched flag
  TouchPixel* GetTouchedPixel();

};

#endif  // TOUCHSCREEN_H