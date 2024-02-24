#include <sys/_stdint.h>
#include <string>
#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "common.h"
// #include "secrets.h"

class WiFiStuff {

public:
  WiFiStuff();
  void SaveWiFiDetails();
  void SaveWeatherLocationDetails();
  void TurnWiFiOn();
  void TurnWiFiOff();
  void GetTodaysWeatherInfo();
  bool GetTimeFromNtpServer();

  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
    std::string wifi_ssid_ = MY_WIFI_SSID;
  #else
    std::string wifi_ssid_ = "";
  #endif
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
    std::string wifi_password_ = MY_WIFI_PASSWD;
  #else
    std::string wifi_password_ = "";
  #endif

  // weather information
  std::string weather_main_ = "";
  std::string weather_description_ = "";
  std::string weather_temp_ = "";
  std::string weather_temp_feels_like_ = "";
  std::string weather_temp_max_ = "";
  std::string weather_temp_min_ = "";
  std::string weather_wind_speed_ = "";
  std::string weather_humidity_ = "";
  std::string city_ = "";
  int32_t gmt_offset_sec_ = 0;

  bool got_weather_info_ = false;   // whether weather information has been pulled
  unsigned long got_weather_info_time_ms = 0;

  unsigned long last_ntp_server_time_update_time_ms = 0;

  uint32_t location_zip_code_ = 92104;

  std::string location_country_code_ = "US";     // https://developer.accuweather.com/countries-by-region

  bool weather_units_metric_not_imperial_ = false;

// PRIVATE VARIABLES

private:

  void ConvertEpochIntoDate(unsigned long epoch_since_1970, int &today, int &month, int &year);

};

#endif  // WIFI_STUFF_H