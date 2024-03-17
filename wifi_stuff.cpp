#include <string.h>
#include <string>
#include <cstddef>
#include "wifi_stuff.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "eeprom.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "rtc.h"
#if defined(MCU_IS_ESP32)
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  // Web OTA Update https://github.com/programmer131/ESP8266_ESP32_SelfUpdate/tree/master
  #include <HTTPUpdate.h>
  #include <WiFiClientSecure.h>
#endif

WiFiStuff::WiFiStuff() {

  eeprom->RetrieveWiFiDetails(wifi_ssid_, wifi_password_);

  eeprom->RetrieveWeatherLocationDetails(location_zip_code_, location_country_code_, weather_units_metric_not_imperial_);

  TurnWiFiOff();

  PrintLn("WiFiStuff Initialized!");
}

void WiFiStuff::SaveWiFiDetails() {
  eeprom->SaveWiFiDetails(wifi_ssid_, wifi_password_);
  incorrect_wifi_details_ = false;
}

void WiFiStuff::SaveWeatherLocationDetails() {
  eeprom->SaveWeatherLocationDetails(location_zip_code_, location_country_code_, weather_units_metric_not_imperial_);
  incorrect_zip_code = false;
}

void WiFiStuff::SaveWeatherUnits() {
  eeprom->SaveWeatherUnits(weather_units_metric_not_imperial_);
}

bool WiFiStuff::TurnWiFiOn() {

  PrintLn("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  delay(1);
  WiFi.persistent(true);
  delay(1);
  WiFi.begin(wifi_ssid_.c_str(), wifi_password_.c_str());
  int i = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
    i++;
    if(i >= 5) break;
  }
  if(WiFi.status() == WL_CONNECTED) {
    PrintLn("WiFi Connected.");
    digitalWrite(LED_BUILTIN, HIGH);
    wifi_connected_ = true;
    incorrect_wifi_details_ = false;
  }
  else {
    PrintLn("Could NOT connect to WiFi.");
    digitalWrite(LED_BUILTIN, LOW);
    wifi_connected_ = false;
    incorrect_wifi_details_ = true;
  }

  return wifi_connected_;
}

void WiFiStuff::TurnWiFiOff() {
  WiFi.persistent(false);
  delay(1);
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  PrintLn("WiFi Off.");
  digitalWrite(LED_BUILTIN, LOW);
  wifi_connected_ = false;
}

void WiFiStuff::GetTodaysWeatherInfo() {
  got_weather_info_ = false;

  // don't fetch frequently otherwise can get banned
  if(last_fetch_weather_info_time_ms_ != 0 && millis() - last_fetch_weather_info_time_ms_ < kFetchWeatherInfoMinIntervalMs) {
    get_weather_info_wait_seconds_ = (kFetchWeatherInfoMinIntervalMs - (millis() - last_fetch_weather_info_time_ms_)) / 1000;
    return;
  }
  get_weather_info_wait_seconds_ = 0;

  // turn On Wifi
  if(!wifi_connected_)
    if(!TurnWiFiOn())
      return;

  // Your Domain name with URL path or IP address with path
  // std::string openWeatherMapApiKey = 

  //https://api.openweathermap.org/data/2.5/weather?zip=92104,US&appid=
  //{"coord":{"lon":-117.1272,"lat":32.7454},"weather":[{"id":701,"main":"Mist","description":"mist","icon":"50n"}],"base":"stations","main":{"temp":284.81,"feels_like":284.41,"temp_min":283.18,"temp_max":286.57,"pressure":1020,"humidity":91},"visibility":10000,"wind":{"speed":3.09,"deg":0},"clouds":{"all":75},"dt":1708677188,"sys":{"type":2,"id":2019527,"country":"US","sunrise":1708698233,"sunset":1708738818},"timezone":-28800,"id":0,"name":"San Diego","cod":200}

  // Replace with your country code and city
  // std::string city = "San%20Diego";
  // std::string countryCode = "840";

  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED) {
    // std::string serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city_copy + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=imperial";
    std::string serverPath = "http://api.openweathermap.org/data/2.5/weather?zip=" + std::to_string(location_zip_code_) + "," + location_country_code_ + "&appid=" + openWeatherMapApiKey + "&units=" + (weather_units_metric_not_imperial_ ? "metric" : "imperial" );
    WiFiClient client;
    HTTPClient http;
      
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    last_fetch_weather_info_time_ms_ = millis();
    
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
    // if (JSON.typeof(myObject) == "undefined" || httpResponseCode <= 0) {
    //   Serial.println("Parsing input failed!");
    // }
    if(httpResponseCode >= 200 && httpResponseCode < 300)
    {
      // got response
      got_weather_info_ = true;

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Weather: ");
      weather_main_.assign(myObject["weather"][0]["main"]);
      weather_description_.assign(myObject["weather"][0]["description"]);
      double val = atof(JSONVar::stringify(myObject["main"]["temp"]).c_str());
      char valArr[10]; sprintf(valArr,"%.1f%c", val, (weather_units_metric_not_imperial_ ? 'C' : 'F'));
      weather_temp_.assign(valArr);
      val = atof(JSONVar::stringify(myObject["main"]["feels_like"]).c_str());
      sprintf(valArr,"%.1f%c", val, (weather_units_metric_not_imperial_ ? 'C' : 'F'));
      weather_temp_feels_like_.assign(valArr);
      val = atof(JSONVar::stringify(myObject["main"]["temp_max"]).c_str());
      sprintf(valArr,"%.1f%c", val, (weather_units_metric_not_imperial_ ? 'C' : 'F'));
      weather_temp_max_.assign(valArr);
      val = atof(JSONVar::stringify(myObject["main"]["temp_min"]).c_str());
      sprintf(valArr,"%.1f%c", val, (weather_units_metric_not_imperial_ ? 'C' : 'F'));
      weather_temp_min_.assign(valArr);
      val = atof(JSONVar::stringify(myObject["wind"]["speed"]).c_str());
      sprintf(valArr,"%d%s", (int)val, (weather_units_metric_not_imperial_ ? "m/s" : "mi/hr"));
      weather_wind_speed_.assign(valArr);
      weather_humidity_.assign(JSONVar::stringify(myObject["main"]["humidity"]).c_str());
      weather_humidity_ = weather_humidity_ + '%';
      city_.assign(myObject["name"]);
      gmt_offset_sec_ = atoi(JSONVar::stringify(myObject["timezone"]).c_str());
      Serial.print("weather_main "); Serial.println(weather_main_.c_str());
      Serial.print("weather_description "); Serial.println(weather_description_.c_str());
      Serial.print("weather_temp "); Serial.println(weather_temp_.c_str());
      Serial.print("weather_temp_feels_like_ "); Serial.println(weather_temp_feels_like_.c_str());
      Serial.print("weather_temp_max "); Serial.println(weather_temp_max_.c_str());
      Serial.print("weather_temp_min "); Serial.println(weather_temp_min_.c_str());
      Serial.print("weather_wind_speed "); Serial.println(weather_wind_speed_.c_str());
      Serial.print("weather_humidity "); Serial.println(weather_humidity_.c_str());
      Serial.print("city_ "); Serial.println(city_.c_str());
      Serial.print("gmt_offset_sec_ "); Serial.println(gmt_offset_sec_);

    }
    else if(httpResponseCode >= 400) {
      Serial.println("Incorrect zip code!");
      incorrect_zip_code = true;
    }
    else {
      Serial.println("Parsing input failed!");
    }
  }
  else {
    PrintLn("WiFi not connected");
  }

  // turn off WiFi
  // TurnWiFiOff();
}

bool WiFiStuff::GetTimeFromNtpServer() {
  manual_time_update_successful_ = false;

  if(!got_weather_info_) { // we need gmt_offset_sec_ before getting time update!
    GetTodaysWeatherInfo();
    if(!got_weather_info_)
      return false;
  }

  // turn On Wifi
  if(!wifi_connected_)
    if(!TurnWiFiOn())
      return false;

  bool returnVal = false;

  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED) {

    const char* NTP_SERVER = "pool.ntp.org";
    // const long  GMT_OFFSET_SEC = -8*60*60;

    // Define an NTP Client object
    WiFiUDP udpSocket;
    NTPClient ntpClient(udpSocket, NTP_SERVER, gmt_offset_sec_);

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

      last_ntp_server_time_update_time_ms = millis();
      // auto update time today at 2:01AM success
      if(rtc->hourModeAndAmPm() == 1 && rtc->hour() == 2 && rtc->minute() >= 1)
        auto_updated_time_today_ = true;
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
  // TurnWiFiOff();

  manual_time_update_successful_ = returnVal;
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

#if defined(MCU_IS_ESP32)
void WiFiStuff::StartSetWiFiSoftAP() {
  PrintLn("WiFiStuff::StartSetWiFiSoftAP()");
  extern void _SoftAPWiFiDetails();

  extern AsyncWebServer* server;

  TurnWiFiOff();
  delay(100);

  if(server != NULL) {
    delete server;
    server = NULL;
  }

  WiFi.mode(WIFI_AP);
  delay(100);

  server = new AsyncWebServer(80);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(softApSsid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  soft_AP_IP = IP.toString().c_str();
  
  server->begin();

  _SoftAPWiFiDetails();
}

void WiFiStuff::StopSetWiFiSoftAP() {
  PrintLn("WiFiStuff::StopSetWiFiSoftAP()");
  extern AsyncWebServer* server;
  extern String temp_ssid_str, temp_passwd_str;

  // To access your stored values on ssid_str, passwd_str
  Serial.print("SSID: ");
  Serial.println(temp_ssid_str);

  Serial.print("PASSWD: ");
  Serial.println(temp_passwd_str);

  TurnWiFiOff();
  delay(100);

  if(server != NULL) {
    server->end();

    delete server;
    server = NULL;
  }

  wifi_stuff->wifi_ssid_ = temp_ssid_str.c_str();
  wifi_stuff->wifi_password_ = temp_passwd_str.c_str();
}

void WiFiStuff::StartSetLocationLocalServer() {
  PrintLn("WiFiStuff::StartSetLocationLocalServer()");
  extern void _LocalServerLocationInputs();

  extern AsyncWebServer* server;

  TurnWiFiOff();
  delay(100);

  if(server != NULL) {
    delete server;
    server = NULL;
  }

  soft_AP_IP = "";

  if(!TurnWiFiOn())
    return;
  delay(100);

  server = new AsyncWebServer(80);

  IPAddress IP = WiFi.localIP();
  Serial.print("Local IP address: ");
  Serial.println(IP);
  soft_AP_IP = IP.toString().c_str();
  
  _LocalServerLocationInputs();
}

void WiFiStuff::StopSetLocationLocalServer() {
  PrintLn("WiFiStuff::StopSetLocationLocalServer()");
  extern AsyncWebServer* server;
  extern String temp_zip_pin_str, temp_country_code_str;

  // To access your stored values on ssid_str, passwd_str
  Serial.print("ZIP/PIN: ");
  Serial.println(temp_zip_pin_str);

  Serial.print("Country Code: ");
  Serial.println(temp_country_code_str);

  TurnWiFiOff();
  delay(100);

  if(server != NULL) {
    server->end();

    delete server;
    server = NULL;
  }

  wifi_stuff->location_zip_code_ = std::atoi(temp_zip_pin_str.c_str());
  wifi_stuff->location_country_code_ = temp_country_code_str.c_str();
}

AsyncWebServer* server = NULL;

const char* kHtmlParamKeySsid = "html_ssid";
const char* kHtmlParamKeyPasswd = "html_passwd";
const char* kHtmlParamKeyZipPin = "html_zip_pin";
const char* kHtmlParamKeyCountryCode = "html_country_code";

String temp_ssid_str = "Enter SSID";
String temp_passwd_str = "Enter Passwd";
String temp_zip_pin_str = "Enter ZIP/PIN";
String temp_country_code_str = "Enter Country Code";

// HTML web page to handle 2 input fields (html_ssid, html_passwd)
const char index_html_wifi_details[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Long Press Alarm Clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Sent! Press 'Save' on device to set!");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
  	<a href="https://github.com/pk17r/Long_Press_Alarm_Clock/tree/release" target="_blank"><h3>Long Press Alarm Clock</h3></a>
    <h4>Enter WiFi Details:</h4>
    <label>WiFi SSID:</label><br>
    <input type="text" name="html_ssid" value="%html_ssid%"><br><br>
    <label>WiFi Password:</label><br>
    <input type="text" name="html_passwd" value="%html_passwd%"><br><br>
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

// HTML web page to handle 2 input fields (html_zip_pin, html_country_code)
const char index_html_location_details[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Long Press Alarm Clock</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Sent! Press 'Save' on device to set!");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
  	<a href="https://github.com/pk17r/Long_Press_Alarm_Clock/tree/release" target="_blank"><h3>Long Press Alarm Clock</h3></a>
    <h4>Enter Location Details:</h4>
    <label>Location ZIP/PIN Code:</label><br>
    <input type="number" name="html_zip_pin" value="%html_zip_pin%" min="10000" max="999999"><br><br>
    <label>2-Letter Country Code (</label>
    <a href="https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes#Current_ISO_3166_country_codes" target="_blank">List</a>
    <label>):</label><br>
    <input type="text" name="html_country_code" value="%html_country_code%" oninput="this.value = this.value.toUpperCase()"><br><br>
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

// Replaces placeholder with stored values
String processor(const String& var){
  if(strcmp(var.c_str(), kHtmlParamKeySsid) == 0){
    return temp_ssid_str;
  }
  else if(strcmp(var.c_str(), kHtmlParamKeyPasswd) == 0){
    return temp_passwd_str;
  }
  else if(strcmp(var.c_str(), kHtmlParamKeyZipPin) == 0){
    return temp_zip_pin_str;
  }
  else if(strcmp(var.c_str(), kHtmlParamKeyCountryCode) == 0){
    return temp_country_code_str;
  }
  return String();
}

void _SoftAPWiFiDetails() {

  temp_ssid_str = wifi_stuff->wifi_ssid_.c_str();
  temp_passwd_str = wifi_stuff->wifi_password_.c_str();

  extern String processor(const String& var);

  // Send web page with input fields to client
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html_wifi_details, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server->on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(kHtmlParamKeySsid)) {
      inputMessage = request->getParam(kHtmlParamKeySsid)->value();
      temp_ssid_str = inputMessage;
    }
    // GET html_passwd value on <ESP_IP>/get?html_passwd=<inputMessage>
    if (request->hasParam(kHtmlParamKeyPasswd)) {
      inputMessage = request->getParam(kHtmlParamKeyPasswd)->value();
      temp_passwd_str = inputMessage;
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

  // server->onNotFound(notFound);
  server->begin();
}

void _LocalServerLocationInputs() {

  temp_zip_pin_str = std::to_string(wifi_stuff->location_zip_code_).c_str();
  temp_country_code_str = wifi_stuff->location_country_code_.c_str();

  extern String processor(const String& var);

  // Send web page with input fields to client
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html_location_details, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server->on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(kHtmlParamKeyZipPin)) {
      inputMessage = request->getParam(kHtmlParamKeyZipPin)->value();
      temp_zip_pin_str = inputMessage;
    }
    // GET html_passwd value on <ESP_IP>/get?html_passwd=<inputMessage>
    if (request->hasParam(kHtmlParamKeyCountryCode)) {
      inputMessage = request->getParam(kHtmlParamKeyCountryCode)->value();
      temp_country_code_str = inputMessage;
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

  // server->onNotFound(notFound);
  server->begin();
}

// ESP32 Web OTA Update

// check for available firmware update
// Web OTA Update https://github.com/programmer131/ESP8266_ESP32_SelfUpdate/tree/master
// ESP32 WiFiClientSecure examples: WiFiClientInsecure.ino WiFiClientSecure.ino
bool WiFiStuff::FirmwareVersionCheck() {
  // turn On Wifi
  if(!wifi_connected_)
    if(!TurnWiFiOn())
      return false;

  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += (debug_mode ? URL_fw_Version_debug_mode.c_str() : URL_fw_Version_release.c_str());
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    if(use_secure_connection)
      client->setCACert(rootCACertificate);
    else
      client->setInsecure();//skip verification

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
        payload = https.getString(); // save received version
      else
        Serial.printf("error in downloading version file: %d\n", httpCode);
      https.end();
    }
    delete client;
  }

  std::string payload_str = payload.c_str();
  Serial.printf("payload_str: %s\n\n", payload_str.c_str());
  Serial.printf("kFwSearchStr: %s\n", kFwSearchStr.c_str());

  int search_str_index = payload_str.find(kFwSearchStr);
  // Serial.print("search_str_index = ");
  // Serial.println(search_str_index);

  if(search_str_index >= 0) {
    int fw_start_index = payload_str.find('"', search_str_index) + 1;
    int fw_end_index = payload_str.find('"', fw_start_index);
    // Serial.print("fw_start_index = ");
    // Serial.println(fw_start_index);
    // Serial.print("fw_end_index = ");
    // Serial.println(fw_end_index);
    std::string fw_str = payload_str.substr(fw_start_index, fw_end_index - fw_start_index);
    Serial.printf("Available kFirmwareVersion: %s\n", fw_str.c_str());
    Serial.printf("Active kFirmwareVersion: %s\n", kFirmwareVersion.c_str());
    firmware_update_available_str_ = fw_str;

    if(strcmp(fw_str.c_str(), kFirmwareVersion.c_str()) != 0) {
      Serial.println("New firmware detected");
      firmware_update_available_ = true;
      return true;
    }
    else {
      Serial.printf("Device already on latest firmware version: %s\n", kFirmwareVersion.c_str());
      return false;
    }
  }
  return false;
}

// update firmware
// Web OTA Update https://github.com/programmer131/ESP8266_ESP32_SelfUpdate/tree/master
// ESP32 WiFiClientSecure examples: WiFiClientInsecure.ino WiFiClientSecure.ino
void WiFiStuff::UpdateFirmware() {
  // turn On Wifi
  if(!wifi_connected_)
    if(!TurnWiFiOn())
      return;

  WiFiClientSecure client;

  if(use_secure_connection)
    client.setCACert(rootCACertificate);
  else
    client.setInsecure();//skip verification

  httpUpdate.setLedPin(LED_PIN, HIGH);

  // increase watchdog timeout to 90s to accomodate OTA update
  if(!debug_mode) SetWatchdogTime(kWatchdogTimeoutOtaUpdateMs);

  Serial.println(debug_mode ? URL_fw_Bin_debug_mode.c_str() : URL_fw_Bin_release.c_str());
  t_httpUpdate_return ret = httpUpdate.update(client, (debug_mode ? URL_fw_Bin_debug_mode.c_str() : URL_fw_Bin_release.c_str()));

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
  PrintLn("UpdateFirmware() unsuccessful.");
  if(!debug_mode) SetWatchdogTime(kWatchdogTimeoutMs);
}
#endif