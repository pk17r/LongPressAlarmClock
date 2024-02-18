#include "wifi_stuff.h"
#include "WiFi.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "eeprom.h"

void WiFiStuff::RetrieveWiFiDetails() {
  eeprom->RetrieveWiFiDetails(wifi_ssid_, wifi_password_);
}

void WiFiStuff::SaveWiFiDetails() {
  eeprom->SaveWiFiDetails(wifi_ssid_, wifi_password_);
}

void WiFiStuff::TurnWiFiOn() {
  Serial.println(F("Connecting to WiFi"));
  WiFi.persistent(true);
  delay(1);
  WiFi.begin(wifi_ssid_, wifi_password_);
  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    i++;
    if(i >= 10) break;
  }
  if(WiFi.status() == WL_CONNECTED)
    Serial.println(F("WiFi Connected."));
  else
    Serial.println(F("Could NOT connect to WiFi."));
}

void WiFiStuff::TurnWiFiOff() {
  WiFi.persistent(false);
  delay(1);
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  Serial.println(F("WiFi Off."));
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
  if(WiFi.status()== WL_CONNECTED){
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
    Serial.println("WiFi not connected");
  }

  // turn off WiFi
  TurnWiFiOff();
}