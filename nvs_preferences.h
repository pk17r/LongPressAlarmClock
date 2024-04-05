#ifndef NVS_PREFERENCES_H
#define NVS_PREFERENCES_H
/*
  Preferences is Arduino EEPROM replacement library using ESP32's On-Board Non-Volatile Memory
*/

#include <Preferences.h> //https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
#include "common.h"

class NvsPreferences {

public:

NvsPreferences();
void getOrSetDefaultEEPROMparams();
void setEEPROMparams();

unsigned int data_version = 0;
const unsigned int kDataVersion = 101;

private:

// ESP32 EEPROM Data Access
Preferences preferences;
const char* kNvsDataKey = "longPressData";
const char* kDataVersionKey = "kDataVersion";

};

#endif  // NVS_PREFERENCES_H