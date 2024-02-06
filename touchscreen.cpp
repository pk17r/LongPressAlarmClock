#include "touchscreen.h"
#include "pin_defs.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

bool touchscreen::isTouched() {
  // irq touch is super fast
  if(!touchscreenObj.tirqTouched())
    return false;
  // when irq touch is triggered, it takes a few hundred milliseconds to turn off
  // during this time we will poll touch screen pressure using SPI to know if touchscreen is pressed or not
  if(millis() - lastPolledMillis <= POLLING_GAP_MS) {
    return lastTouchPt.isTouched;
  }
  else {
    getTouchedPixel();
    return lastTouchPt.isTouched;
  }
}

void touchscreen::setupAndCalibrate(int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax, int16_t w, int16_t h) {
  touchscreenObj.begin(SPI);
  touchscreenObj.setRotation(1);
  touchscreenCal = TouchCalibration{xMin, xMax, yMin, yMax, w, h};
}

TouchPoint* touchscreen::getTouchedPixel() {
  if(millis() - lastPolledMillis <= POLLING_GAP_MS) {
    // return last touch point
  }
  else {
    // note polling time
    lastPolledMillis = millis();

    // get touch point from XPT2046
    TS_Point touch = touchscreenObj.getPoint();

    if(touch.z < 100) {
      lastTouchPt = TouchPoint{-1, -1, false};
      return &lastTouchPt;
    }
    
    // Rotate and map
    int16_t x = map(touch.x, touchscreenCal.xMin, touchscreenCal.xMax, 0, touchscreenCal.screenWidth);
    int16_t y = map(touch.y, touchscreenCal.yMin, touchscreenCal.yMax, 0, touchscreenCal.screenHeight);

    if (x > touchscreenCal.screenWidth){
        x = touchscreenCal.screenWidth;
    }
    if (x < 0) {
        x = 0;
    }
    if (y > touchscreenCal.screenHeight) {
        y = touchscreenCal.screenHeight;
    }
    if (y < 0) {
        y = 0;
    }
    lastTouchPt = TouchPoint{x, y, true};
  }
  return &lastTouchPt;
}
