#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "common.h"
// #include "secrets.h"

class WiFiStuff {

public:
  WiFiStuff() { RetrieveWiFiDetails(); TurnWiFiOff(); }
  void RetrieveWiFiDetails();
  void SaveWiFiDetails();
  void TurnWiFiOn();
  void TurnWiFiOff();
  void GetTodaysWeatherInfo();
  bool GetTimeFromNtpServer();
  void ConvertEpochIntoDate(unsigned long epoch_since_1970, int &today, int &month, int &year);

  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
    char* wifi_ssid_ = MY_WIFI_SSID;
  #else
    char* wifi_ssid_ = NULL;
  #endif
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
    char* wifi_password_ = MY_WIFI_PASSWD;
  #else
    char* wifi_password_ = NULL;
  #endif

  // weather information
  char* weather_main_ = NULL;
  char* weather_description_ = NULL;
  char* weather_temp_ = NULL;
  char* weather_temp_max_ = NULL;
  char* weather_temp_min_ = NULL;
  char* weather_wind_speed_ = NULL;
  char* weather_humidity_ = NULL;

  bool got_weather_info_ = false;   // whether weather information has been pulled
  unsigned long got_weather_info_time_ms = 0;

  bool temp_in_C_not_F_ = false;

// PRIVATE VARIABLES
private:


};

#endif  // WIFI_STUFF_H