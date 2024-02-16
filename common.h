#ifndef COMMON_H
#define COMMON_H

// forward decleration of other classes
class RGBDisplay;
class AlarmClock;
class WiFiStuff;
class EEPROM;
class PushButtonTaps;
class Touchscreen;

// extern all global variables
extern RGBDisplay* display;
extern AlarmClock* alarmClock;
extern WiFiStuff* wifiStuff;
extern EEPROM* eeprom;
extern PushButtonTaps pushBtn;
extern Touchscreen ts;

// extern all global functions
extern int freeRam();

#endif // COMMON_H