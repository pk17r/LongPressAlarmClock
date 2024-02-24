#include <sys/_stdint.h>
#include "eeprom.h"
#include "Wire.h"

EEPROM::EEPROM() {
  eeprom_.set_address(0x57);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();

  // check if data is compatible with code, otherwise set the respective flag and default data
  if(Fetch1Byte(kDataModelVersionAddress) != kDataModelVersion)
    SaveDefaults();
  
  PrintLn("EEPROM Initialized!");
}

void EEPROM::SaveDefaults() {
  Save1Byte(kDataModelVersionAddress, kDataModelVersion);
  Save1Byte(kAlarmHrAddress, kAlarmHr);
  Save1Byte(kAlarmMinAddress, kAlarmMin);
  Save1Byte(kAlarmIsAmAddress, kAlarmIsAm);
  Save1Byte(kAlarmOnAddress, kAlarmOn);
  SaveString(kWiFiSsidLengthAddress, kWiFiSsidLengthMax, kWiFiSsidAddress, kWiFiSsid);
  SaveString(kWiFiPasswdLengthAddress, kWiFiPasswdLengthMax, kWiFiPasswdAddress, kWiFiPasswd);
  Save4Bytes(kWeatherZipCodeAddress, kWeatherZipCode);
  Save1Byte(kWeatherCountryCodeAddress, static_cast<uint8_t>(kWeatherCountryCode[0]));
  Save1Byte(kWeatherCountryCodeAddress + 1, static_cast<uint8_t>(kWeatherCountryCode[1]));
  Save1Byte(kWeatherUnitsMetricNotImperialAddress, kWeatherUnitsMetricNotImperial);
  PrintLn("Defaults saved to EEPROM!");
}

void EEPROM::RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {
  alarmHr = Fetch1Byte(kAlarmHrAddress);
  alarmMin = Fetch1Byte(kAlarmMinAddress);
  alarmIsAm = static_cast<bool>(Fetch1Byte(kAlarmIsAmAddress));
  alarmOn = static_cast<bool>(Fetch1Byte(kAlarmOnAddress));
}

void EEPROM::SaveAlarm(uint8_t alarmHr, uint8_t alarmMin, bool alarmIsAm, bool alarmOn) {
  Save1Byte(kAlarmHrAddress, alarmHr);
  Save1Byte(kAlarmMinAddress, alarmMin);
  Save1Byte(kAlarmIsAmAddress, static_cast<uint8_t>(alarmIsAm));
  Save1Byte(kAlarmOnAddress, static_cast<uint8_t>(alarmOn));
}

void EEPROM::RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password) {
  wifi_ssid = FetchString(kWiFiSsidLengthAddress, kWiFiSsidAddress);
  wifi_password = FetchString(kWiFiPasswdLengthAddress, kWiFiPasswdAddress);
  PrintLn("EEPROM wifi_ssid: ", wifi_ssid);
  PrintLn("EEPROM wifi_password: ", wifi_password);
  PrintLn("WiFi details retrieved from EEPROM.");
}

void EEPROM::SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password) {
  SaveString(kWiFiSsidLengthAddress, kWiFiSsidLengthMax, kWiFiSsidAddress, wifi_ssid);
  SaveString(kWiFiPasswdLengthAddress, kWiFiPasswdLengthMax, kWiFiPasswdAddress, wifi_password);
  PrintLn("WiFi ssid and password written to EEPROM");

}

void EEPROM::RetrieveWeatherLocationDetails(uint32_t &location_zip_code, std::string &location_country_code, bool &weather_units_metric_not_imperial) {
  location_zip_code = Fetch4Bytes(kWeatherZipCodeAddress);
  location_country_code = static_cast<char>(Fetch1Byte(kWeatherCountryCodeAddress));
  location_country_code = location_country_code + static_cast<char>(Fetch1Byte(kWeatherCountryCodeAddress + 1));
  weather_units_metric_not_imperial = static_cast<bool>(Fetch1Byte(kWeatherUnitsMetricNotImperialAddress));
  PrintLn("EEPROM location_zip_code: ", location_zip_code);
  PrintLn("EEPROM location_country_code: ", location_country_code);
  PrintLn("EEPROM weather_units_metric_not_imperial: ", weather_units_metric_not_imperial);
  PrintLn("Weather Location details retrieved from EEPROM.");
}

void EEPROM::SaveWeatherLocationDetails(uint32_t location_zip_code, std::string location_country_code, bool weather_units_metric_not_imperial) {
  Save4Bytes(kWeatherZipCodeAddress, location_zip_code);
  Save1Byte(kWeatherCountryCodeAddress, static_cast<uint8_t>(location_country_code[0]));
  Save1Byte(kWeatherCountryCodeAddress + 1, static_cast<uint8_t>(location_country_code[1]));
  Save1Byte(kWeatherUnitsMetricNotImperialAddress, static_cast<uint8_t>(weather_units_metric_not_imperial));
  PrintLn("Weather Location details written to EEPROM");

}

std::string EEPROM::FetchString(uint16_t length_address, uint16_t data_address) {
  int text_length = Fetch1Byte(length_address);
  std::string text = "";
  for (int i = 0; i < text_length; i++) {
    text = text + static_cast<char>(Fetch1Byte(data_address + i));
  }
  return text;
}

void EEPROM::SaveString(uint16_t length_address, uint8_t length_max, uint16_t data_address, std::string text) {
  Save1Byte(length_address, min(text.length(), length_max));
  for (int i = 0; i < min(text.length(), length_max); i++) {
    Save1Byte(data_address + i, static_cast<uint8_t>(text[i]));
  }
}


uint8_t EEPROM::Fetch1Byte(uint16_t address) {
  uint8_t value;
  eeprom_.eeprom_read(address, &value);
  return value;
}

void EEPROM::Save1Byte(uint16_t address, uint8_t value) {
  if (!eeprom_.eeprom_write(address, value)) {
    PrintLn("Failed to save to EEPROM!");
  } 
}

uint32_t EEPROM::Fetch4Bytes(uint16_t address) {
  uint32_t value = Fetch1Byte(address);   // MSB
  value = value << 8;
  value = value | Fetch1Byte(address + 1);
  value = value << 8;
  value = value | Fetch1Byte(address + 2);
  value = value << 8;
  value = value | Fetch1Byte(address + 3);    // LSB
  return value;
}

void EEPROM::Save4Bytes(uint16_t address, uint32_t value) {
  uint8_t val8bit = value & 0xff; // LSB
  Save1Byte(address + 3, val8bit);
  value = value >> 8;
  val8bit = value & 0xff;
  Save1Byte(address + 2, val8bit);
  value = value >> 8;
  val8bit = value & 0xff;
  Save1Byte(address + 1, val8bit);
  value = value >> 8;
  val8bit = value & 0xff;
  Save1Byte(address, val8bit);   // MSB
}


