#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H
#include "pin_defs.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// struct declerations
struct TouchPoint {
  int16_t x = -1;
  int16_t y = -1;
  bool isTouched = false;
};

struct TouchCalibration {
  int16_t xMin;
  int16_t xMax;
  int16_t yMin;
  int16_t yMax;
  int16_t screenWidth;
  int16_t screenHeight;
};

class touchscreen {

private:

// OBJECTS

  // store last touchpoint
  TouchPoint lastTouchPt;

  // store last time touchscreen was polled
  unsigned long lastPolledMillis = 0;

  // minimum time gap in milliseconds before polling touchscreen
  const unsigned short POLLING_GAP_MS = 100;

  // store touchscreen calibration
  TouchCalibration touchscreenCal;

  // Param 2 - Touch IRQ Pin - interrupt enabled polling
  XPT2046_Touchscreen touchscreenObj;//(TS_CS_PIN, TS_IRQ_PIN);

// FUNCTIONS

public:

  // constructor
  touchscreen() : touchscreenObj(TS_CS_PIN, TS_IRQ_PIN) {};
  void setupAndCalibrate(int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax, int16_t w, int16_t h);
  bool isTouched();
  // function to get x, y and isTouched flag
  TouchPoint* getTouchedPixel();

};

#endif  // TOUCHSCREEN_H
