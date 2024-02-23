#include "wifi_stuff.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "eeprom.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "rtc.h"

WiFiStuff::WiFiStuff() {

  RetrieveWiFiDetails();

  TurnWiFiOff();

  PrintLn("WiFiStuff Initialized!");
}

void WiFiStuff::RetrieveWiFiDetails() {
  eeprom->RetrieveWiFiDetails(wifi_ssid_, wifi_password_);
}

void WiFiStuff::SaveWiFiDetails() {
  eeprom->SaveWiFiDetails(wifi_ssid_, wifi_password_);
}

void WiFiStuff::TurnWiFiOn() {
  PrintLn("Connecting to WiFi");
  WiFi.persistent(true);
  delay(1);
  WiFi.mode(WIFI_STA);
  delay(1);
  WiFi.begin(wifi_ssid_.c_str(), wifi_password_.c_str());
  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.print(".");
    i++;
    if(i >= 10) break;
  }
  if(WiFi.status() == WL_CONNECTED)
    PrintLn("WiFi Connected.");
  else
    PrintLn("Could NOT connect to WiFi.");
}

void WiFiStuff::TurnWiFiOff() {
  WiFi.persistent(false);
  delay(1);
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  PrintLn("WiFi Off.");
}

void WiFiStuff::GetTodaysWeatherInfo() {
  got_weather_info_ = false;

  // turn On Wifi
  TurnWiFiOn();

  // Your Domain name with URL path or IP address with path
  String openWeatherMapApiKey = "0fad3740b3a6b502ad57504f6fc3521e";

  // Replace with your country code and city
  String city = "San%20Diego";
  String countryCode = "840";

  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED) {
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=imperial";

    WiFiClient client;
    HTTPClient http;
      
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    
    String jsonBuffer = "{}"; 
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      jsonBuffer = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
    
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined" || httpResponseCode <= 0) {
      Serial.println("Parsing input failed!");
    }
    else 
    {
      // got response
      got_weather_info_ = true;
      got_weather_info_time_ms = millis();

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Weather: ");
      {
      String sss = myObject["weather"][0]["main"];
      if(weather_main_ != NULL) { delete weather_main_; weather_main_ = NULL;}
      weather_main_ = new char[sss.length()];
      strcpy(weather_main_, sss.c_str());
      } {
      String sss = myObject["weather"][0]["description"];
      if(weather_description_ != NULL) { delete weather_description_; weather_description_ = NULL;}
      weather_description_ = new char[sss.length()];
      strcpy(weather_description_, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp"]);
      if(weather_temp_ != NULL) { delete weather_temp_; weather_temp_ = NULL;}
      weather_temp_ = new char[sss.length()];
      strcpy(weather_temp_, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp_max"]);
      if(weather_temp_max_ != NULL) { delete weather_temp_max_; weather_temp_max_ = NULL;}
      weather_temp_max_ = new char[sss.length()];
      strcpy(weather_temp_max_, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp_min"]);
      if(weather_temp_min_ != NULL) { delete weather_temp_min_; weather_temp_min_ = NULL;}
      weather_temp_min_ = new char[sss.length()];
      strcpy(weather_temp_min_, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["wind"]["speed"]);
      if(weather_wind_speed_ != NULL) { delete weather_wind_speed_; weather_wind_speed_ = NULL;}
      weather_wind_speed_ = new char[sss.length()];
      strcpy(weather_wind_speed_, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["humidity"]);
      if(weather_humidity_ != NULL) { delete weather_humidity_; weather_humidity_ = NULL;}
      weather_humidity_ = new char[sss.length()];
      strcpy(weather_humidity_, sss.c_str());
      }
      Serial.print("weather_main "); Serial.println(weather_main_);
      Serial.print("weather_description "); Serial.println(weather_description_);
      Serial.print("weather_temp "); Serial.println(weather_temp_);
      Serial.print("weather_temp_max "); Serial.println(weather_temp_max_);
      Serial.print("weather_temp_min "); Serial.println(weather_temp_min_);
      Serial.print("weather_wind_speed "); Serial.println(weather_wind_speed_);
      Serial.print("weather_humidity "); Serial.println(weather_humidity_);

    }
  }
  else {
    PrintLn("WiFi not connected");
  }

  // turn off WiFi
  TurnWiFiOff();
}

bool WiFiStuff::GetTimeFromNtpServer() {

  bool returnVal = false;

  // turn On Wifi
  TurnWiFiOn();


  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED) {

    const char* NTP_SERVER = "pool.ntp.org";
    const long  GMT_OFFSET_SEC = -8*60*60;
    const int   DAYLIGHT_OFFSET_SEC = 60*60;

    // Define an NTP Client object
    WiFiUDP udpSocket;
    NTPClient ntpClient(udpSocket, NTP_SERVER, GMT_OFFSET_SEC);

    ntpClient.begin();
    returnVal = ntpClient.update();

    if(returnVal) {
      unsigned long epoch_since_1970 = ntpClient.getEpochTime();
      int hours = ntpClient.getHours();
      int minutes = ntpClient.getMinutes();
      int seconds = ntpClient.getSeconds();
      int dayOfWeekSunday0 = ntpClient.getDay();


      Serial.print("TIME FROM NTP SERVER:  Success="); Serial.println(returnVal);
      Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.print(seconds); Serial.print("  DoW: "); Serial.print(kDaysTable_[dayOfWeekSunday0]); Serial.print("  EpochTime: "); Serial.println(epoch_since_1970);

      int today, month, year;
      ConvertEpochIntoDate(epoch_since_1970, today, month, year);

      // RTC::SetRtcTimeAndDate(uint8_t second, uint8_t minute, uint8_t hour_24_hr_mode, uint8_t dayOfWeek_Sun_is_1, uint8_t day, uint8_t month_Jan_is_1, uint16_t year)
      rtc->SetRtcTimeAndDate(seconds, minutes, hours, dayOfWeekSunday0 + 1, today, month, year);
    }

    ntpClient.end();

  }
  else {
    Serial.println("WiFi not connected");
  }

  // // test
  // Serial.println();
  // Serial.print("Test Date:  8/20/2024   "); ConvertEpochIntoDate(1724195000);
  // Serial.print("Test Date:  10/20/2024   "); ConvertEpochIntoDate(1729385200);
  // Serial.print("Test Date:  10/10/2029   "); ConvertEpochIntoDate(1886367800);
  // Serial.print("Test Date:  3/1/2036   "); ConvertEpochIntoDate(2087942600);
  // Serial.print("Test Date:  12/31/2024   "); ConvertEpochIntoDate(1735603400);
  // Serial.print("Test Date:  1/1/2025   "); ConvertEpochIntoDate(1735689800);
  // Serial.print("Test Date:  1/31/2025   "); ConvertEpochIntoDate(1738281800);
  // Serial.print("Test Date:  3/1/2028   "); ConvertEpochIntoDate(1835481800);

  // turn off WiFi
  TurnWiFiOff();

  return returnVal;
}

void WiFiStuff::ConvertEpochIntoDate(unsigned long epoch_since_1970, int &today, int &month, int &year) {

  unsigned long epoch_Jan_1_2023_12_AM = 1704067200;
  float day = static_cast<float>(epoch_since_1970 - epoch_Jan_1_2023_12_AM) / (24*60*60);
  year = 2024;
  int monthJan0 = 0;
  // calculate year
  while(1) {
    if(day - 365 - (year % 4 == 0 ? 1 : 0) < 0)
      break;
    day -= 365 + (year % 4 == 0 ? 1 : 0);
    year++;
  }
  // calculate month
  // jan
  if(day - 31 > 0) {
    monthJan0++; day -= 31;
    // feb
    if(day - 28 - (year % 4 == 0 ? 1 : 0) > 0) {
      monthJan0++; day -= 28 + (year % 4 == 0 ? 1 : 0);
      // march
      if(day - 31 > 0) {
        monthJan0++; day -= 31;
        // apr
        if(day - 30 > 0) {
          monthJan0++; day -= 30;
          // may
          if(day - 31 > 0) {
            monthJan0++; day -= 31;
            // jun
            if(day - 30 > 0) {
              monthJan0++; day -= 30;
              // jul
              if(day - 31 > 0) {
                monthJan0++; day -= 31;
                // aug
                if(day - 31 > 0) {
                  monthJan0++; day -= 31;
                  // sep
                  if(day - 30 > 0) {
                    monthJan0++; day -= 30;
                    // oct
                    if(day - 31 > 0) {
                      monthJan0++; day -= 31;
                      // nov
                      if(day - 30 > 0) {
                        monthJan0++; day -= 30;
                        // dec
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  today = ceil(day);
  Serial.print(kMonthsTable[monthJan0]); Serial.print(" "); Serial.print(today); Serial.print(" "); Serial.println(year);
  month = monthJan0 + 1;
}
