#include <string>
#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "common.h"
// #include "secrets.h"
#include <sys/_stdint.h>      // try removing it, don't know why it is here

class WiFiStuff {

public:
  WiFiStuff();
  void SaveWiFiDetails();
  void SaveWeatherLocationDetails();
  void SaveWeatherUnits();
  bool TurnWiFiOn();
  void TurnWiFiOff();
  void GetTodaysWeatherInfo();
  bool GetTimeFromNtpServer();
  void StartSetWiFiSoftAP();
  void StopSetWiFiSoftAP();
  void StartSetLocationLocalServer();
  void StopSetLocationLocalServer();

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
  uint8_t get_weather_info_wait_seconds_ = 0;   // wait to delay weather info pulls
  unsigned long last_fetch_weather_info_time_ms_ = 0;
  const unsigned long kFetchWeatherInfoMinIntervalMs = 60*1000;    //  1 minute
  bool incorrect_zip_code = false;

  bool auto_updated_time_today_ = false;   // auto update time once every day at 2:01 AM
  bool manual_time_update_successful_ = false;   // flag used to know if manual time update fetch was success
  unsigned long last_ntp_server_time_update_time_ms = 0;

  uint32_t location_zip_code_ = 92104;

  std::string location_country_code_ = "US";     // https://developer.accuweather.com/countries-by-region

  bool weather_units_metric_not_imperial_ = false;

  volatile bool wifi_connected_ = false;

  // flag to stop trying auto connect to WiFi
  bool incorrect_wifi_details_ = false;

  std::string soft_AP_IP = "";

// PRIVATE VARIABLES

private:

  void ConvertEpochIntoDate(unsigned long epoch_since_1970, int &today, int &month, int &year);

};

#endif  // WIFI_STUFF_H