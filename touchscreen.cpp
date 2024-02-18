#include "touchscreen.h"
#include <SPI.h>
#include "rgb_display.h"

Touchscreen::Touchscreen() {
  touchscreenObj.begin(SPI);
  touchscreenObj.setRotation(1);
  touchscreenCal = TouchCalibration{220, 3800, 280, 3830, display->TFT_WIDTH, display->TFT_HEIGHT};
}

bool Touchscreen::isTouched() {
  // irq touch is super fast
  if(!touchscreenObj.tirqTouched())
    return false;
  // when irq touch is triggered, it takes a few hundred milliseconds to turn off
  // during this time we will poll touch screen pressure using SPI to know if touchscreen is pressed or not
  if(millis() - lastPolledMillis <= POLLING_GAP_MS) {
    return lastTouchPixel.isTouched;
  }
  else {
    getTouchedPixel();
    return lastTouchPixel.isTouched;
  }
}

TouchPixel* Touchscreen::getTouchedPixel() {
  if(millis() - lastPolledMillis <= POLLING_GAP_MS) {
    // return last touch point
  }
  else {
    // note polling time
    lastPolledMillis = millis();

    // get touch point from XPT2046
    TS_Point touch = touchscreenObj.getPoint();

    if(touch.z < 100) {
      lastTouchPixel = TouchPixel{-1, -1, false};
      return &lastTouchPixel;
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
    lastTouchPixel = TouchPixel{x, y, true};
  }
  return &lastTouchPixel;
}
