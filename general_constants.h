#ifndef GENERAL_CONSTANTS_H
#define GENERAL_CONSTANTS_H

// max user inactivity seconds before turning down brightness or turning On screensaver
const uint32_t kInactivityMillisLimit = 6000;

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

const int16_t kCancelButtonSize = 21, kCancelButtonX1 = kTftWidth - kCancelButtonSize - kDisplayTextGap, kCancelButtonY1 = kTftHeight - kCancelButtonSize - kDisplayTextGap;
const int16_t kSaveButtonW = 60, kSaveButtonH = kCancelButtonSize, kSaveButtonX1 = kTftWidth - kSaveButtonW - kCancelButtonSize - 2*kDisplayTextGap, kSaveButtonY1 = kTftHeight - kSaveButtonH - kDisplayTextGap;

const uint8_t kSettingsGearWidth = 40, kSettingsGearHeight = 40;
const int16_t kSettingsGearX1 = kTftWidth - kSettingsGearWidth - 10, kSettingsGearY1 = kDateRow_Y0 - kSettingsGearHeight + 5;

const char saveStr[5] = "SAVE", setStr[4] = "SET", cancelStr[2] = "X";
const char slowStr[5] = "SLOW", medStr[7] = "MEDIUM", fastStr[5] = "FAST", flyOutScreensaverStr[8] = "FLY OUT", bounceScreensaverStr[7] = "BOUNCE";
const char metricUnitStr[7] = "METRIC", imperialUnitStr[9] = "IMPERIAL";
const char manualStr[7] = "MANUAL", eveningStr[8] = "EVENING", sunDownStr[9] = "SUN-DOWN";

// time of day at which its respective brightness starts
const uint16_t kDayTimeMinutes = 420;   // 7AM
const uint16_t kEveningTimeMinutes = 1080;    // 6PM

#endif  // GENERAL_CONSTANTS_H