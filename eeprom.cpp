#include "eeprom.h"
#include "Wire.h"

EEPROM::EEPROM() {
  eeprom_.set_address(0x57);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();

  // check if data is compatible with code, otherwise set the respective flag and default data
  
}

bool EEPROM::RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  // start reading from the first byte (address 0) of the EEPROM
  unsigned int address = kAlarmAddressEEPROM;
  // read a byte from the current address of the EEPROM
  uint8_t value;
  eeprom_.eeprom_read(address, &value); address++;
  if(value == 1) {
    // alarm data is stored in EEPROM
    // retrieve it
    eeprom_.eeprom_read(address, &alarmHr); address++;
    eeprom_.eeprom_read(address, &alarmMin); address++;
    eeprom_.eeprom_read(address, &alarmIsAm); address++;
    eeprom_.eeprom_read(address, &alarmOn); address++;

    Serial.println(F("Alarm settings retrieved from EEPROM."));

    return true;
  }

  return false;
}

void EEPROM::SaveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  // start writing from the first byte of the EEPROM
  unsigned int address = kAlarmAddressEEPROM;
  // write alarm on EEPROM
  if (!eeprom_.eeprom_write(address, 1)) {
    Serial.println("Failed to store 1");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmHr)) {
    Serial.println("Failed to store alarmHr");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmMin)) {
    Serial.println("Failed to store alarmMin");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmIsAm)) {
    Serial.println("Failed to store alarmIsAm");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmOn)) {
    Serial.println("Failed to store alarmOn");
  }address++;

  Serial.println(F("Alarm settings saved to EEPROM."));

}

void EEPROM::RetrieveWiFiDetails(char* &wifi_ssid, char* &wifi_password) {

  // read WiFi SSID and Password
  char eeprom_read_array[kWifiSsidPasswordLengthMax + 1];

  // read wifi_ssid
  int address = kWifiAddressEEPROM;
  int char_arr_start_address = kWifiAddressEEPROM;
  while(1) {
    char eeprom_char_read;
    eeprom_.eeprom_read(address, &eeprom_char_read);
    eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
    address++;
    // break at null character
    if(eeprom_char_read == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(address >= char_arr_start_address + kWifiSsidPasswordLengthMax) {
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
  // Serial.print("EEPROM wifi_ssid: "); Serial.println(wifi_ssid);

  // read wifi_password
  char_arr_start_address = address;
  while(1) {
    char eeprom_char_read;
    eeprom_.eeprom_read(address, &eeprom_char_read);
    eeprom_read_array[address - char_arr_start_address] = eeprom_char_read;
    address++;
    // break at null character
    if(eeprom_char_read == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(address >= char_arr_start_address + kWifiSsidPasswordLengthMax) {
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
  // Serial.print("EEPROM wifi_password: "); Serial.println(wifi_password);

  Serial.println(F("WiFi details retrieved from EEPROM."));
}

void EEPROM::SaveWiFiDetails(char* wifi_ssid, char* wifi_password) {

  // start writing from the first byte of the EEPROM
  unsigned int address = kWifiAddressEEPROM;

  // write wifi_ssid on EEPROM
  int i = 0;
  while(1) {
    char c = *(wifi_ssid + i);
    if (!eeprom_.eeprom_write(address, c)) {
      Serial.println("Failed to store c");
    }address++;
    i++;
    // break at null character
    if(c == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(i >= kWifiSsidPasswordLengthMax) {
      if (!eeprom_.eeprom_write(address, '\0')) {
        Serial.println("Failed to store '\0'");
      }address++;
      break;
    }
  }
  
  // write wifi_password on EEPROM
  i = 0;
  while(1) {
    char c = *(wifi_password + i);
    if (!eeprom_.eeprom_write(address, c)) {
      Serial.println("Failed to store c");
    }address++;
    i++;
    // break at null character
    if(c == '\0')
      break;
    // limit to force out of while loop, won't reach here in normal operation
    if(i >= kWifiSsidPasswordLengthMax) {
      if (!eeprom_.eeprom_write(address, '\0')) {
        Serial.println("Failed to store '\0'");
      }address++;
      break;
    }
  }

  Serial.println(F("WiFi ssid and password written to EEPROM"));

}

