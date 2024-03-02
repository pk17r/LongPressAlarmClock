#include "wifi_stuff.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "eeprom.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "rtc.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

WiFiStuff::WiFiStuff() {

  eeprom->RetrieveWiFiDetails(wifi_ssid_, wifi_password_);

  eeprom->RetrieveWeatherLocationDetails(location_zip_code_, location_country_code_, weather_units_metric_not_imperial_);

  TurnWiFiOff();

  PrintLn("WiFiStuff Initialized!");
}

void WiFiStuff::SaveWiFiDetails() {
  eeprom->SaveWiFiDetails(wifi_ssid_, wifi_password_);
}

void WiFiStuff::SaveWeatherLocationDetails() {
  eeprom->SaveWeatherLocationDetails(location_zip_code_, location_country_code_, weather_units_metric_not_imperial_);
}

void WiFiStuff::SaveWeatherUnits() {
  eeprom->SaveWeatherUnits(weather_units_metric_not_imperial_);
}

void WiFiStuff::TurnWiFiOn() {
  PrintLn("Connecting to WiFi");
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
  }
  else {
    PrintLn("Could NOT connect to WiFi.");
    digitalWrite(LED_BUILTIN, LOW);
    wifi_connected_ = false;
  }
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

  // turn On Wifi
  if(!wifi_connected_)
    TurnWiFiOn();

  // Your Domain name with URL path or IP address with path
  std::string openWeatherMapApiKey = "0fad3740b3a6b502ad57504f6fc3521e";

  //https://api.openweathermap.org/data/2.5/weather?zip=92104,US&appid=0fad3740b3a6b502ad57504f6fc3521e
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
  }
  else {
    PrintLn("WiFi not connected");
  }

  // turn off WiFi
  // TurnWiFiOff();
}

bool WiFiStuff::GetTimeFromNtpServer() {

  if(!got_weather_info_) return false; // we need gmt_offset_sec_ before getting time update!

  bool returnVal = false;

  // turn On Wifi
  if(!wifi_connected_)
    TurnWiFiOn();


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

void WiFiStuff::GetSsidAndPasswdUsingSoftAP() {
  extern void SoftAP();
  SoftAP();
}

AsyncWebServer* server = NULL;

const char* PARAM_ssid = "html_ssid";
const char* PARAM_passwd = "html_passwd";
const char* PARAM_INT = "html_int";

String ssid_str = "Enter SSID";
String passwd_str = "Enter Passwd";
String int_var_str = "22";

// HTML web page to handle 3 input fields (html_ssid, html_passwd, html_int)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Long Press Alarm WiFi Details Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved!");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    <label>ssid:</label>
    <input type="text" name="html_ssid" value="%html_ssid%"><br><br>
    <label>password:</label>
    <input type="text" name="html_passwd" value="%html_passwd%"><br><br>
    <label>html_int:</label>
    <input type="text" name="html_int" value="%html_int%"><br><br>
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(strcmp(var.c_str(), PARAM_ssid) == 0){
    return ssid_str;
  }
  else if(strcmp(var.c_str(), PARAM_passwd) == 0){
    return passwd_str;
  }
  else if(strcmp(var.c_str(), PARAM_INT) == 0){
    return int_var_str;
  }
  return String();
}

void SoftAP() {

  server = new AsyncWebServer(80);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(softApSsid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server->begin();

  ssid_str = wifi_stuff->wifi_ssid_.c_str();
  passwd_str = wifi_stuff->wifi_password_.c_str();

  extern String processor(const String& var);

  // Send web page with input fields to client
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server->on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_ssid)) {
      inputMessage = request->getParam(PARAM_ssid)->value();
      ssid_str = inputMessage;
    }
    // GET html_int value on <ESP_IP>/get?html_int=<inputMessage>
    if (request->hasParam(PARAM_passwd)) {
      inputMessage = request->getParam(PARAM_passwd)->value();
      passwd_str = inputMessage;
    }
    // GET html_passwd value on <ESP_IP>/get?html_passwd=<inputMessage>
    if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      int_var_str = inputMessage;
    }
    // else {
    //   inputMessage = "No message sent";
    // }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  // server->onNotFound(notFound);
  server->begin();

  while(1)
  {
    ResetWatchdog();
    // To access your stored values on ssid_str, passwd_str, int_var_str
    Serial.print("SSID: ");
    Serial.println(ssid_str);
    
    Serial.print("PASSWD: ");
    Serial.println(passwd_str);
    
    int yourInputInt = atoi(int_var_str.c_str());
    Serial.print("INT VAR: ");
    Serial.println(yourInputInt);
    delay(5000);

    if(yourInputInt == 1) {
      Serial.println("break");
      break;
    }
  }

  server->end();

  delete server;
  server = NULL;

  // wifi_stuff->wifi_ssid_ = ssid_str.c_str();
  // wifi_stuff->wifi_password_ = passwd_str.c_str();
}
