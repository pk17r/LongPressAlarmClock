
#include "persistent_data.h"


bool persistent_data::retrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {
  #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);
  #endif

  #if defined(MCU_IS_TEENSY) || defined(MCU_IS_RASPBERRY_PI_PICO_W)

    // start reading from the first byte (address 0) of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    byte value;
    // read a byte from the current address of the EEPROM
    value = EEPROM.read(address);
    if(value == 1) {
      // alarm data is stored in EEPROM
      // retrieve it
      address++;
      alarmHr = EEPROM.read(address); address++;
      alarmMin = EEPROM.read(address); address++;
      alarmIsAm = EEPROM.read(address); address++;
      alarmOn = EEPROM.read(address); address++;

      #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
        // End reading EEPROM on Raspberry Pi Pico
        EEPROM.end();
      #endif

      Serial.println(F("Alarm settings retrieved from EEPROM."));

      return true;
    }
  
    #if defined(MCU_IS_RASPBERRY_PI_PICO_W)
      // End reading EEPROM on Raspberry Pi Pico
      EEPROM.end();
    #endif
  #endif

  return false;
}

void persistent_data::saveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  #if defined(MCU_IS_TEENSY)

    // start writing from the first byte of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    // write alarm on EEPROM
    EEPROM.update(address, 1); address++;
    EEPROM.update(address, alarmHr); address++;
    EEPROM.update(address, alarmMin); address++;
    EEPROM.update(address, alarmIsAm); address++;
    EEPROM.update(address, alarmOn);

    Serial.println("Alarm written to EEPROM");

  #elif defined(MCU_IS_RASPBERRY_PI_PICO_W)

    // Begin reading EEPROM on Raspberry Pi Pico
    EEPROM.begin(512);

    // start writing from the first byte of the EEPROM
    unsigned int address = ALARM_ADDRESS_EEPROM;
    // write alarm on EEPROM
    EEPROM.write(address, 1); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmHr); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmMin); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmIsAm); address++;
    EEPROM.commit();
    EEPROM.write(address, alarmOn);
    EEPROM.commit();

    // End reading EEPROM on Raspberry Pi Pico
    EEPROM.end();
    Serial.println("Alarm written to EEPROM");

  #endif

}

void persistent_data::retrieveWiFiDetails(char* wifi_ssid, char* wifi_password) {
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

void persistent_data::saveWiFiDetails(char* wifi_ssid, char* wifi_password) {
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

