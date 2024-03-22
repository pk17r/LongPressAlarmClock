#include <string>
#include <sys/_stdint.h>
#ifndef EEPROM_H
#define EEPROM_H

#include "common.h"
#include "uEEPROMLib.h"
#include "secrets.h"

class EEPROM {

public:
  EEPROM();
  void RetrieveLongPressSeconds(uint8_t &long_press_seconds);
  void SaveLongPressSeconds(uint8_t long_press_seconds);
  void RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn);
  void SaveAlarm(uint8_t alarmHr, uint8_t alarmMin, bool alarmIsAm, bool alarmOn);
  void RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password);
  void SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password);
  void RetrieveWeatherLocationDetails(uint32_t &location_zip_code, std::string &location_country_code, bool &weather_units_metric_not_imperial);
  void SaveWeatherLocationDetails(uint32_t location_zip_code, std::string location_country_code, bool weather_units_metric_not_imperial);
  void SaveWeatherUnits(bool weather_units_metric_not_imperial);
  void RetrieveSavedFirmwareVersion(std::string &savedFirmwareVersion);
  void SaveCurrentFirmwareVersion();
  uint32_t RetrieveSavedCpuSpeed();
  void SaveCpuSpeed();
  bool RetrieveScreensaverBounceNotFlyHorizontally();
  void SaveScreensaverBounceNotFlyHorizontally(bool screensaverBounceNotFlyHorizontally);

private:

  void SaveDefaults();
  std::string FetchString(uint16_t length_address, uint16_t data_address);
  void SaveString(uint16_t length_address, uint8_t length_max, uint16_t data_address, std::string text);
  uint32_t Fetch4Bytes(uint16_t address);
  void Save4Bytes(uint16_t address, uint32_t value);
  // last leg of interaction with EEPROM
  uint8_t Fetch1Byte(uint16_t address);
  // last leg of interaction with EEPROM
  void Save1Byte(uint16_t address, uint8_t value);


  // uEEPROMLib eeprom for AT24C32 EEPROM
  uEEPROMLib eeprom_;


  // ADDRESSES

  // Address are uint16_t (2 byte sized)
  // Each address has 1 byte (uint8_t) of data

  const uint8_t kDataModelVersion = 101;    //  data model version : if this is not there on EEPROM, then defaults will be saved.
  const uint16_t kDataModelVersionAddress = 0;  // data model address
  const uint16_t kAlarmHrAddress = 1;
  const uint16_t kAlarmMinAddress = 2;
  const uint16_t kAlarmIsAmAddress = 3;
  const uint16_t kAlarmOnAddress = 4;
  const uint16_t kWiFiSsidLengthAddress = 5;
  const uint8_t kWiFiSsidLengthMax = kWifiSsidPasswordLengthMax;
  const uint16_t kWiFiSsidAddress = 6;  // kWifiSsidPasswordLengthMax bytes
  const uint16_t kWiFiPasswdLengthAddress = 38;
  const uint8_t kWiFiPasswdLengthMax = kWifiSsidPasswordLengthMax;
  const uint16_t kWiFiPasswdAddress = 39; // kWifiSsidPasswordLengthMax bytes
  const uint16_t kWeatherZipCodeAddress = 71;
  const uint8_t kWeatherZipCodeBytes = 4;
  const uint16_t kWeatherCountryCodeAddress = 75;
  const uint8_t kWeatherCountryCodeBytes = 2;
  const uint16_t kWeatherUnitsMetricNotImperialAddress = 77;
  const uint16_t kAlarmLongPressSecondsAddress = 79;
  const uint16_t kFirmwareVersionLengthAddress = 80;
  const uint8_t kFirmwareVersionLengthMax = 6;
  const uint16_t kFirmwareVersionAddress = 81;  // 6 bytes
  const uint16_t kCpuSpeedMhzAddress = 87;  // 4 bytes
  const uint16_t kScreensaverMotionTypeAddress = 91;  // 1 byte


  // DEFAULTS

  const uint8_t kAlarmHr = 7;
  const uint8_t kAlarmMin = 30;
  const bool kAlarmIsAm = true;
  const bool kAlarmOn = false;
  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
    std::string kWiFiSsid = MY_WIFI_SSID;
  #else
    std::string kWiFiSsid = "Enter SSID";
  #endif
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
    std::string kWiFiPasswd = MY_WIFI_PASSWD;
  #else
    std::string kWiFiPasswd = "Enter Passwd";
  #endif
  const uint32_t kWeatherZipCode = 92104;
  const std::string kWeatherCountryCode = "US";
  const bool kWeatherUnitsMetricNotImperial = false;
  const uint8_t kAlarmLongPressSeconds = 15;

};


#endif  // EEPROM_H