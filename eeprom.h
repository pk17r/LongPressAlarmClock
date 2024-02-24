#include <sys/_stdint.h>
#ifndef EEPROM_H
#define EEPROM_H

#include "common.h"
#include "uEEPROMLib.h"

class EEPROM {

public:
  EEPROM();
  bool RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void SaveAlarm(uint8_t alarmHr, uint8_t alarmMin, bool alarmIsAm, bool alarmOn);
  void RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password);
  void SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password);

private:

  void SaveDefaults();
  std::string FetchString(uint16_t length_address, uint16_t data_address);
  void SaveString(uint16_t length_address, uint8_t length_max, uint16_t data_address, std::string text);
  uint8_t Fetch1Byte(uint16_t address);
  void Save1Byte(uint16_t address, uint8_t value);
  uint32_t Fetch4Bytes(uint16_t address);
  void Save4Bytes(uint16_t address, uint32_t value);

  // uEEPROMLib eeprom for AT24C32 EEPROM
  uEEPROMLib eeprom_;

  // byte 0 needs to be this to read further data. Otherwise default data will be saved to EEPROM
  const unsigned int kDataCompatibilityByte0Flag = 121;

  /** the address in the EEPROM **/
  const unsigned int kAlarmAddressEEPROM = 0; // stores data in order 0 = data is set, 1 = hr, 2 = min, 3 = isAm, 4 = alarmOn

  const unsigned int kWifiAddressEEPROM = 5; // stores data in order 5 = wifi_ssid ending with \0 thereafter wifi_password ending with \0


  // ADDRESSES

  const uint8_t kDataModelVersion = 101;
  const uint16_t kDataModelVersionAddress = 0;
  const uint16_t kAlarmHrAddress = 1;
  const uint16_t kAlarmMinAddress = 2;
  const uint16_t kAlarmIsAmAddress = 3;
  const uint16_t kAlarmOnAddress = 4;
  const uint8_t kWiFiSsidLengthAddress = 5;
  const uint8_t kWiFiSsidLengthMax = kWifiSsidPasswordLengthMax;
  const uint16_t kWiFiSsidAddress = 6;
  const uint16_t kWiFiPasswdLengthAddress = 38;
  const uint8_t kWiFiPasswdLengthMax = kWifiSsidPasswordLengthMax;
  const uint16_t kWiFiPasswdAddress = 39;
  const uint16_t kWeatherZipCodeAddress = 71;
  const uint8_t kWeatherZipCodeBytes = 4;
  const uint16_t kWeatherCountryCodeAddress = 75;
  const uint8_t kWeatherCountryCodeBytes = 2;
  const uint16_t kWeatherUnitsMetricNotImperialAddress = 77;


  // DEFAULTS

  const uint8_t kAlarmHr = 7;
  const uint8_t kAlarmMin = 30;
  const bool kAlarmIsAm = true;
  const bool kAlarmOn = false;
  const std::string kWiFiSsid = "Enter SSID";
  const std::string kWiFiPasswd = "Enter Passwd";
  const uint32_t kWeatherZipCode = 92104;
  const std::string kWeatherCountryCode = "US";
  const bool kWeatherUnitsMetricNotImperial = false;


};


#endif  // EEPROM_H