#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "pin_defs.h"
// #include "secrets.h"
#include "WiFi.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#if defined(MCU_IS_RASPBERRY_PI_PICO_W)
  #include <EEPROM.h>
#endif

class wifi_stuff {

public:
  void retrieveWiFiDetails();
  void saveWiFiDetails();
  void turn_WiFi_On();
  void turn_WiFi_Off();
  void getTodaysWeatherInfo();

  const unsigned int WIFI_SSID_PASSWORD_LENGTH_MAX = 32;
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


// PRIVATE VARIABLES
private:

  const unsigned int WIFI_ADDRESS_EEPROM = 5; // stores data in order 5 = wifi_ssid ending with \0 thereafter wifi_password ending with \0

};

#endif  // WIFI_STUFF_H