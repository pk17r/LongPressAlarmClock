#include "eeprom.h"
#include "Wire.h"

EEPROM::EEPROM() {
  eeprom_.set_address(0x57);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();

  // check if data is compatible with code, otherwise set the respective flag and default data
  
  PrintLn("EEPROM Initialized!");
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

    PrintLn("Alarm settings retrieved from EEPROM.");

    return true;
  }

  PrintLn("Could not retrieve Alarm settings from EEPROM.");

  return false;
}

void EEPROM::SaveAlarm(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {

  // start writing from the first byte of the EEPROM
  unsigned int address = kAlarmAddressEEPROM;
  // write alarm on EEPROM
  if (!eeprom_.eeprom_write(address, 1)) {
    PrintLn("Failed to store 1");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmHr)) {
    PrintLn("Failed to store alarmHr");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmMin)) {
    PrintLn("Failed to store alarmMin");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmIsAm)) {
    PrintLn("Failed to store alarmIsAm");
  }address++;
  if (!eeprom_.eeprom_write(address, alarmOn)) {
    PrintLn("Failed to store alarmOn");
  }address++;

  PrintLn("Alarm settings saved to EEPROM.");

}

void EEPROM::RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password) {

  PrintLn("RetrieveWiFiDetails");
  // read wifi_ssid
  wifi_ssid = "";
  int address = kWifiAddressEEPROM;
  int char_arr_start_address = kWifiAddressEEPROM;
  while(1) {
    char eeprom_char_read;
    eeprom_.eeprom_read(address, &eeprom_char_read);
    address++;
    // break at null character
    if(eeprom_char_read == '\0')
      break;
    wifi_ssid = wifi_ssid + eeprom_char_read;
    // limit to force out of while loop, won't reach here in normal operation
    if(address >= char_arr_start_address + kWifiSsidPasswordLengthMax) {
      break;
    }
  }
  // PrintLn("EEPROM wifi_ssid: ", wifi_ssid);

  // read wifi_password
  wifi_password = "";
  char_arr_start_address = address;
  while(1) {
    char eeprom_char_read;
    eeprom_.eeprom_read(address, &eeprom_char_read);
    address++;
    // break at null character
    if(eeprom_char_read == '\0')
      break;
    wifi_password = wifi_password + eeprom_char_read;
    // limit to force out of while loop, won't reach here in normal operation
    if(address >= char_arr_start_address + kWifiSsidPasswordLengthMax) {
      break;
    }
  }
  // PrintLn("EEPROM wifi_password: ", wifi_password);

  PrintLn("WiFi details retrieved from EEPROM.");
}

void EEPROM::SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password) {

  // start writing from the first byte of the EEPROM
  unsigned int address = kWifiAddressEEPROM;

  // write wifi_ssid on EEPROM
  int i = 0;
  while(1) {
    char c = wifi_ssid[i];
    if (!eeprom_.eeprom_write(address, c)) {
      PrintLn("Failed to store c");
    }address++;
    i++;
    // limit to force out of while loop, won't reach here in normal operation
    if(i == kWifiSsidPasswordLengthMax || i == wifi_ssid.length()) {
      if (!eeprom_.eeprom_write(address, '\0')) {
        PrintLn("Failed to store '\0'");
      }address++;
      break;
    }
  }

  // write wifi_password on EEPROM
  i = 0;
  while(1) {
    char c = wifi_password[i];
    if (!eeprom_.eeprom_write(address, c)) {
      PrintLn("Failed to store c");
    }address++;
    i++;
    // limit to force out of while loop, won't reach here in normal operation
    if(i == kWifiSsidPasswordLengthMax || i == wifi_password.length()) {
      if (!eeprom_.eeprom_write(address, '\0')) {
        PrintLn("Failed to store '\0'");
      }address++;
      break;
    }
  }

  PrintLn("WiFi ssid and password written to EEPROM");

}

