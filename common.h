#ifndef COMMON_H
#define COMMON_H

#include "alarm_clock.h"
#include "rgb_display.h"

// forward decleration of other classes
class RGBDisplay;
class AlarmClock;

// extern all global variables
extern RGBDisplay* display;
extern AlarmClock* alarmClock;


#endif // COMMON_H