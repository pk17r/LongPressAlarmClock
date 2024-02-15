#ifndef PERSISTENT_DATA_H
#define PERSISTENT_DATA_H

#include <Arduino.h>
#include "pin_defs.h"
#if defined(MCU_IS_TEENSY) || defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include <EEPROM.h>
#endif

class persistent_data {

public:

  bool retrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void saveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void retrieveWiFiDetails(char* wifi_ssid, char* wifi_password);
  void saveWiFiDetails(char* wifi_ssid, char* wifi_password);

  const unsigned int WIFI_SSID_PASSWORD_LENGTH_MAX = 32;

private:

  /** the address in the EEPROM **/
  const unsigned int ALARM_ADDRESS_EEPROM = 0; // stores data in order 0 = data is set, 1 = hr, 2 = min, 3 = isAm, 4 = alarmOn

  const unsigned int WIFI_ADDRESS_EEPROM = 5; // stores data in order 5 = wifi_ssid ending with \0 thereafter wifi_password ending with \0

};


#endif  // PERSISTENT_DATA_H