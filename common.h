#ifndef COMMON_H
#define COMMON_H

// forward decleration of other classes
class RGBDisplay;
class AlarmClock;

// extern all global variables
extern RGBDisplay* display;
extern AlarmClock* alarmClock;
extern int freeRam();

#endif // COMMON_H