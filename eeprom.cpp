#include <sys/_stdint.h>
#include "eeprom.h"
#include "Wire.h"

EEPROM::EEPROM() {
  eeprom_.set_address(0x57);
  #if defined(MCU_IS_RP2040)
    Wire.setSDA(SDA_PIN);
    Wire.setSCL(SCL_PIN);
    Wire.begin();
  #elif defined (MCU_IS_ESP32)
    Wire.begin(SDA_PIN, SCL_PIN);
  #endif

  // check if data is compatible with code, otherwise set the respective flag and default data
  if(Fetch1Byte(kDataModelVersionAddress) != kDataModelVersion)
    SaveDefaults();

  PrintLn("EEPROM Initialized!");
}

void EEPROM::SaveDefaults() {
  int delay_ms = 25;
  delay(delay_ms);
  Save1Byte(kDataModelVersionAddress, kDataModelVersion);
  delay(delay_ms);
  Save1Byte(kAlarmHrAddress, kAlarmHr);
  delay(delay_ms);
  Save1Byte(kAlarmMinAddress, kAlarmMin);
  delay(delay_ms);
  Save1Byte(kAlarmIsAmAddress, kAlarmIsAm);
  delay(delay_ms);
  Save1Byte(kAlarmOnAddress, kAlarmOn);
  delay(delay_ms);
  SaveString(kWiFiSsidLengthAddress, kWiFiSsidLengthMax, kWiFiSsidAddress, kWiFiSsid);
  delay(delay_ms);
  SaveString(kWiFiPasswdLengthAddress, kWiFiPasswdLengthMax, kWiFiPasswdAddress, kWiFiPasswd);
  delay(delay_ms);
  Save4Bytes(kWeatherZipCodeAddress, kWeatherZipCode);
  delay(delay_ms);
  Save1Byte(kWeatherCountryCodeAddress, static_cast<uint8_t>(kWeatherCountryCode[0]));
  delay(delay_ms);
  Save1Byte(kWeatherCountryCodeAddress + 1, static_cast<uint8_t>(kWeatherCountryCode[1]));
  delay(delay_ms);
  Save1Byte(kWeatherUnitsMetricNotImperialAddress, kWeatherUnitsMetricNotImperial);
  delay(delay_ms);
  Save1Byte(kAlarmLongPressSecondsAddress, kAlarmLongPressSeconds);
  delay(delay_ms);
  SaveCurrentFirmwareVersion();
  delay(delay_ms);
  SaveCpuSpeed();
  delay(delay_ms);
  SaveScreensaverBounceNotFlyHorizontally(true);

  PrintLn("Defaults saved to EEPROM!");
}

void EEPROM::RetrieveLongPressSeconds(uint8_t &long_press_seconds) {
  long_press_seconds = Fetch1Byte(kAlarmLongPressSecondsAddress);
}

void EEPROM::SaveLongPressSeconds(uint8_t long_press_seconds) {
  Save1Byte(kAlarmLongPressSecondsAddress, long_press_seconds);
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
  PrintLn("EEPROM wifi_ssid: ", wifi_ssid.c_str());
  if(debug_mode) PrintLn("EEPROM wifi_password: ", wifi_password.c_str());
  PrintLn("WiFi details retrieved from EEPROM.");
}

void EEPROM::SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password) {
  SaveString(kWiFiSsidLengthAddress, kWiFiSsidLengthMax, kWiFiSsidAddress, wifi_ssid);
  SaveString(kWiFiPasswdLengthAddress, kWiFiPasswdLengthMax, kWiFiPasswdAddress, wifi_password);
  if(debug_mode) {
    PrintLn("EEPROM wifi_ssid: ", wifi_ssid.c_str());
    PrintLn("EEPROM wifi_password: ", wifi_password.c_str());
  }
  PrintLn("WiFi ssid and password written to EEPROM");
}

void EEPROM::RetrieveSavedFirmwareVersion(std::string &savedFirmwareVersion) {
  savedFirmwareVersion = FetchString(kFirmwareVersionLengthAddress, kFirmwareVersionAddress);
  PrintLn("Saved Firmware Version: ", savedFirmwareVersion.c_str());
}

void EEPROM::SaveCurrentFirmwareVersion() {
  SaveString(kFirmwareVersionLengthAddress, kFirmwareVersionLengthMax, kFirmwareVersionAddress, kFirmwareVersion);
  PrintLn("Current Firmware Version written to EEPROM");
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

void EEPROM::SaveWeatherUnits(bool weather_units_metric_not_imperial) {
  Save1Byte(kWeatherUnitsMetricNotImperialAddress, static_cast<uint8_t>(weather_units_metric_not_imperial));
  PrintLn("Weather Location details written to EEPROM");
}

uint32_t EEPROM::RetrieveSavedCpuSpeed() {
  uint32_t saved_cpu_speed_mhz = Fetch4Bytes(kCpuSpeedMhzAddress);
  Serial.printf("EEPROM saved_cpu_speed_mhz: %u MHz\n", saved_cpu_speed_mhz);
  return saved_cpu_speed_mhz;
}

void EEPROM::SaveCpuSpeed() {
  Save4Bytes(kCpuSpeedMhzAddress, cpu_speed_mhz);
  Serial.printf("EEPROM cpu_speed_mhz: %u MHz saved.\n", cpu_speed_mhz);
}

bool EEPROM::RetrieveScreensaverBounceNotFlyHorizontally() {
  uint8_t screensaver_motion_type = Fetch1Byte(kScreensaverMotionTypeAddress);
  bool screensaver_bounce_not_fly_horiontally = (screensaver_motion_type == 0 ? false : true);
  Serial.printf("EEPROM screensaver_bounce_not_fly_horiontally: %d retrieved.\n", screensaver_bounce_not_fly_horiontally);
  return screensaver_bounce_not_fly_horiontally;
}

void EEPROM::SaveScreensaverBounceNotFlyHorizontally(bool screensaver_bounce_not_fly_horiontally) {
  Save1Byte(kScreensaverMotionTypeAddress, screensaver_bounce_not_fly_horiontally);
  Serial.printf("EEPROM screensaver_bounce_not_fly_horiontally: %d saved.\n", screensaver_bounce_not_fly_horiontally);
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
  int text_length = min((int)(text.length()), (int)length_max);
  Save1Byte(length_address, text_length);
  for (int i = 0; i < text_length; i++) {
    Save1Byte(data_address + i, static_cast<uint8_t>(text[i]));
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

// last leg of interaction with EEPROM
uint8_t EEPROM::Fetch1Byte(uint16_t address) {
  uint8_t value;
  eeprom_.eeprom_read(address, &value);
  return value;
}

// last leg of interaction with EEPROM
void EEPROM::Save1Byte(uint16_t address, uint8_t value) {
  if (!eeprom_.eeprom_write(address, value)) {
    PrintLn("Failed to save to EEPROM!");
  }
}
