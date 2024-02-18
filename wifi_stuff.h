#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "common.h"
// #include "secrets.h"

class WiFiStuff {

public:
  void retrieveWiFiDetails();
  void saveWiFiDetails();
  void turn_WiFi_On();
  void turn_WiFi_Off();
  void getTodaysWeatherInfo();

  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID and uncomment the include statement at top of this file
    char* wifi_ssid = MY_WIFI_SSID;
  #else
    char* wifi_ssid = NULL;
  #endif
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD and uncomment the include statement at top of this file
    char* wifi_password = MY_WIFI_PASSWD;
  #else
    char* wifi_password = NULL;
  #endif

  // weather information
  char* weather_main = NULL;
  char* weather_description = NULL;
  char* weather_temp = NULL;
  char* weather_temp_max = NULL;
  char* weather_temp_min = NULL;
  char* weather_wind_speed = NULL;
  char* weather_humidity = NULL;

  bool gotWeatherInfo = false;

  bool tempInCnotF = false;


// PRIVATE VARIABLES
private:


};

#endif  // WIFI_STUFF_H