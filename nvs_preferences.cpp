#include "nvs_preferences.h"

NvsPreferences::NvsPreferences() {

  preferences.begin(kNvsDataKey, /*readOnly = */ false);

  // save key values
  // ADD NEW KEYS HERE
  if(!preferences.isKey(kAlarmHrKey))
    preferences.putUChar(kAlarmHrKey, kAlarmHr);
  if(!preferences.isKey(kAlarmMinKey))
    preferences.putUChar(kAlarmMinKey, kAlarmMin);
  if(!preferences.isKey(kAlarmIsAmKey))
    preferences.putBool(kAlarmIsAmKey, kAlarmIsAm);
  if(!preferences.isKey(kAlarmOnKey))
    preferences.putBool(kAlarmOnKey, kAlarmOn);
  if(!preferences.isKey(kWiFiSsidKey)) {
    String kWiFiSsidString = kWiFiSsid.c_str();
    preferences.putString(kWiFiSsidKey, kWiFiSsidString);
  }
  if(!preferences.isKey(kWiFiPasswdKey)) {
    String kWiFiPasswdString = kWiFiPasswd.c_str();
    preferences.putString(kWiFiPasswdKey, kWiFiPasswdString);
  }
  if(!preferences.isKey(kWeatherZipCodeKey))
    preferences.putUInt(kWeatherZipCodeKey, kWeatherZipCode);
  if(!preferences.isKey(kWeatherCountryCodeKey)) {
    String kWeatherCountryCodeString = kWeatherCountryCode.c_str();
    preferences.putString(kWeatherCountryCodeKey, kWeatherCountryCodeString);
  }
  if(!preferences.isKey(kWeatherUnitsMetricNotImperialKey))
    preferences.putBool(kWeatherUnitsMetricNotImperialKey, kWeatherUnitsMetricNotImperial);
  if(!preferences.isKey(kAlarmLongPressSecondsKey))
    preferences.putUChar(kAlarmLongPressSecondsKey, kAlarmLongPressSeconds);
  if(!preferences.isKey(kFirmwareVersionKey)) {
    String kFirmwareVersionString = kFirmwareVersion.c_str();
    preferences.putString(kFirmwareVersionKey, kFirmwareVersionString);
  }
  else {
    // older firmware present, means device has older buzzer 12085 with rated frequency 2048 Hz
    if(!preferences.isKey(kBuzzerFrequencyKey))   // if key is not present then it is older buzzer
      preferences.putUShort(kBuzzerFrequencyKey, 2048);
  }
  if(!preferences.isKey(kBuzzerFrequencyKey))
    preferences.putUShort(kBuzzerFrequencyKey, kBuzzerFrequency);
  if(!preferences.isKey(kCpuSpeedMhzKey))
    preferences.putUChar(kCpuSpeedMhzKey, cpu_speed_mhz);
  if(!preferences.isKey(kScreensaverMotionTypeKey))
    preferences.putBool(kScreensaverMotionTypeKey, true);
  if(!preferences.isKey(kNightTimeDimHourKey))
    preferences.putUChar(kNightTimeDimHourKey, kNightTimeDimHour);
  if(!preferences.isKey(kScreenOrientationKey))
    preferences.putUChar(kScreenOrientationKey, kScreenOrientation);
  if(!preferences.isKey(kAutorunRgbLedStripModeKey))
    preferences.putUChar(kAutorunRgbLedStripModeKey, kAutorunRgbLedStripMode);
  if(!preferences.isKey(kUseLDRKey))
    preferences.putBool(kUseLDRKey, kUseLDR);
  if(!preferences.isKey(kTouchscreenTypeKey))
    preferences.putUChar(kTouchscreenTypeKey, kTouchscreenType);
  if(!preferences.isKey(kTouchscreenFlipKey))
    preferences.putBool(kTouchscreenFlipKey, kTouchscreenFlip);
  if(!preferences.isKey(kRgbStripLedCountKey))
    preferences.putUChar(kRgbStripLedCountKey, kRgbStripLedCount);
  if(!preferences.isKey(kRgbStripLedBrightnessKey))
    preferences.putUChar(kRgbStripLedBrightnessKey, kRgbStripLedBrightness);

  // save new key values
  // ADD NEW KEYS ABOVE
  preferences.end();

  // nvs_preferences->PrintSavedData();

  PrintLn(__func__);
}

void NvsPreferences::PrintSavedData() {
  uint8_t long_press_seconds;
  RetrieveLongPressSeconds(long_press_seconds);
  uint8_t alarmHr, alarmMin;
  bool alarmIsAm, alarmOn;
  RetrieveAlarmSettings(alarmHr, alarmMin, alarmIsAm, alarmOn);
  std::string wifi_ssid, wifi_password;
  RetrieveWiFiDetails(wifi_ssid, wifi_password);
  uint32_t location_zip_code;
  std::string location_country_code;
  bool weather_units_metric_not_imperial;
  RetrieveWeatherLocationDetails(location_zip_code, location_country_code, weather_units_metric_not_imperial);
  std::string savedFirmwareVersion;
  RetrieveSavedFirmwareVersion(savedFirmwareVersion);
}

void NvsPreferences::RetrieveLongPressSeconds(uint8_t &long_press_seconds) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  long_press_seconds = preferences.getUChar(kAlarmLongPressSecondsKey, kAlarmLongPressSeconds);
  preferences.end();
}

void NvsPreferences::SaveLongPressSeconds(uint8_t long_press_seconds) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kAlarmLongPressSecondsKey, long_press_seconds);
  preferences.end();
  PrintLn(__func__, long_press_seconds);
}

void NvsPreferences::RetrieveBuzzerFrequency(uint16_t &buzzer_freq) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  buzzer_freq = preferences.getUShort(kBuzzerFrequencyKey, kBuzzerFrequency);
  preferences.end();
  PrintLn(__func__, buzzer_freq);
}

void NvsPreferences::SaveBuzzerFrequency(uint16_t buzzer_freq) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUShort(kBuzzerFrequencyKey, buzzer_freq);
  preferences.end();
  PrintLn(__func__, buzzer_freq);
}

void NvsPreferences::RetrieveAlarmSettings(uint8_t &alarmHr, uint8_t &alarmMin, bool &alarmIsAm, bool &alarmOn) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  alarmHr = preferences.getUChar(kAlarmHrKey);
  alarmMin = preferences.getUChar(kAlarmMinKey);
  alarmIsAm = preferences.getBool(kAlarmIsAmKey);
  alarmOn = preferences.getBool(kAlarmOnKey);
  preferences.end();
  PrintLn(__func__, alarmOn);
}

void NvsPreferences::SaveAlarm(uint8_t alarmHr, uint8_t alarmMin, bool alarmIsAm, bool alarmOn) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kAlarmHrKey, alarmHr);
  preferences.putUChar(kAlarmMinKey, alarmMin);
  preferences.putBool(kAlarmIsAmKey, alarmIsAm);
  preferences.putBool(kAlarmOnKey, alarmOn);
  preferences.end();
  // Serial.printf("NVS Memory SaveAlarm %2d:%02d alarmIsAm=%d alarmOn=%d\n", alarmHr, alarmMin, alarmIsAm, alarmOn);
  PrintLn(__func__, alarmOn);
}

void NvsPreferences::RetrieveWiFiDetails(std::string &wifi_ssid, std::string &wifi_password) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  String kWiFiSsidString = preferences.getString(kWiFiSsidKey);
  String kWiFiPasswdString = preferences.getString(kWiFiPasswdKey);
  preferences.end();
  wifi_ssid = kWiFiSsidString.c_str();
  wifi_password = kWiFiPasswdString.c_str();
  // if(debug_mode) PrintLn("NVS Memory wifi_password: ", wifi_password.c_str());
  // PrintLn("WiFi details retrieved from NVS Memory.");
  PrintLn(__func__, wifi_ssid);
}

void NvsPreferences::SaveWiFiDetails(std::string wifi_ssid, std::string wifi_password) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  String kWiFiSsidString = wifi_ssid.c_str();
  preferences.putString(kWiFiSsidKey, kWiFiSsidString);
  String kWiFiPasswdString = wifi_password.c_str();
  preferences.putString(kWiFiPasswdKey, kWiFiPasswdString);
  preferences.end();
  // if(debug_mode) {
  //   PrintLn("NVS Memory wifi_ssid: ", wifi_ssid.c_str());
  //   PrintLn("NVS Memory wifi_password: ", wifi_password.c_str());
  // }
  PrintLn(__func__, wifi_ssid);
}

void NvsPreferences::RetrieveSavedFirmwareVersion(std::string &savedFirmwareVersion) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  String savedFirmwareVersionString = preferences.getString(kFirmwareVersionKey);
  preferences.end();
  savedFirmwareVersion = savedFirmwareVersionString.c_str();
  PrintLn(__func__, savedFirmwareVersion);
}

void NvsPreferences::SaveCurrentFirmwareVersion() {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  String kFirmwareVersionString = kFirmwareVersion.c_str();
  preferences.putString(kFirmwareVersionKey, kFirmwareVersionString);
  preferences.end();
  // PrintLn("Current Firmware Version written to NVS Memory");
  PrintLn(__func__, kFirmwareVersion);
}

void NvsPreferences::RetrieveWeatherLocationDetails(uint32_t &location_zip_code, std::string &location_country_code, bool &weather_units_metric_not_imperial) {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  location_zip_code = preferences.getUInt(kWeatherZipCodeKey);
  String kWeatherCountryCodeString = preferences.getString(kWeatherCountryCodeKey);
  location_country_code = kWeatherCountryCodeString.c_str();
  weather_units_metric_not_imperial = preferences.getBool(kWeatherUnitsMetricNotImperialKey);
  preferences.end();
  PrintLn(__func__, location_zip_code);
  PrintLn(__func__, location_country_code);
  PrintLn(__func__, weather_units_metric_not_imperial);
}

void NvsPreferences::SaveWeatherLocationDetails(uint32_t location_zip_code, std::string location_country_code, bool weather_units_metric_not_imperial) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUInt(kWeatherZipCodeKey, location_zip_code);
  String kWeatherCountryCodeString = location_country_code.c_str();
  preferences.putString(kWeatherCountryCodeKey, kWeatherCountryCodeString);
  preferences.putBool(kWeatherUnitsMetricNotImperialKey, weather_units_metric_not_imperial);
  preferences.end();
  PrintLn(__func__, location_zip_code);
  PrintLn(__func__, location_country_code);
  PrintLn(__func__, weather_units_metric_not_imperial);
}

void NvsPreferences::SaveWeatherUnits(bool weather_units_metric_not_imperial) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putBool(kWeatherUnitsMetricNotImperialKey, weather_units_metric_not_imperial);
  preferences.end();
  PrintLn(__func__, weather_units_metric_not_imperial);
}

uint8_t NvsPreferences::RetrieveSavedCpuSpeed() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t saved_cpu_speed_mhz = preferences.getUChar(kCpuSpeedMhzKey);
  preferences.end();
  PrintLn(__func__, saved_cpu_speed_mhz);
  return saved_cpu_speed_mhz;
}

void NvsPreferences::SaveCpuSpeed() {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kCpuSpeedMhzKey, cpu_speed_mhz);
  preferences.end();
  PrintLn(__func__, cpu_speed_mhz);
}

bool NvsPreferences::RetrieveScreensaverBounceNotFlyHorizontally() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  bool screensaver_bounce_not_fly_horiontally = preferences.getBool(kScreensaverMotionTypeKey);
  preferences.end();
  PrintLn(__func__, screensaver_bounce_not_fly_horiontally);
  return screensaver_bounce_not_fly_horiontally;
}

void NvsPreferences::SaveScreensaverBounceNotFlyHorizontally(bool screensaver_bounce_not_fly_horiontally) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putBool(kScreensaverMotionTypeKey, screensaver_bounce_not_fly_horiontally);
  preferences.end();
  PrintLn(__func__, screensaver_bounce_not_fly_horiontally);
}

uint8_t NvsPreferences::RetrieveNightTimeDimHour() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t night_time_dim_hour = preferences.getUChar(kNightTimeDimHourKey);
  preferences.end();
  PrintLn(__func__, night_time_dim_hour);
  return night_time_dim_hour;
}

void NvsPreferences::SaveNightTimeDimHour(uint8_t night_time_dim_hour) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kNightTimeDimHourKey, night_time_dim_hour);
  preferences.end();
  PrintLn(__func__, night_time_dim_hour);
}

uint8_t NvsPreferences::RetrieveScreenOrientation() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t screen_orientation = preferences.getUChar(kScreenOrientationKey);
  preferences.end();
  PrintLn(__func__, screen_orientation);
  return screen_orientation;
}

void NvsPreferences::SaveScreenOrientation(uint8_t screen_orientation) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kScreenOrientationKey, screen_orientation);
  preferences.end();
  PrintLn(__func__, screen_orientation);
}

uint8_t NvsPreferences::RetrieveAutorunRgbLedStripMode() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t autorun_rgb_led_strip_mode_retrieved = preferences.getUChar(kAutorunRgbLedStripModeKey, 0);
  preferences.end();
  PrintLn(__func__, autorun_rgb_led_strip_mode_retrieved);
  return autorun_rgb_led_strip_mode_retrieved;
}

void NvsPreferences::SaveAutorunRgbLedStripMode(uint8_t autorun_rgb_led_strip_mode_to_save) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kAutorunRgbLedStripModeKey, autorun_rgb_led_strip_mode_to_save);
  preferences.end();
  PrintLn(__func__, autorun_rgb_led_strip_mode_to_save);
}

bool NvsPreferences::RetrieveUseLdr() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  bool use_ldr = preferences.getBool(kUseLDRKey);
  preferences.end();
  PrintLn(__func__, use_ldr);
  return use_ldr;
}

void NvsPreferences::SaveUseLdr(bool use_ldr) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putBool(kUseLDRKey, use_ldr);
  preferences.end();
  PrintLn(__func__, use_ldr);
}

uint8_t NvsPreferences::RetrieveTouchscreenType() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t touchscreen_type = preferences.getUChar(kTouchscreenTypeKey);
  preferences.end();
  PrintLn(__func__, touchscreen_type);
  return touchscreen_type;
}

void NvsPreferences::SaveTouchscreenType(uint8_t touchscreen_type) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kTouchscreenTypeKey, touchscreen_type);
  preferences.end();
  PrintLn(__func__, touchscreen_type);
}

bool NvsPreferences::RetrieveTouchscreenFlip() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  bool touchscreen_flip = preferences.getBool(kTouchscreenFlipKey);
  preferences.end();
  PrintLn(__func__, touchscreen_flip);
  return touchscreen_flip;
}

void NvsPreferences::SaveTouchscreenFlip(bool touchscreen_flip) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putBool(kTouchscreenFlipKey, touchscreen_flip);
  preferences.end();
  PrintLn(__func__, touchscreen_flip);
}

uint8_t NvsPreferences::RetrieveRgbStripLedCount() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t rgb_strip_led_count = preferences.getUChar(kRgbStripLedCountKey, 0);
  preferences.end();
  PrintLn(__func__, rgb_strip_led_count);
  return rgb_strip_led_count;
}

void NvsPreferences::SaveRgbStripLedCount(uint8_t rgb_strip_led_count) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kRgbStripLedCountKey, rgb_strip_led_count);
  preferences.end();
  PrintLn(__func__, rgb_strip_led_count);
}

uint8_t NvsPreferences::RetrieveRgbStripLedBrightness() {
  preferences.begin(kNvsDataKey, /*readOnly = */ true);
  uint8_t rgb_strip_led_brightness = preferences.getUChar(kRgbStripLedBrightnessKey, 0);
  preferences.end();
  PrintLn(__func__, rgb_strip_led_brightness);
  return rgb_strip_led_brightness;
}

void NvsPreferences::SaveRgbStripLedBrightness(uint8_t rgb_strip_led_brightness) {
  preferences.begin(kNvsDataKey, /*readOnly = */ false);
  preferences.putUChar(kRgbStripLedBrightnessKey, rgb_strip_led_brightness);
  preferences.end();
  PrintLn(__func__, rgb_strip_led_brightness);
}
