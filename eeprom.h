#ifndef EEPROM_H
#define EEPROM_H

#include "common.h"
#include "uEEPROMLib.h"

class EEPROM {

public:
  EEPROM();
  bool RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void SaveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password);
  void SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password);

private:

  // uEEPROMLib eeprom for AT24C32 EEPROM
  uEEPROMLib eeprom_;

  // byte 0 needs to be this to read further data. Otherwise default data will be saved to EEPROM
  const unsigned int kDataCompatibilityByte0Flag = 121;

  /** the address in the EEPROM **/
  const unsigned int kAlarmAddressEEPROM = 0; // stores data in order 0 = data is set, 1 = hr, 2 = min, 3 = isAm, 4 = alarmOn

  const unsigned int kWifiAddressEEPROM = 5; // stores data in order 5 = wifi_ssid ending with \0 thereafter wifi_password ending with \0

};


#endif  // EEPROM_H