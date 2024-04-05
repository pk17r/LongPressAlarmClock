#ifndef NVS_PREFERENCES_H
#define NVS_PREFERENCES_H
/*
  Preferences is Arduino EEPROM replacement library using ESP32's On-Board Non-Volatile Memory
*/

#include <Preferences.h> //https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
#include "common.h"
#include "secrets.h"

class NvsPreferences {

public:

  NvsPreferences();
  void SaveDefaults();
  void SaveDataModelVersion();
  void PrintSavedData();

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
  void CopyFirmwareVersionFromEepromToNvs(std::string firmwareVersion);
  uint32_t RetrieveSavedCpuSpeed();
  void SaveCpuSpeed();
  void CopyCpuSpeedFromEepromToNvsMemory(uint32_t cpu_speed_mhz_eeprom);
  bool RetrieveScreensaverBounceNotFlyHorizontally();
  void SaveScreensaverBounceNotFlyHorizontally(bool screensaverBounceNotFlyHorizontally);

  unsigned int data_model_version = 0;
  const unsigned int kDataModelVersion = 101;

private:

  void RetrieveDataModelVersion();

  // ESP32 NVS Memory Data Access
  Preferences preferences;
  const char* kNvsDataKey = "longPressData";

  const char* kDataModelVersionKey = "kDataModelVer";

  const char* kAlarmHrKey = "AlarmHr";
  const uint8_t kAlarmHr = 7;

  const char* kAlarmMinKey = "AlarmMin";
  const uint8_t kAlarmMin = 30;

  const char* kAlarmIsAmKey = "AlarmIsAm";
  const bool kAlarmIsAm = true;

  const char* kAlarmOnKey = "AlarmOn";
  const bool kAlarmOn = false;

  const char* kWiFiSsidKey = "WiFiSsid";  // kWifiSsidPasswordLengthMax bytes
  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
    std::string kWiFiSsid = MY_WIFI_SSID;
  #else
    std::string kWiFiSsid = "Enter SSID";
  #endif

  const char* kWiFiPasswdKey = "WiFiPasswd"; // kWifiSsidPasswordLengthMax bytes
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
    std::string kWiFiPasswd = MY_WIFI_PASSWD;
  #else
    std::string kWiFiPasswd = "Enter Passwd";
  #endif

  const char* kWeatherZipCodeKey = "WeatherZip";
  const uint32_t kWeatherZipCode = 92104;

  const char* kWeatherCountryCodeKey = "WeatherCC";
  const std::string kWeatherCountryCode = "US";

  const char* kWeatherUnitsMetricNotImperialKey = "WeatherUnits";
  const bool kWeatherUnitsMetricNotImperial = false;

  const char* kAlarmLongPressSecondsKey = "AlarmLongPrsSec";
  const uint8_t kAlarmLongPressSeconds = 15;

  const char* kFirmwareVersionKey = "FwVersion";  // 6 bytes

  const char* kCpuSpeedMhzKey = "CpuSpeedMhz";  // 4 bytes

  const char* kScreensaverMotionTypeKey = "ScSvrMotionTy";  // 1 byte

};

#endif  // NVS_PREFERENCES_H