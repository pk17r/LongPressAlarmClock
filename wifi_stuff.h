#include <string>
#ifndef WIFI_STUFF_H
#define WIFI_STUFF_H

#include "common.h"
#include "secrets.h"
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
  void UpdateFirmware();
  bool FirmwareVersionCheck();

  #if defined(MY_WIFI_SSID)   // create a secrets.h file with #define for MY_WIFI_SSID
    std::string wifi_ssid_ = MY_WIFI_SSID;
  #else
    std::string wifi_ssid_ = "";
  #endif
  #if defined(MY_WIFI_PASSWD)   // create a secrets.h file with #define for MY_WIFI_PASSWD
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

  // flag to to know if new firmware update is available
  bool firmware_update_available_ = false;

  // DigiCert root certificate has expiry date of 10 Nov 2031
  // from https://github.com/programmer131/ESP8266_ESP32_SelfUpdate/tree/master
  const char * rootCACertificate = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
  "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
  "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
  "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
  "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
  "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
  "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
  "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
  "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
  "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
  "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
  "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
  "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
  "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
  "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
  "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
  "-----END CERTIFICATE-----\n";

  // bool to indicate whether Web OTA Update needs to be secure or insecure
  const bool use_secure_connection = false;

  // Web OTA Update https://github.com/programmer131/ESP8266_ESP32_SelfUpdate/tree/master
  // ESP32 WiFiClientSecure examples: WiFiClientInsecure.ino WiFiClientSecure.ino
  #define URL_fw_Version "https://raw.githubusercontent.com/pk17r/Long_Press_Alarm_Clock/main/configuration.h"
  #if defined(MCU_IS_ESP32_WROOM_DA_MODULE)
    #define URL_fw_Bin "https://raw.githubusercontent.com/pk17r/Long_Press_Alarm_Clock/main/build/esp32.esp32.esp32da/long_press_alarm_clock.ino.bin"
  #elif defined(MCU_IS_ESP32_S2_MINI)
    #define URL_fw_Bin "https://raw.githubusercontent.com/pk17r/Long_Press_Alarm_Clock/main/build/esp32.esp32.lolin_s2_mini/long_press_alarm_clock.ino.bin"
  #endif

// PRIVATE VARIABLES

private:

  void ConvertEpochIntoDate(unsigned long epoch_since_1970, int &today, int &month, int &year);

  #if defined(MY_OPEN_WEATHER_MAP_API_KEY)   // create a secrets.h file with #define for MY_OPEN_WEATHER_MAP_API_KEY
    std::string openWeatherMapApiKey = MY_OPEN_WEATHER_MAP_API_KEY;
  #else
    std::string openWeatherMapApiKey = "";
  #endif

};

#endif  // WIFI_STUFF_H