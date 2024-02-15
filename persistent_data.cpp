
#include "persistent_data.h"

persistent_data::persistent_data() {
  eeprom.set_address(0x57);
  Wire.begin();
}


bool persistent_data::retrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  // start reading from the first byte (address 0) of the EEPROM
  unsigned int address = ALARM_ADDRESS_EEPROM;
  // read a byte from the current address of the EEPROM
  uint8_t value;
  eeprom.eeprom_read(address, &value); address++;
  if(value == 1) {
    // alarm data is stored in EEPROM
    // retrieve it
    eeprom.eeprom_read(address, &alarmHr); address++;
    eeprom.eeprom_read(address, &alarmMin); address++;
    eeprom.eeprom_read(address, &alarmIsAm); address++;
    eeprom.eeprom_read(address, &alarmOn); address++;

    Serial.println(F("Alarm settings retrieved from EEPROM."));

    return true;
  }

  return false;
}

void persistent_data::saveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  // start writing from the first byte of the EEPROM
  unsigned int address = ALARM_ADDRESS_EEPROM;
  // write alarm on EEPROM
  if (!eeprom.eeprom_write(address, 1)) {
    Serial.println("Failed to store 1");
  }address++;
  if (!eeprom.eeprom_write(address, alarmHr)) {
    Serial.println("Failed to store alarmHr");
  }address++;
  if (!eeprom.eeprom_write(address, alarmMin)) {
    Serial.println("Failed to store alarmMin");
  }address++;
  if (!eeprom.eeprom_write(address, alarmIsAm)) {
    Serial.println("Failed to store alarmIsAm");
  }address++;
  if (!eeprom.eeprom_write(address, alarmOn)) {
    Serial.println("Failed to store alarmOn");
  }address++;

  Serial.println(F("Alarm settings saved to EEPROM."));

}

void persistent_data::retrieveWiFiDetails(char* &wifi_ssid, char* &wifi_password) {

  // read WiFi SSID and Password
  char eeprom_read_array[WIFI_SSID_PASSWORD_LENGTH_MAX + 1];

  // read wifi_ssid
  int address = WIFI_ADDRESS_EEPROM;
  int char_arr_start_address = WIFI_ADDRESS_EEPROM;
  while(1) {
    char eeprom_char_read;
    eeprom.eeprom_read(address, &eeprom_char_read);
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
    char eeprom_char_read;
    eeprom.eeprom_read(address, &eeprom_char_read);
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

  Serial.println(F("WiFi details retrieved from EEPROM."));
}


void persistent_data::saveWiFiDetails(char* wifi_ssid, char* wifi_password) {

  // start writing from the first byte of the EEPROM
  unsigned int address = WIFI_ADDRESS_EEPROM;

  // write wifi_ssid on EEPROM
  int i = 0;
  while(1) {
    char c = *(wifi_ssid + i);
    if (!eeprom.eeprom_write(address, c)) {
      Serial.println("Failed to store c");
    }address++;
    i++;
    // break at null character
    if(c == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
      if (!eeprom.eeprom_write(address, '\0')) {
        Serial.println("Failed to store '\0'");
      }address++;
      break;
    }
  }
  
  // write wifi_password on EEPROM
  i = 0;
  while(1) {
    char c = *(wifi_password + i);
    if (!eeprom.eeprom_write(address, c)) {
      Serial.println("Failed to store c");
    }address++;
    i++;
    // break at null character
    if(c == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(i >= WIFI_SSID_PASSWORD_LENGTH_MAX) {
      if (!eeprom.eeprom_write(address, '\0')) {
        Serial.println("Failed to store '\0'");
      }address++;
      break;
    }
  }

  Serial.println(F("WiFi ssid and password written to EEPROM"));

}

