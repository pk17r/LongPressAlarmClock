#include "wifi_stuff.h"

void wifi_stuff::retrieveWiFiDetails() {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    
    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);
    
    // read WiFi SSID and Password
    char eeprom_read_array[WIFI_SSID_PASSWORD_LENGTH_MAX + 1];

    // read wifi_ssid
    int address = WIFI_ADDRESS_EEPROM;
    int char_arr_start_address = WIFI_ADDRESS_EEPROM;
    while(1) {
      char eeprom_char_read = EEPROM.read(address);
      eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
      address++;
      // break at null character
      if(eeprom_char_read == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(address >= char_arr_start_address + WIFI_SSID_PASSWORD_LENGTH_MAX) {
        eeprom_read_array[address - char_arr_start_address] = '\0';
        break;
      }
    }
    // fill wifi_ssid
    if(wifi_ssid != NULL) {
      delete wifi_ssid;
      wifi_ssid = NULL;
    }
    wifi_ssid = new char[address - char_arr_start_address];   // allocate space
    strcpy(wifi_ssid,eeprom_read_array);

    // read wifi_password
    char_arr_start_address = address;
    while(1) {
      char eeprom_char_read = EEPROM.read(address);
      eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
      address++;
      // break at null character
      if(eeprom_char_read == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(address >= char_arr_start_address + WIFI_SSID_PASSWORD_LENGTH_MAX) {
        eeprom_read_array[address - char_arr_start_address] = '\0';
        break;
      }
    }
    // fill wifi_password
    if(wifi_password != NULL) {
      delete wifi_password;
      wifi_password = NULL;
    }
    wifi_password = new char[address - char_arr_start_address];   // allocate space
    strcpy(wifi_password,eeprom_read_array);

    // End reading EEPROM on Raspberry Pi Pico
    EEPROM.end();

    Serial.println(F("WiFi details retrieved from EEPROM."));

  #elif defined(MCU_IS_ESP32)

    Serial.println(F("WiFi details retrieving on ESP32 is not Implemented yet."));

  #endif
}

void wifi_stuff::saveWiFiDetails() {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);

    // start writing from the first byte of the EEPROM
    unsigned int address = WIFI_ADDRESS_EEPROM;

    // write wifi_ssid on EEPROM
    int i = 0;
    while(1) {
      char c = *(wifi_ssid + i);
      EEPROM.write(address, c); address++;
      EEPROM.commit();
      i++;
      // break at null character
      if(c == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
        EEPROM.write(address, '\0'); address++;
        EEPROM.commit();
        break;
      }
    }
    
    // write wifi_password on EEPROM
    i = 0;
    while(1) {
      char c = *(wifi_password + i);
      EEPROM.write(address, c); address++;
      EEPROM.commit();
      i++;
      // break at null character
      if(c == '\0')
        break;
      // limit to force out of while loop, won't reach here in normal operation
      if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
        EEPROM.write(address, '\0'); address++;
        EEPROM.commit();
        break;
      }
    }

    // End reading EEPROM on Raspberry Pi Pico
    EEPROM.end();

    Serial.println(F("WiFi ssid and password written to EEPROM"));

  #elif defined(MCU_IS_ESP32)

    Serial.println(F("WiFi details saving on ESP32 is not Implemented yet."));

  #endif

}

void wifi_stuff::turn_WiFi_On() {
  Serial.println(F("Connecting to WiFi"));
  WiFi.persistent(true);
  delay(1);
  WiFi.begin(wifi_ssid, wifi_password);
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

void wifi_stuff::turn_WiFi_Off() {
  WiFi.persistent(false);
  delay(1);
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  Serial.println(F("WiFi Off."));
}

void wifi_stuff::getTodaysWeatherInfo() {
  gotWeatherInfo = false;

  // turn On Wifi
  turn_WiFi_On();

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
      gotWeatherInfo = true;

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Weather: ");
      {
      String sss = myObject["weather"][0]["main"];
      if(weather_main != NULL) { delete weather_main; weather_main = NULL;}
      weather_main = new char[sss.length()];
      strcpy(weather_main, sss.c_str());
      } {
      String sss = myObject["weather"][0]["description"];
      if(weather_description != NULL) { delete weather_description; weather_description = NULL;}
      weather_description = new char[sss.length()];
      strcpy(weather_description, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp"]);
      if(weather_temp != NULL) { delete weather_temp; weather_temp = NULL;}
      weather_temp = new char[sss.length()];
      strcpy(weather_temp, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp_max"]);
      if(weather_temp_max != NULL) { delete weather_temp_max; weather_temp_max = NULL;}
      weather_temp_max = new char[sss.length()];
      strcpy(weather_temp_max, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["temp_min"]);
      if(weather_temp_min != NULL) { delete weather_temp_min; weather_temp_min = NULL;}
      weather_temp_min = new char[sss.length()];
      strcpy(weather_temp_min, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["wind"]["speed"]);
      if(weather_wind_speed != NULL) { delete weather_wind_speed; weather_wind_speed = NULL;}
      weather_wind_speed = new char[sss.length()];
      strcpy(weather_wind_speed, sss.c_str());
      } {
      String sss = JSONVar::stringify(myObject["main"]["humidity"]);
      if(weather_humidity != NULL) { delete weather_humidity; weather_humidity = NULL;}
      weather_humidity = new char[sss.length()];
      strcpy(weather_humidity, sss.c_str());
      }
      Serial.print("weather_main "); Serial.println(weather_main);
      Serial.print("weather_description "); Serial.println(weather_description);
      Serial.print("weather_temp "); Serial.println(weather_temp);
      Serial.print("weather_temp_max "); Serial.println(weather_temp_max);
      Serial.print("weather_temp_min "); Serial.println(weather_temp_min);
      Serial.print("weather_wind_speed "); Serial.println(weather_wind_speed);
      Serial.print("weather_humidity "); Serial.println(weather_humidity);
    }
  }
  else {
    Serial.println("WiFi not connected");
  }

  // turn off WiFi
  turn_WiFi_Off();
}