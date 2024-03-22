#ifndef GENERAL_CONSTANTS_H
#define GENERAL_CONSTANTS_H

// max user inactivity seconds before turning down brightness or turning On screensaver
const uint32_t kInactivityMillisLimit = 60000;

// user input delay
const uint32_t kUserInputDelayMs = 200;

// watchdog timeout time (RP2040 has a max watchdog timeout time of 8.3 seconds)
const unsigned long kWatchdogTimeoutMs = 8300;
const unsigned long kWatchdogTimeoutOtaUpdateMs = 90000;

// char array sizes for time, date and alarm texts
const int kHHMM_ArraySize = 6, kSS_ArraySize = 4, kDateArraySize = 13, kAlarmArraySize = 10;

// day arrays
#define DAY_ARR_SIZE 4
const char kDaySun[DAY_ARR_SIZE] = "Sun";
const char kDayMon[DAY_ARR_SIZE] = "Mon";
const char kDayTue[DAY_ARR_SIZE] = "Tue";
const char kDayWed[DAY_ARR_SIZE] = "Wed";
const char kDayThu[DAY_ARR_SIZE] = "Thu";
const char kDayFri[DAY_ARR_SIZE] = "Fri";
const char kDaySat[DAY_ARR_SIZE] = "Sat";

// Then set up a table to refer to your strings.
const char *const kDaysTable_[7] = { kDaySun, kDayMon, kDayTue, kDayWed, kDayThu, kDayFri, kDaySat };

// day arrays
#define MONTH_ARR_SIZE 4
const char kMonthJan[MONTH_ARR_SIZE] = "Jan";
const char kMonthFeb[MONTH_ARR_SIZE] = "Feb";
const char kMonthMar[MONTH_ARR_SIZE] = "Mar";
const char kMonthApr[MONTH_ARR_SIZE] = "Apr";
const char kMonthMay[MONTH_ARR_SIZE] = "May";
const char kMonthJun[MONTH_ARR_SIZE] = "Jun";
const char kMonthJul[MONTH_ARR_SIZE] = "Jul";
const char kMonthAug[MONTH_ARR_SIZE] = "Aug";
const char kMonthSep[MONTH_ARR_SIZE] = "Sep";
const char kMonthOct[MONTH_ARR_SIZE] = "Oct";
const char kMonthNov[MONTH_ARR_SIZE] = "Nov";
const char kMonthDec[MONTH_ARR_SIZE] = "Dec";

// Then set up a table to refer to your strings.
const char *const kMonthsTable[12] = { kMonthJan, kMonthFeb, kMonthMar, kMonthApr, kMonthMay, kMonthJun, kMonthJul, kMonthAug, kMonthSep, kMonthOct, kMonthNov, kMonthDec };

const char kAmLabel[3] = "AM", kPmLabel[3] = "PM", kOffLabel[4] = "Off", kAlarmLabel[6] = "Alarm";
const char kCharSpace = ' ', kCharZero = '0', kCharColon = ':';

const unsigned int kWifiSsidPasswordLengthMax = 32;

const char softApSsid[24] = "Long-Press-Alarm-SoftAP";


// DISPLAY ITEM LOCATIONS

  // tft dimensions
  const uint16_t kTftWidth = 320, kTftHeight = 240;

  // user defined locations of various text strings on display
  const int16_t kTimeRowX0 = 10, kTimeRowY0 = 80, kAM_PM_row_Y0 = 45, kTimeRowY0IncorrectTime = 100;
  const int16_t kDisplayTextGap = 5;
  const int16_t kDateRow_Y0 = 140;
  const int16_t kAlarmRowY0 = 210, kAlarmRowY1 = 160;
  const int16_t kRadiusButtonRoundRect = 5;
  const int kPageRowHeight = 30;

  const int16_t kCancelButtonSize = 40, kCancelButtonX1 = kTftWidth - kCancelButtonSize - 5, kCancelButtonY1 = kTftHeight - kCancelButtonSize - 5;
  const int16_t kSaveButtonW = 80, kSaveButtonH = 40, kSaveButtonX1 = kTftWidth - kSaveButtonW - kCancelButtonSize - 15, kSaveButtonY1 = kTftHeight - kSaveButtonH - 5;

  const uint8_t kSettingsGearWidth = 40, kSettingsGearHeight = 40;
  const int16_t kSettingsGearX1 = kTftWidth - kSettingsGearWidth - 10, kSettingsGearY1 = kDateRow_Y0 - kSettingsGearHeight + 5;

  const int16_t kWiFiSettingsButtonW = 100, kWiFiSettingsButtonX1 = kTftWidth - kWiFiSettingsButtonW - 5, kWiFiSettingsButtonY1 = 10, kWiFiSettingsButtonH = 25;
  const int16_t kWeatherSettingsButtonW = 230, kWeatherSettingsButtonX1 = kTftWidth - kWeatherSettingsButtonW - 5, kWeatherSettingsButtonY1 = 40, kWeatherSettingsButtonH = 25;
  const int16_t kAlarmLongPressSecondsX0 = 190, kAlarmLongPressSecondsY0 = 120, kAlarmLongPressSecondsW = 30, kAlarmLongPressSecondsH = 25, kAlarmLongPressSecondsY1 = kAlarmLongPressSecondsY0 - kAlarmLongPressSecondsH, kAlarmLongPressSecondsTriangleButtonsSize = 8;
  const int16_t kAlarmLongPressSecondsSetButtonW = 80, kAlarmLongPressSecondsSetButtonH = 25, kAlarmLongPressSecondsSetButtonX1 = kTftWidth - kAlarmLongPressSecondsSetButtonW - 5, kAlarmLongPressSecondsSetButtonY1 = kAlarmLongPressSecondsY1;
  const int16_t kRunScreensaverButtonW = 60, kRunScreensaverButtonX1 = kTftWidth - kRunScreensaverButtonW - 5, kRunScreensaverButtonY1 = 150, kRunScreensaverButtonH = 25;
  const int16_t kScreensaverMotionButtonW = 130, kScreensaverMotionButtonX1 = kTftWidth - kRunScreensaverButtonW - kScreensaverMotionButtonW - 15, kScreensaverMotionButtonY1 = 130, kScreensaverMotionButtonH = 25;
  const int16_t kScreensaverSpeedButtonW = 130, kScreensaverSpeedButtonX1 = kTftWidth - kRunScreensaverButtonW - kScreensaverSpeedButtonW - 15, kScreensaverSpeedButtonY1 = 160, kScreensaverSpeedButtonH = 25;

  const int16_t kUpdateButtonW = 40, kUpdateButtonX1 = kTftWidth - kCancelButtonSize - kUpdateButtonW - 20, kUpdateButtonH = 25, kUpdateButtonY1 = kTftHeight - kUpdateButtonH - 5;

  const int16_t kSetWiFiButtonW = 150, kSetWiFiButtonX1 = kTftWidth - kSetWiFiButtonW - 5, kSetWiFiButtonY1 = 0, kSetWiFiButtonH = 25;
  const int16_t kConnectWiFiButtonW = 130, kConnectWiFiButtonX1 = 5, kConnectWiFiButtonY1 = 90, kConnectWiFiButtonH = 25;
  const int16_t kDisconnectWiFiButtonW = 170, kDisconnectWiFiButtonX1 = kTftWidth - kDisconnectWiFiButtonW - 5, kDisconnectWiFiButtonY1 = 90, kDisconnectWiFiButtonH = 25;

  const int16_t kSetLocationButtonW = 100, kSetLocationButtonX1 = kTftWidth - kSetLocationButtonW - 5, kSetLocationButtonY1 = 0, kSetLocationButtonH = 25;
  const int16_t kSetCountryCodeButtonW = 130, kSetCountryCodeButtonX1 = kTftWidth - kSetCountryCodeButtonW - 5, kSetCountryCodeButtonY1 = 30, kSetCountryCodeButtonH = 25;
  const int16_t kUnitsButtonW = 120, kUnitsButtonX1 = kTftWidth - kUnitsButtonW - 5, kUnitsButtonY1 = 60, kUnitsButtonH = 25;
  const int16_t kFetchWeatherButtonW = 110, kFetchWeatherButtonX1 = 5, kFetchWeatherButtonY1 = 90, kFetchWeatherButtonH = 25;
  const int16_t kUpdateTimeButtonW = 180, kUpdateTimeButtonX1 = kTftWidth - kUpdateTimeButtonW - 5, kUpdateTimeButtonY1 = 90, kUpdateTimeButtonH = 25;

  const char saveStr[5] = "SAVE", setStr[4] = "SET", cancelStr[2] = "X", wifiSettingsStr[5] = "WiFi", setWiFiStr[9] = "SET WIFI", weatherStr[15] = "WEATHER & TIME";
  const char slowStr[5] = "SLOW", medStr[7] = "MEDIUM", fastStr[5] = "FAST", runScreensaverStr[4] = "RUN", flyOutScreensaverStr[8] = "FLY OUT", bounceScreensaverStr[7] = "BOUNCE", updateStr[3] = "UP";
  const char fetchStr[8] = "FETCH", updateTimeStr[12] = "UPDATE TIME", zipPinCodeStr[8] = "ZIP/PIN", countryCodeStr[8] = "COUNTRY";
  const char unitsStr[7] = "UNITS", metricUnitStr[7] = "METRIC", imperialUnitStr[9] = "IMPERIAL", connectWiFiStr[10] = "CONNECT", disconnectWiFiStr[11] = "DISCONNECT";



#endif  // GENERAL_CONSTANTS_H