#include "nvs_preferences.h"

NvsPreferences::NvsPreferences() {
  // init saved data in EEPROM
 	getOrSetDefaultEEPROMparams();

  setEEPROMparams();

  getOrSetDefaultEEPROMparams();

  Serial.println(F("ESP32 NVS Memory setup successful!"));
}

void NvsPreferences::getOrSetDefaultEEPROMparams() {
  //init preference
 	preferences.begin(kNvsDataKey, /*readOnly = */ false);

 	// set default alarm time if it doesn't exist in memory
  data_version = preferences.getUInt(kDataVersionKey, 0); // get alarmHour or if key doesn't exist set variable to ALARM_HOUR_DEFAULT
  Serial.printf("Data Version read = %u\n", data_version);

  preferences.end();

}

void NvsPreferences::setEEPROMparams() {
 	//init preference
 	preferences.begin(kNvsDataKey, /*readOnly = */ false);

 	preferences.putUInt(kDataVersionKey, kDataVersion);
  Serial.printf("Data Version set = %u\n", kDataVersion);

 	preferences.end();
}